#include <cmath>

#include "grayworld.h"

void apply_matrix_c(const float matrix[3][3], const float input[3], float output[3])
{
    output[0] = matrix[0][0] * input[0] + matrix[0][1] * input[1] + matrix[0][2] * input[2];
    output[1] = matrix[1][0] * input[0] + matrix[1][1] * input[1] + matrix[1][2] * input[2];
    output[2] = matrix[2][0] * input[0] + matrix[2][1] * input[1] + matrix[2][2] * input[2];
}

void rgb2lab_c(const float rgb[3], float lab[3])
{
    float lms[3];

    apply_matrix_c(rgb2lms, rgb, lms);

    lms[0] = (lms[0] > 0.0f) ? logf(lms[0]) : -1024.0f;
    lms[1] = (lms[1] > 0.0f) ? logf(lms[1]) : -1024.0f;
    lms[2] = (lms[2] > 0.0f) ? logf(lms[2]) : -1024.0f;

    apply_matrix_c(lms2lab, lms, lab);
}

void lab2rgb_c(const float lab[3], float rgb[3])
{
    float lms[3];

    apply_matrix_c(lab2lms, lab, lms);

    lms[0] = expf(lms[0]);
    lms[1] = expf(lms[1]);
    lms[2] = expf(lms[2]);

    apply_matrix_c(lms2rgb, lms, rgb);
}

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

        r += pitch;
        g += pitch;
        b += pitch;
    }
}

static std::pair<float, float> compute_correction(float* line_sum, int* line_count_pels, const int height)
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

grayworld::grayworld(PClip _child, int opt, IScriptEnvironment* env)
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
        convert = convert_frame_avx512;
        correct = correct_frame_avx512;
    }
    else if ((avx2 && opt < 0) || opt == 2)
    {
        convert = convert_frame_avx2;
        correct = correct_frame_avx2;
    }
    else if ((sse2 && opt < 0) || opt == 1)
    {
        convert = convert_frame_sse2;
        correct = correct_frame_sse2;
    }
    else
    {
        convert = convert_frame_c;
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
    avg = compute_correction(line_sum.get(), line_count_pels.get(), height);
    correct(dst, tmpplab.get(), avg, dst_pitch / 4, width, height);

    if (vi.NumComponents() == 4)
        env->BitBlt(dst->GetWritePtr(PLANAR_A), dst_pitch, src->GetReadPtr(PLANAR_A), pitch, src->GetRowSize(), height);

    return dst;
}

AVSValue __cdecl Create_grayworld(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    enum { CLIP, OPT };

    return new grayworld(args[CLIP].AsClip(), args[OPT].AsInt(-1), env);
}

const AVS_Linkage* AVS_linkage;

extern "C" __declspec(dllexport)
const char* __stdcall AvisynthPluginInit3(IScriptEnvironment * env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;

    env->AddFunction("grayworld", "c[opt]i", Create_grayworld, 0);

    return "grayworld";
}
