#include "grayworld.h"
#include "VCL2/vectorclass.h"
#include "VCL2/vectormath_exp.h"

static void apply_matrix(const float matrix[3][3], const Vec8f input0, const Vec8f input1, const Vec8f input2, Vec8f& output0, Vec8f& output1, Vec8f& output2)
{
    output0 = Vec8f(matrix[0][0]) * input0 + Vec8f(matrix[0][1]) * input1 + Vec8f(matrix[0][2]) * input2;
    output1 = Vec8f(matrix[1][0]) * input0 + Vec8f(matrix[1][1]) * input1 + Vec8f(matrix[1][2]) * input2;
    output2 = Vec8f(matrix[2][0]) * input0 + Vec8f(matrix[2][1]) * input1 + Vec8f(matrix[2][2]) * input2;
}

static void rgb2lab(const Vec8f r, const Vec8f g, const Vec8f b, Vec8f& l_lab, Vec8f& a_lab, Vec8f& b_lab)
{
    Vec8f l_lms;
    Vec8f m_lms;
    Vec8f s_lms;

    apply_matrix(rgb2lms, r, g, b, l_lms, m_lms, s_lms);

    const auto zero{ zero_8f() };
    const auto c{ Vec8f(-1024.0f) };

    l_lms = select(l_lms > zero, log(l_lms), c);
    m_lms = select(m_lms > zero, log(m_lms), c);
    s_lms = select(s_lms > zero, log(s_lms), c);

    apply_matrix(lms2lab, l_lms, m_lms, s_lms, l_lab, a_lab, b_lab);
}

static void lab2rgb(const Vec8f l_lab, const Vec8f a_lab, const Vec8f b_lab, Vec8f& r, Vec8f& g, Vec8f& b)
{
    Vec8f l_lms;
    Vec8f m_lms;
    Vec8f s_lms;

    apply_matrix(lab2lms, l_lab, a_lab, b_lab, l_lms, m_lms, s_lms);

    l_lms = exp(l_lms);
    m_lms = exp(m_lms);
    s_lms = exp(s_lms);

    apply_matrix(lms2rgb, l_lms, m_lms, s_lms, r, g, b);
}

void convert_frame_avx2(float* __restrict tmpplab, PVideoFrame& src, float* line_sum, int* line_count_pels, const int pitch, const int width, const int height) noexcept
{
    const int width_mod8{ width - (width % 8) };
    const float* r{ reinterpret_cast<const float*>(src->GetReadPtr(PLANAR_R)) };
    const float* g{ reinterpret_cast<const float*>(src->GetReadPtr(PLANAR_G)) };
    const float* b{ reinterpret_cast<const float*>(src->GetReadPtr(PLANAR_B)) };

    float rgb[3];
    float lab[3];

    Vec8f r1;
    Vec8f g1;
    Vec8f b1;
    Vec8f l_lab;
    Vec8f a_lab;
    Vec8f b_lab;

    for (int y{ 0 }; y < height; ++y)
    {
        float* acur{ tmpplab + y * width + width * height };
        float* bcur{ tmpplab + y * width + 2 * width * height };
        float* lcur{ tmpplab + y * width };

        line_sum[y] = 0.0f;
        line_sum[y + height] = 0.0f;
        line_count_pels[y] = 0;

        for (int x{ 0 }; x < width_mod8; x += 8)
        {
            r1 = Vec8f().load(r + x);
            g1 = Vec8f().load(g + x);
            b1 = Vec8f().load(b + x);

            rgb2lab(r1, g1, b1, l_lab, a_lab, b_lab);

            *(lcur++) = l_lab.extract(0);
            *(lcur++) = l_lab.extract(1);
            *(lcur++) = l_lab.extract(2);
            *(lcur++) = l_lab.extract(3);
            *(lcur++) = l_lab.extract(4);
            *(lcur++) = l_lab.extract(5);
            *(lcur++) = l_lab.extract(6);
            *(lcur++) = l_lab.extract(7);

            *(acur++) = a_lab.extract(0);
            *(acur++) = a_lab.extract(1);
            *(acur++) = a_lab.extract(2);
            *(acur++) = a_lab.extract(3);
            *(acur++) = a_lab.extract(4);
            *(acur++) = a_lab.extract(5);
            *(acur++) = a_lab.extract(6);
            *(acur++) = a_lab.extract(7);

            *(bcur++) = b_lab.extract(0);
            *(bcur++) = b_lab.extract(1);
            *(bcur++) = b_lab.extract(2);
            *(bcur++) = b_lab.extract(3);
            *(bcur++) = b_lab.extract(4);
            *(bcur++) = b_lab.extract(5);
            *(bcur++) = b_lab.extract(6);
            *(bcur++) = b_lab.extract(7);

            line_sum[y] += horizontal_add(a_lab);
            line_sum[y + height] += horizontal_add(b_lab);
            line_count_pels[y] += 8;
        }

        for (int x{ width_mod8 }; x < width; ++x)
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

void correct_frame_avx2(PVideoFrame& dst, float* tmpplab, std::pair<float, float>& avg, const int pitch, const int width, const int height) noexcept
{
    const int width_mod8{ width - (width % 8) };
    float* __restrict r{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_R)) };
    float* __restrict g{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_G)) };
    float* __restrict b{ reinterpret_cast<float*>(dst->GetWritePtr(PLANAR_B)) };

    float rgb[3];
    float lab[3];

    Vec8f r1;
    Vec8f g1;
    Vec8f b1;
    Vec8f l_lab;
    Vec8f a_lab;
    Vec8f b_lab;

    for (int y{ 0 }; y < height; ++y)
    {
        float* lcur{ tmpplab + y * width };
        float* acur{ tmpplab + y * width + width * height };
        float* bcur{ tmpplab + y * width + 2 * width * height };

        for (int x{ 0 }; x < width_mod8; x += 8)
        {
            l_lab.insert(0, *lcur++);
            l_lab.insert(1, *lcur++);
            l_lab.insert(2, *lcur++);
            l_lab.insert(3, *lcur++);
            l_lab.insert(4, *lcur++);
            l_lab.insert(5, *lcur++);
            l_lab.insert(6, *lcur++);
            l_lab.insert(7, *lcur++);

            a_lab.insert(0, *acur++);
            a_lab.insert(1, *acur++);
            a_lab.insert(2, *acur++);
            a_lab.insert(3, *acur++);
            a_lab.insert(4, *acur++);
            a_lab.insert(5, *acur++);
            a_lab.insert(6, *acur++);
            a_lab.insert(7, *acur++);

            b_lab.insert(0, *bcur++);
            b_lab.insert(1, *bcur++);
            b_lab.insert(2, *bcur++);
            b_lab.insert(3, *bcur++);
            b_lab.insert(4, *bcur++);
            b_lab.insert(5, *bcur++);
            b_lab.insert(6, *bcur++);
            b_lab.insert(7, *bcur++);

            // subtract the average for the color channels
            a_lab -= Vec8f(avg.first);
            b_lab -= Vec8f(avg.second);

            //convert back to linear rgb
            lab2rgb(l_lab, a_lab, b_lab, r1, g1, b1);

            r1 = max(min(r1, Vec8f(1.0f)), Vec8f(0.0f));
            g1 = max(min(g1, Vec8f(1.0f)), Vec8f(0.0f));
            b1 = max(min(b1, Vec8f(1.0f)), Vec8f(0.0f));

            r1.store(r + x);
            g1.store(g + x);
            b1.store(b + x);
        }

        for (int x{ width_mod8 }; x < width; ++x)
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
