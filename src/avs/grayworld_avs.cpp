#include "grayworld_avs.h"

template <grayworld_mode mode>
static void convert_frame_c(float* __restrict tmpplab, PVideoFrame& src, float* line_sum, int* line_count_pels, const int pitch, const int width, const int height) noexcept
{
    const float* r{ reinterpret_cast<const float*>(src->GetReadPtr(PLANAR_R)) };
    const float* g{ reinterpret_cast<const float*>(src->GetReadPtr(PLANAR_G)) };
    const float* b{ reinterpret_cast<const float*>(src->GetReadPtr(PLANAR_B)) };

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

static void correct_frame_c(PVideoFrame& dst, float* tmpplab, std::pair<float, float>& avg, const int pitch, const int width, const int height) noexcept
{
    float* __restrict r{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_R)) };
    float* __restrict g{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_G)) };
    float* __restrict b{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_B)) };

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

grayworld::grayworld(PClip _child, int opt, grayworld_mode mode, IScriptEnvironment* env)
    : GenericVideoFilter(_child)
{
    if (!vi.IsRGB() || !vi.IsPlanar() || vi.ComponentSize() != 4)
        env->ThrowError("grayworld: clip must be in RGB 32-bit planar format.");
    if (opt < -1 || opt > 3)
        env->ThrowError("grayworld: opt must be between -1..3.");

    const bool avx512{ !!(env->GetCPUFlags() & CPUF_AVX512F) };
    if (!avx512 && opt == 3)
        env->ThrowError("grayworld: opt=3 requires AVX512F.");

    const bool avx2{ !!(env->GetCPUFlags() & CPUF_AVX2) };
    if (!avx2 && opt == 2)
        env->ThrowError("grayworld: opt=2 requires AVX2.");

    const bool sse2{ !!(env->GetCPUFlags() & CPUF_SSE2) };
    if (!sse2 && opt == 1)
        env->ThrowError("grayworld: opt=1 requires SSE2.");

    if ((avx512 && opt < 0) || opt == 3)
    {
        if (mode == grayworld_mode::mean)
        {
            convert = convert_frame_avx512<grayworld_mode::mean>;
            compute = compute_correction<grayworld_mode::mean>;
        }
        else
        {
            convert = convert_frame_avx512<grayworld_mode::median>;
            compute = compute_correction<grayworld_mode::median>;
        }

        correct = correct_frame_avx512;
    }
    else if ((avx2 && opt < 0) || opt == 2)
    {
        if (mode == grayworld_mode::mean)
        {
            convert = convert_frame_avx2<grayworld_mode::mean>;
            compute = compute_correction<grayworld_mode::mean>;
        }
        else
        {
            convert = convert_frame_avx2<grayworld_mode::median>;
            compute = compute_correction<grayworld_mode::median>;
        }

        correct = correct_frame_avx2;
    }
    else if ((sse2 && opt < 0) || opt == 1)
    {
        if (mode == grayworld_mode::mean)
        {
            convert = convert_frame_sse2<grayworld_mode::mean>;
            compute = compute_correction<grayworld_mode::mean>;
        }
        else
        {
            convert = convert_frame_sse2<grayworld_mode::median>;
            compute = compute_correction<grayworld_mode::median>;
        }

        correct = correct_frame_sse2;
    }
    else
    {
        if (mode == grayworld_mode::mean)
        {
            convert = convert_frame_c<grayworld_mode::mean>;
            compute = compute_correction<grayworld_mode::mean>;
        }
        else
        {
            convert = convert_frame_c<grayworld_mode::median>;
            compute = compute_correction<grayworld_mode::median>;
        }

        correct = correct_frame_c;
    }

    tmpplab = std::make_unique<float[]>(vi.height * vi.width * 3);
    line_count_pels = std::make_unique<int[]>(vi.height);
    line_sum = std::make_unique<float[]>(vi.height * 2);
}

PVideoFrame __stdcall grayworld::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src{ child->GetFrame(n, env) };
    PVideoFrame dst{ env->NewVideoFrameP(vi, &src) };

    const int pitch{ src->GetPitch() };
    const int dst_pitch{ dst->GetPitch() };
    const int width{ src->GetRowSize() / 4 };
    const int height{ src->GetHeight() };

    convert(tmpplab.get(), src, line_sum.get(), line_count_pels.get(), pitch / 4, width, height);
    avg = compute(line_sum.get(), line_count_pels.get(), height);
    correct(dst, tmpplab.get(), avg, dst_pitch / 4, width, height);

    if (vi.NumComponents() == 4)
        env->BitBlt(dst->GetWritePtr(PLANAR_A), dst_pitch, src->GetReadPtr(PLANAR_A), pitch, src->GetRowSize(), height);

    return dst;
}

AVSValue __cdecl Create_grayworld(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    enum { CLIP, OPT, CC };

    const int cc{ args[CC].AsInt(0) };
    if (cc < 0 || cc > 1)
        env->ThrowError("grayworld: cc must be either 0 or 1.");

    return new grayworld(args[CLIP].AsClip(), args[OPT].AsInt(-1), (cc == 0) ? grayworld_mode::mean : grayworld_mode::median, env);
}

const AVS_Linkage* AVS_linkage;

extern "C" __declspec(dllexport)
const char* __stdcall AvisynthPluginInit3(IScriptEnvironment * env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;

    env->AddFunction("grayworld", "c[opt]i[cc]i", Create_grayworld, 0);

    return "grayworld";
}
