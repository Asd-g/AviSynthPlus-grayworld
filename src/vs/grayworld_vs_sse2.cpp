#include "grayworld_vs.h"
#include "../common/common_sse2.h"

template <grayworld_mode mode>
void convert_frame_sse2(float* __restrict tmpplab, const VSFrame* src, float* line_sum, int* line_count_pels, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept
{
    const int width_mod4{ width - (width % 4) };
    const float* r{ reinterpret_cast<const float*>(vsapi->getReadPtr(src, 0)) };
    const float* g{ reinterpret_cast<const float*>(vsapi->getReadPtr(src, 1)) };
    const float* b{ reinterpret_cast<const float*>(vsapi->getReadPtr(src, 2)) };

    float rgb[3];
    float lab[3];

    Vec4f r1;
    Vec4f g1;
    Vec4f b1;
    Vec4f l_lab;
    Vec4f a_lab;
    Vec4f b_lab;

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

            for (int x{ 0 }; x < width_mod4; x += 4)
            {
                r1 = Vec4f().load(r + x);
                g1 = Vec4f().load(g + x);
                b1 = Vec4f().load(b + x);

                rgb2lab_sse2(r1, g1, b1, l_lab, a_lab, b_lab);

                for (int i{ 0 }; i < 4; ++i)
                {
                    *(lcur++) = l_lab.extract(i);
                    *(acur++) = a_lab.extract(i);
                    *(bcur++) = b_lab.extract(i);
                }

                line_sum[y] += horizontal_add(a_lab);
                line_sum[y + height] += horizontal_add(b_lab);
                line_count_pels[y] += 4;
            }

            for (int x{ width_mod4 }; x < width; ++x)
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
            std::vector<float> m0(width);
            std::vector<float> m1(width);

            for (int x{ 0 }; x < width_mod4; x += 4)
            {
                r1 = Vec4f().load(r + x);
                g1 = Vec4f().load(g + x);
                b1 = Vec4f().load(b + x);

                rgb2lab_sse2(r1, g1, b1, l_lab, a_lab, b_lab);

                for (int i{ 0 }; i < 4; ++i)
                {
                    *(lcur++) = l_lab.extract(i);
                    *(acur++) = a_lab.extract(i);
                    *(bcur++) = b_lab.extract(i);
                }

                a_lab.store(&m0[x]);
                b_lab.store(&m1[x]);
            }

            for (int x{ width_mod4 }; x < width; ++x)
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

template void convert_frame_sse2<grayworld_mode::mean>(float* __restrict tmpplab, const VSFrame* src, float* line_sum, int* line_count_pels, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept;
template void convert_frame_sse2<grayworld_mode::median>(float* __restrict tmpplab, const VSFrame* src, float* line_sum, int* line_count_pels, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept;

void correct_frame_sse2(VSFrame* dst, float* tmpplab, std::pair<float, float>& avg, const ptrdiff_t pitch, const int width, const int height, const VSAPI* vsapi) noexcept
{
    const int width_mod4{ width - (width % 4) };
    float* __restrict r{ reinterpret_cast<float*>(vsapi->getWritePtr(dst, 0)) };
    float* __restrict g{ reinterpret_cast<float*>(vsapi->getWritePtr(dst, 1)) };
    float* __restrict b{ reinterpret_cast<float*>(vsapi->getWritePtr(dst, 2)) };

    float rgb[3];
    float lab[3];

    Vec4f r1;
    Vec4f g1;
    Vec4f b1;
    Vec4f l_lab;
    Vec4f a_lab;
    Vec4f b_lab;

    for (int y{ 0 }; y < height; ++y)
    {
        float* lcur{ tmpplab + y * width };
        float* acur{ tmpplab + y * width + width * height };
        float* bcur{ tmpplab + y * width + 2 * width * height };

        for (int x{ 0 }; x < width_mod4; x += 4)
        {
            for (int i{ 0 }; i < 4; ++i)
            {
                l_lab.insert(i, *lcur++);
                a_lab.insert(i, *acur++);
                b_lab.insert(i, *bcur++);
            }

            // subtract the average for the color channels
            a_lab -= Vec4f(avg.first);
            b_lab -= Vec4f(avg.second);

            //convert back to linear rgb
            lab2rgb_sse2(l_lab, a_lab, b_lab, r1, g1, b1);

            r1 = max(min(r1, Vec4f(1.0f)), Vec4f(0.0f));
            g1 = max(min(g1, Vec4f(1.0f)), Vec4f(0.0f));
            b1 = max(min(b1, Vec4f(1.0f)), Vec4f(0.0f));

            r1.store(r + x);
            g1.store(g + x);
            b1.store(b + x);
        }

        for (int x{ width_mod4 }; x < width; ++x)
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
