#include "grayworld.h"
#include "VCL2/vectorclass.h"
#include "VCL2/vectormath_exp.h"

static void apply_matrix(const float matrix[3][3], const Vec16f input0, const Vec16f input1, const Vec16f input2, Vec16f& output0, Vec16f& output1, Vec16f& output2)
{
    output0 = Vec16f(matrix[0][0]) * input0 + Vec16f(matrix[0][1]) * input1 + Vec16f(matrix[0][2]) * input2;
    output1 = Vec16f(matrix[1][0]) * input0 + Vec16f(matrix[1][1]) * input1 + Vec16f(matrix[1][2]) * input2;
    output2 = Vec16f(matrix[2][0]) * input0 + Vec16f(matrix[2][1]) * input1 + Vec16f(matrix[2][2]) * input2;
}

static void rgb2lab(const Vec16f r, const Vec16f g, const Vec16f b, Vec16f& l_lab, Vec16f& a_lab, Vec16f& b_lab)
{
    Vec16f l_lms;
    Vec16f m_lms;
    Vec16f s_lms;

    apply_matrix(rgb2lms, r, g, b, l_lms, m_lms, s_lms);

    const Vec16f zero{ Vec16f(0.0f) };
    const Vec16f c{ Vec16f(-1024.0f) };

    l_lms = select(l_lms > zero, log(l_lms), c);
    m_lms = select(m_lms > zero, log(m_lms), c);
    s_lms = select(s_lms > zero, log(s_lms), c);

    apply_matrix(lms2lab, l_lms, m_lms, s_lms, l_lab, a_lab, b_lab);
}

static void lab2rgb(const Vec16f l_lab, const Vec16f a_lab, const Vec16f b_lab, Vec16f& r, Vec16f& g, Vec16f& b)
{
    Vec16f l_lms;
    Vec16f m_lms;
    Vec16f s_lms;

    apply_matrix(lab2lms, l_lab, a_lab, b_lab, l_lms, m_lms, s_lms);

    l_lms = exp(l_lms);
    m_lms = exp(m_lms);
    s_lms = exp(s_lms);

    apply_matrix(lms2rgb, l_lms, m_lms, s_lms, r, g, b);
}

template <grayworld_mode mode>
void convert_frame_avx512(float* __restrict tmpplab, PVideoFrame& src, float* line_sum, int* line_count_pels, const int pitch, const int width, const int height) noexcept
{
    const int width_mod16{ width - (width % 16) };
    const float* r{ reinterpret_cast<const float*>(src->GetReadPtr(PLANAR_R)) };
    const float* g{ reinterpret_cast<const float*>(src->GetReadPtr(PLANAR_G)) };
    const float* b{ reinterpret_cast<const float*>(src->GetReadPtr(PLANAR_B)) };

    float rgb[3];
    float lab[3];

    Vec16f r1;
    Vec16f g1;
    Vec16f b1;
    Vec16f l_lab;
    Vec16f a_lab;
    Vec16f b_lab;

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

            for (int x{ 0 }; x < width_mod16; x += 16)
            {
                r1 = Vec16f().load(r + x);
                g1 = Vec16f().load(g + x);
                b1 = Vec16f().load(b + x);

                rgb2lab(r1, g1, b1, l_lab, a_lab, b_lab);

                for (int i{ 0 }; i < 16; ++i)
                {
                    *(lcur++) = l_lab.extract(i);
                    *(acur++) = a_lab.extract(i);
                    *(bcur++) = b_lab.extract(i);
                }

                line_sum[y] += horizontal_add(a_lab);
                line_sum[y + height] += horizontal_add(b_lab);
                line_count_pels[y] += 16;
            }

            for (int x{ width_mod16 }; x < width; ++x)
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

            for (int x{ 0 }; x < width_mod16; x += 16)
            {
                r1 = Vec16f().load(r + x);
                g1 = Vec16f().load(g + x);
                b1 = Vec16f().load(b + x);

                rgb2lab(r1, g1, b1, l_lab, a_lab, b_lab);

                for (int i{ 0 }; i < 16; ++i)
                {
                    *(lcur++) = l_lab.extract(i);
                    *(acur++) = a_lab.extract(i);
                    *(bcur++) = b_lab.extract(i);
                }

                a_lab.store(&m0[x]);
                b_lab.store(&m1[x]);
            }

            for (int x{ width_mod16 }; x < width; ++x)
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

template void convert_frame_avx512<grayworld_mode::mean>(float* __restrict tmpplab, PVideoFrame& src, float* line_sum, int* line_count_pels, const int pitch, const int width, const int height) noexcept;
template void convert_frame_avx512<grayworld_mode::median>(float* __restrict tmpplab, PVideoFrame& src, float* line_sum, int* line_count_pels, const int pitch, const int width, const int height) noexcept;

void correct_frame_avx512(PVideoFrame& dst, float* tmpplab, std::pair<float, float>& avg, const int pitch, const int width, const int height) noexcept
{
    const int width_mod16{ width - (width % 16) };
    float* __restrict r{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_R)) };
    float* __restrict g{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_G)) };
    float* __restrict b{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_B)) };

    float rgb[3];
    float lab[3];

    Vec16f r1;
    Vec16f g1;
    Vec16f b1;
    Vec16f l_lab;
    Vec16f a_lab;
    Vec16f b_lab;

    for (int y{ 0 }; y < height; ++y)
    {
        float* lcur{ tmpplab + y * width };
        float* acur{ tmpplab + y * width + width * height };
        float* bcur{ tmpplab + y * width + 2 * width * height };

        for (int x{ 0 }; x < width_mod16; x += 16)
        {
            for (int i{ 0 }; i < 16; ++i)
            {
                l_lab.insert(i, *lcur++);
                a_lab.insert(i, *acur++);
                b_lab.insert(i, *bcur++);
            }

            // subtract the average for the color channels
            a_lab -= Vec16f(avg.first);
            b_lab -= Vec16f(avg.second);

            //convert back to linear rgb
            lab2rgb(l_lab, a_lab, b_lab, r1, g1, b1);

            r1 = max(min(r1, Vec16f(1.0f)), Vec16f(0.0f));
            g1 = max(min(g1, Vec16f(1.0f)), Vec16f(0.0f));
            b1 = max(min(b1, Vec16f(1.0f)), Vec16f(0.0f));

            r1.store(r + x);
            g1.store(g + x);
            b1.store(b + x);
        }

        for (int x{ width_mod16 }; x < width; ++x)
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
