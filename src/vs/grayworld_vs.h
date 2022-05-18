#pragma once

#include <memory>

#include "../common/common.h"
#include "VapourSynth4.h"
#include "VSHelper4.h"

template <grayworld_mode mode>
void convert_frame_sse2(float* __restrict tmpplab, const VSFrame* src, float* line_sum, int* line_count_pels, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept;
void correct_frame_sse2(VSFrame* dst, float* tmpplab, std::pair<float, float>& avg, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept;

template <grayworld_mode mode>
void convert_frame_avx2(float* __restrict tmpplab, const VSFrame* src, float* line_sum, int* line_count_pels, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept;
void correct_frame_avx2(VSFrame* dst, float* tmpplab, std::pair<float, float>& avg, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept;

template <grayworld_mode mode>
void convert_frame_avx512(float* __restrict tmpplab, const VSFrame* src, float* line_sum, int* line_count_pels, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept;
void correct_frame_avx512(VSFrame* dst, float* tmpplab, std::pair<float, float>& avg, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept;

struct grayworldData
{
    VSNode* node;
    const VSVideoInfo* vi;

    std::pair<float, float> avg;

    std::unique_ptr<float[]>tmpplab;
    std::unique_ptr<int[]>line_count_pels;
    std::unique_ptr<float[]>line_sum;

    void (*convert)(float* __restrict tmpplab, const VSFrame* src, float* line_sum, int* line_count_pels, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept;
    std::pair<float, float>(*compute)(float* line_sum, int* line_count_pels, const int height) noexcept;
    void (*correct)(VSFrame* dst, float* tmpplab, std::pair<float, float>& avg, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept;
};
