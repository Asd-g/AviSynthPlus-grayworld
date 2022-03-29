#include <cmath>
#include <memory>

#include "avisynth.h"

void apply_matrix(const float matrix[3][3], const float input[3], float output[3])
{
    output[0] = matrix[0][0] * input[0] + matrix[0][1] * input[1] + matrix[0][2] * input[2];
    output[1] = matrix[1][0] * input[0] + matrix[1][1] * input[1] + matrix[1][2] * input[2];
    output[2] = matrix[2][0] * input[0] + matrix[2][1] * input[1] + matrix[2][2] * input[2];
}

static constexpr float lms2lab[3][3]{
    {0.5774f, 0.5774f, 0.5774f},
    {0.40825f, 0.40825f, -0.816458f},
    {0.707f, -0.707f, 0.0f}
};

static constexpr float lab2lms[3][3]{
    {0.57735f, 0.40825f, 0.707f},
    {0.57735f, 0.40825f, -0.707f},
    {0.57735f, -0.8165f, 0.0f}
};

static constexpr float rgb2lms[3][3]{
    {0.3811f, 0.5783f, 0.0402f},
    {0.1967f, 0.7244f, 0.0782f},
    {0.0241f, 0.1288f, 0.8444f}
};

static constexpr float lms2rgb[3][3]{
    {4.4679f, -3.5873f, 0.1193f},
    {-1.2186f, 2.3809f, -0.1624f},
    {0.0497f, -0.2439f, 1.2045f}
};

void rgb2lab(const float rgb[3], float lab[3])
{
    float lms[3];

    apply_matrix(rgb2lms, rgb, lms);

    lms[0] = (lms[0] > 0.0f) ? logf(lms[0]) : -1024.0f;
    lms[1] = (lms[1] > 0.0f) ? logf(lms[1]) : -1024.0f;
    lms[2] = (lms[2] > 0.0f) ? logf(lms[2]) : -1024.0f;

    apply_matrix(lms2lab, lms, lab);
}

void lab2rgb(const float lab[3], float rgb[3])
{
    float lms[3];

    apply_matrix(lab2lms, lab, lms);

    lms[0] = expf(lms[0]);
    lms[1] = expf(lms[1]);
    lms[2] = expf(lms[2]);

    apply_matrix(lms2rgb, lms, rgb);
}

void convert_frame(float* tmpplab, PVideoFrame& src, float* line_sum, int* line_count_pels, const int pitch, const int width, const int height)
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

        line_sum[y] = 0.0f;
        line_sum[y + height] = 0.0f;
        line_count_pels[y] = 0;

        for (int x{ 0 }; x < width; ++x)
        {
            rgb[0] = r[x];
            rgb[1] = g[x];
            rgb[2] = b[x];

            rgb2lab(rgb, lab);

            *(lcur++) = lab[0];
            *(acur++) = lab[1];
            *(bcur++) = lab[2];

            line_sum[y] += lab[1];
            line_sum[y + height] += lab[2];
            line_count_pels[y]++;
        }

        r += pitch;
        g += pitch;
        b += pitch;
    }
}

std::pair<float, float> compute_correction(float* line_sum, int* line_count_pels, const int height)
{
    float asum = 0.0f;
    float bsum = 0.0f;
    int pixels = 0;

    for (int y{ 0 }; y < height; ++y)
    {
        asum += line_sum[y];
        bsum += line_sum[y + height];
        pixels += line_count_pels[y];
    }

    return std::make_pair<float, float>(asum / pixels, bsum / pixels);
}

void correct_frame(PVideoFrame& dst, float* tmpplab, std::pair<float, float>& avg, const int pitch, const int width, const int height)
{
    float* r{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_R)) };
    float* g{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_G)) };
    float* b{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_B)) };

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
            lab2rgb(lab, rgb);
            r[x] = rgb[0];
            g[x] = rgb[1];
            b[x] = rgb[2];
        }

        r += pitch;
        g += pitch;
        b += pitch;
    }
}

class grayworld : public GenericVideoFilter
{
    std::pair<float, float> avg;

    std::unique_ptr<float[]>tmpplab;
    std::unique_ptr<int[]>line_count_pels;
    std::unique_ptr<float[]>line_sum;

public:
    grayworld(PClip _child, IScriptEnvironment* env)
        : GenericVideoFilter(_child)
    {
        if (!vi.IsRGB() || !vi.IsPlanar() || vi.ComponentSize() != 4)
            env->ThrowError("grayworld: clip must be in RGB 32-bit planar format.");

        tmpplab = std::make_unique<float[]>(vi.height * vi.width * 3);
        line_count_pels = std::make_unique<int[]>(vi.height);
        line_sum = std::make_unique<float[]>(vi.height * 2);
    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
    {
        PVideoFrame src{ child->GetFrame(n, env) };
        PVideoFrame dst{ env->NewVideoFrameP(vi, &src) };

        const int pitch{ src->GetPitch() };
        const int dst_pitch{ dst->GetPitch() };
        const int width{ src->GetRowSize() / 4 };
        const int height{ src->GetHeight() };

        convert_frame(tmpplab.get(), src, line_sum.get(), line_count_pels.get(), pitch / 4, width, height);
        avg = compute_correction(line_sum.get(), line_count_pels.get(), height);
        correct_frame(dst, tmpplab.get(), avg, dst_pitch / 4, width, height);

        if (vi.NumComponents() == 4)
            env->BitBlt(dst->GetWritePtr(PLANAR_A), dst_pitch, src->GetReadPtr(PLANAR_A), pitch, src->GetRowSize(), height);

        return dst;
    }

    int __stdcall SetCacheHints(int cachehints, int frame_range)
    {
        return cachehints == CACHE_GET_MTMODE ? MT_MULTI_INSTANCE : 0;
    }
};

AVSValue __cdecl Create_grayworld(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new grayworld(args[0].AsClip(), env);
}

const AVS_Linkage* AVS_linkage;

extern "C" __declspec(dllexport)
const char* __stdcall AvisynthPluginInit3(IScriptEnvironment * env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;

    env->AddFunction("grayworld", "c", Create_grayworld, 0);

    return "grayworld";
}
