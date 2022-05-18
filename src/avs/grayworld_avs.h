#pragma once

#include <memory>

#include "avisynth.h"
#include "../common/common.h"

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
    std::pair<float, float>(*compute)(float* line_sum, int* line_count_pels, const int height) noexcept;
    void (*correct)(PVideoFrame& dst, float* tmpplab, std::pair<float, float>& avg, const int pitch, const int width, const int height) noexcept;

public:
    grayworld(PClip _child, int opt, grayworld_mode mode, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;

    int __stdcall SetCacheHints(int cachehints, int frame_range) override
    {
        return cachehints == CACHE_GET_MTMODE ? MT_MULTI_INSTANCE : 0;
    }
};
