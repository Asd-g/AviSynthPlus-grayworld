#pragma once

#include <algorithm>
#include <vector>

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

void rgb2lab_c(const float rgb[3], float lab[3]) noexcept;
void lab2rgb_c(const float lab[3], float rgb[3]) noexcept;

template <grayworld_mode mode>
std::pair<float, float> compute_correction(float* line_sum, int* line_count_pels, const int height) noexcept;
