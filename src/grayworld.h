#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "avisynth.h"

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

enum class grayworld_mode
{
    mean,
    median
};

template <grayworld_mode mode>
void apply_matrix_c(const float matrix[3][3], const float input[3], float output[3]);
void rgb2lab_c(const float rgb[3], float lab[3]);
void lab2rgb_c(const float lab[3], float rgb[3]);

template <grayworld_mode mode>
void convert_frame_sse2(float* __restrict tmpplab, PVideoFrame& src, float* line_sum, int* line_count_pels, const int pitch, const int width, const int height) noexcept;
void correct_frame_sse2(PVideoFrame& dst, float* tmpplab, std::pair<float, float>& avg, const int pitch, const int width, const int height) noexcept;

template <grayworld_mode mode>
void convert_frame_avx2(float* __restrict tmpplab, PVideoFrame& src, float* line_sum, int* line_count_pels, const int pitch, const int width, const int height) noexcept;
void correct_frame_avx2(PVideoFrame& dst, float* tmpplab, std::pair<float, float>& avg, const int pitch, const int width, const int height) noexcept;

template <grayworld_mode mode>
void convert_frame_avx512(float* __restrict tmpplab, PVideoFrame& src, float* line_sum, int* line_count_pels, const int pitch, const int width, const int height) noexcept;
void correct_frame_avx512(PVideoFrame& dst, float* tmpplab, std::pair<float, float>& avg, const int pitch, const int width, const int height) noexcept;

class grayworld : public GenericVideoFilter
{
    std::pair<float, float> avg;

    std::unique_ptr<float[]>tmpplab;
    std::unique_ptr<int[]>line_count_pels;
    std::unique_ptr<float[]>line_sum;

    void (*convert)(float* __restrict tmpplab, PVideoFrame& src, float* line_sum, int* line_count_pels, const int pitch, const int width, const int height) noexcept;
    std::pair<float, float>(*compute)(float* line_sum, int* line_count_pels, const int height);
    void (*correct)(PVideoFrame& dst, float* tmpplab, std::pair<float, float>& avg, const int pitch, const int width, const int height) noexcept;

public:
    grayworld(PClip _child, int opt, grayworld_mode mode, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;

    int __stdcall SetCacheHints(int cachehints, int frame_range) override
    {
        return cachehints == CACHE_GET_MTMODE ? MT_MULTI_INSTANCE : 0;
    }
};
