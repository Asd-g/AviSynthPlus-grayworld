#include <cmath>

#include "common.h"

void apply_matrix_c(const float matrix[3][3], const float input[3], float output[3]) noexcept
{
    output[0] = matrix[0][0] * input[0] + matrix[0][1] * input[1] + matrix[0][2] * input[2];
    output[1] = matrix[1][0] * input[0] + matrix[1][1] * input[1] + matrix[1][2] * input[2];
    output[2] = matrix[2][0] * input[0] + matrix[2][1] * input[1] + matrix[2][2] * input[2];
}

void rgb2lab_c(const float rgb[3], float lab[3]) noexcept
{
    float lms[3];

    apply_matrix_c(rgb2lms, rgb, lms);

    lms[0] = (lms[0] > 0.0f) ? logf(lms[0]) : -1024.0f;
    lms[1] = (lms[1] > 0.0f) ? logf(lms[1]) : -1024.0f;
    lms[2] = (lms[2] > 0.0f) ? logf(lms[2]) : -1024.0f;

    apply_matrix_c(lms2lab, lms, lab);
}

void lab2rgb_c(const float lab[3], float rgb[3]) noexcept
{
    float lms[3];

    apply_matrix_c(lab2lms, lab, lms);

    lms[0] = expf(lms[0]);
    lms[1] = expf(lms[1]);
    lms[2] = expf(lms[2]);

    apply_matrix_c(lms2rgb, lms, rgb);
}

template <grayworld_mode mode>
std::pair<float, float> compute_correction(float* line_sum, int* line_count_pels, const int height) noexcept
{
    if constexpr (mode == grayworld_mode::mean)
    {
        float asum{ 0.0f };
        float bsum{ 0.0f };
        int pixels{ 0 };

        for (int y{ 0 }; y < height; ++y)
        {
            asum += line_sum[y];
            bsum += line_sum[y + height];
            pixels += line_count_pels[y];
        }

        return std::make_pair<float, float>(asum / pixels, bsum / pixels);
    }
    else
    {
        std::vector<float> am;
        am.reserve(height);
        std::vector<float> bm;
        bm.reserve(height);

        for (int y{ 0 }; y < height; ++y)
        {
            am.emplace_back(line_sum[y]);
            bm.emplace_back(line_sum[y + height]);
        }

        const auto middleItr{ am.begin() + am.size() / 2 };
        std::nth_element(am.begin(), middleItr, am.end());

        const auto middleItr1{ bm.begin() + bm.size() / 2 };
        std::nth_element(bm.begin(), middleItr1, bm.end());

        return std::make_pair<float, float>((am.size() % 2 == 0) ? ((*(std::max_element(am.begin(), middleItr)) + *middleItr) / 2) : *middleItr,
            (bm.size() % 2 == 0) ? ((*(std::max_element(bm.begin(), middleItr1)) + *middleItr1) / 2) : *middleItr1);
    }
}

template std::pair<float, float> compute_correction<grayworld_mode::mean>(float* line_sum, int* line_count_pels, const int height) noexcept;
template std::pair<float, float> compute_correction<grayworld_mode::median>(float* line_sum, int* line_count_pels, const int height) noexcept;
