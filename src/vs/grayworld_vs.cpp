#include <string>

#include "grayworld_vs.h"
#include "../VCL2/instrset.h"

using namespace std::literals;

template <grayworld_mode mode>
static void convert_frame_c(float* __restrict tmpplab, const VSFrame* src, float* line_sum, int* line_count_pels, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept
{
    const float* r{ reinterpret_cast<const float*>(vsapi->getReadPtr(src, 0)) };
    const float* g{ reinterpret_cast<const float*>(vsapi->getReadPtr(src, 1)) };
    const float* b{ reinterpret_cast<const float*>(vsapi->getReadPtr(src, 2)) };

    float rgb[3];
    float lab[3];

    for (int y{ 0 }; y < height; ++y)
    {
        float* acur{ tmpplab + y * width + width * height };
        float* bcur{ tmpplab + y * width + 2 * width * height };
        float* lcur{ tmpplab + y * width };

        if constexpr (mode == grayworld_mode::mean)
        {
            line_sum[y] = 0.0f;
            line_sum[y + height] = 0.0f;
            line_count_pels[y] = 0;

            for (int x{ 0 }; x < width; ++x)
            {
                rgb[0] = r[x];
                rgb[1] = g[x];
                rgb[2] = b[x];

                rgb2lab_c(rgb, lab);

                *(lcur++) = lab[0];
                *(acur++) = lab[1];
                *(bcur++) = lab[2];

                line_sum[y] += lab[1];
                line_sum[y + height] += lab[2];
                line_count_pels[y]++;
            }
        }
        else
        {
            std::vector<float> m0;
            m0.reserve(width);
            std::vector<float> m1;
            m1.reserve(width);

            for (int x{ 0 }; x < width; ++x)
            {
                rgb[0] = r[x];
                rgb[1] = g[x];
                rgb[2] = b[x];

                rgb2lab_c(rgb, lab);

                *(lcur++) = lab[0];
                *(acur++) = lab[1];
                *(bcur++) = lab[2];

                m0.emplace_back(lab[1]);
                m1.emplace_back(lab[2]);
            }

            const auto middleItr{ m0.begin() + m0.size() / 2 };
            std::nth_element(m0.begin(), middleItr, m0.end());
            line_sum[y] = (m0.size() % 2 == 0) ? ((*(std::max_element(m0.begin(), middleItr)) + *middleItr) / 2) : *middleItr;

            const auto middleItr1{ m1.begin() + m1.size() / 2 };
            std::nth_element(m1.begin(), middleItr1, m1.end());
            line_sum[y + height] = (m1.size() % 2 == 0) ? ((*(std::max_element(m1.begin(), middleItr1)) + *middleItr1) / 2) : *middleItr1;
        }

        r += pitch;
        g += pitch;
        b += pitch;
    }
}

static void correct_frame_c(VSFrame* dst, float* tmpplab, std::pair<float, float>& avg, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept
{
    float* __restrict r{ reinterpret_cast<float*>(vsapi->getWritePtr(dst, 0)) };
    float* __restrict g{ reinterpret_cast<float*>(vsapi->getWritePtr(dst, 1)) };
    float* __restrict b{ reinterpret_cast<float*>(vsapi->getWritePtr(dst, 2)) };

    float rgb[3];
    float lab[3];

    for (int y{ 0 }; y < height; ++y)
    {
        float* lcur{ tmpplab + y * width };
        float* acur{ tmpplab + y * width + width * height };
        float* bcur{ tmpplab + y * width + 2 * width * height };

        for (int x{ 0 }; x < width; ++x)
        {
            lab[0] = *lcur++;
            lab[1] = *acur++;
            lab[2] = *bcur++;

            // subtract the average for the color channels
            lab[1] -= avg.first;
            lab[2] -= avg.second;

            //convert back to linear rgb
            lab2rgb_c(lab, rgb);
            r[x] = std::clamp(rgb[0], 0.0f, 1.0f);
            g[x] = std::clamp(rgb[1], 0.0f, 1.0f);
            b[x] = std::clamp(rgb[2], 0.0f, 1.0f);
        }

        r += pitch;
        g += pitch;
        b += pitch;
    }
}

static const VSFrame* VS_CC grayworldGetFrame(int n, int activationReason, void* instanceData, [[maybe_unused]] void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi)
{
    grayworldData* d{ static_cast<grayworldData*>(instanceData) };

    if (activationReason == arInitial)
        vsapi->requestFrameFilter(n, d->node, frameCtx);
    else if (activationReason == arAllFramesReady)
    {
        const VSFrame* src{ vsapi->getFrameFilter(n, d->node, frameCtx) };
        VSFrame* dst{ vsapi->newVideoFrame(&d->vi->format, d->vi->width, d->vi->height, src, core) };

        const ptrdiff_t stride{ vsapi->getStride(src, 0) };
        const ptrdiff_t width{ vsapi->getFrameWidth(src, 0) };
        const ptrdiff_t height{ vsapi->getFrameHeight(src, 0) };

        d->convert(d->tmpplab.get(), src, d->line_sum.get(), d->line_count_pels.get(), stride / 4, width, height, vsapi);
        d->avg = d->compute(d->line_sum.get(), d->line_count_pels.get(), height);
        d->correct(dst, d->tmpplab.get(), d->avg, stride / 4, width, height, vsapi);

        vsapi->freeFrame(src);
        return dst;
    }

    return nullptr;
}

static void VS_CC grayworldFree(void* instanceData, [[maybe_unused]] VSCore* core, const VSAPI* vsapi) {
    grayworldData* d{ static_cast<grayworldData*>(instanceData) };
    vsapi->freeNode(d->node);
    delete d;
}

static void VS_CC grayworldCreate(const VSMap* in, VSMap* out, [[maybe_unused]] void* userData, VSCore* core, const VSAPI* vsapi)
{
    std::unique_ptr<grayworldData> d{ std::make_unique<grayworldData>() };

    try
    {
        d->node = vsapi->mapGetNode(in, "clip", 0, nullptr);
        d->vi = vsapi->getVideoInfo(d->node);
        int err{ 0 };

        if (d->vi->format.colorFamily != cfRGB || d->vi->format.sampleType != stFloat || d->vi->format.bytesPerSample != 4)
            throw "clip must be in RGB 32-bit planar format."s;

        int64_t opt{ vsapi->mapGetInt(in, "opt", 0, &err) };
        if (err)
            opt = -1;
        if (opt < -1 || opt > 3)
            throw "opt must be between -1..3."s;

        int64_t cc{ vsapi->mapGetIntSaturated(in, "cc", 0, &err) };
        if (err)
            cc = 0;
        if (cc < 0 || cc > 1)
            throw "cc must be either 0 or 1."s;

        const int iset{ instrset_detect() };

        if ((opt == -1 && iset >= 10) || opt == 3)
        {
            if (cc == 0)
            {
                d->convert = convert_frame_avx512<grayworld_mode::mean>;
                d->compute = compute_correction<grayworld_mode::mean>;
            }
            else
            {
                d->convert = convert_frame_avx512<grayworld_mode::median>;
                d->compute = compute_correction<grayworld_mode::median>;
            }

            d->correct = correct_frame_avx512;
        }
        else if ((opt == -1 && iset >= 8) || opt == 2)
        {
            if (cc == 0)
            {
                d->convert = convert_frame_avx2<grayworld_mode::mean>;
                d->compute = compute_correction<grayworld_mode::mean>;
            }
            else
            {
                d->convert = convert_frame_avx2<grayworld_mode::median>;
                d->compute = compute_correction<grayworld_mode::median>;
            }

            d->correct = correct_frame_avx2;
        }
        else if ((opt == -1 && iset >= 2) || opt == 1)
        {
            if (cc == 0)
            {
                d->convert = convert_frame_sse2<grayworld_mode::mean>;
                d->compute = compute_correction<grayworld_mode::mean>;
            }
            else
            {
                d->convert = convert_frame_sse2<grayworld_mode::median>;
                d->compute = compute_correction<grayworld_mode::median>;
            }

            d->correct = correct_frame_sse2;
        }
        else
        {
            if (cc == 0)
            {
                d->convert = convert_frame_c<grayworld_mode::mean>;
                d->compute = compute_correction<grayworld_mode::mean>;
            }
            else
            {
                d->convert = convert_frame_c<grayworld_mode::median>;
                d->compute = compute_correction<grayworld_mode::median>;
            }

            d->correct = correct_frame_c;
        }

        d->tmpplab = std::make_unique<float[]>(d->vi->height * d->vi->width * 3);
        d->line_count_pels = std::make_unique<int[]>(d->vi->height);
        d->line_sum = std::make_unique<float[]>(d->vi->height * 2);
    }
    catch (const std::string& error)
    {
        vsapi->mapSetError(out, ("grayworld: " + error).c_str());
        vsapi->freeNode(d->node);
        return;
    }

    VSFilterDependency deps[] = { {d->node, rpStrictSpatial} };
    vsapi->createVideoFilter(out, "grayworld", d->vi, grayworldGetFrame, grayworldFree, fmParallel, deps, 1, d.get(), core);
    d.release();
}

VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin* plugin, const VSPLUGINAPI* vspapi)
{
    vspapi->configPlugin("com.vapoursynth.grayworld", "grwrld", "A color correction based on the grayworld assumption", VS_MAKE_VERSION(1, 0), VAPOURSYNTH_API_VERSION, 0, plugin);
    vspapi->registerFunction("grayworld",
        "clip:vnode;"
        "opt:int:opt;"
        "cc:int:opt;",
        "clip:vnode;",
        grayworldCreate, nullptr, plugin);
}
