#include "common_avx2.h"

void apply_matrix_avx2(const float matrix[3][3], const Vec8f input0, const Vec8f input1, const Vec8f input2, Vec8f& output0, Vec8f& output1, Vec8f& output2) noexcept
{
    output0 = Vec8f(matrix[0][0]) * input0 + Vec8f(matrix[0][1]) * input1 + Vec8f(matrix[0][2]) * input2;
    output1 = Vec8f(matrix[1][0]) * input0 + Vec8f(matrix[1][1]) * input1 + Vec8f(matrix[1][2]) * input2;
    output2 = Vec8f(matrix[2][0]) * input0 + Vec8f(matrix[2][1]) * input1 + Vec8f(matrix[2][2]) * input2;
}

void rgb2lab_avx2(const Vec8f r, const Vec8f g, const Vec8f b, Vec8f& l_lab, Vec8f& a_lab, Vec8f& b_lab) noexcept
{
    Vec8f l_lms;
    Vec8f m_lms;
    Vec8f s_lms;

    apply_matrix_avx2(rgb2lms, r, g, b, l_lms, m_lms, s_lms);

    const auto zero{ zero_8f() };
    const auto c{ Vec8f(-1024.0f) };

    l_lms = select(l_lms > zero, log(l_lms), c);
    m_lms = select(m_lms > zero, log(m_lms), c);
    s_lms = select(s_lms > zero, log(s_lms), c);

    apply_matrix_avx2(lms2lab, l_lms, m_lms, s_lms, l_lab, a_lab, b_lab);
}

void lab2rgb_avx2(const Vec8f l_lab, const Vec8f a_lab, const Vec8f b_lab, Vec8f& r, Vec8f& g, Vec8f& b) noexcept
{
    Vec8f l_lms;
    Vec8f m_lms;
    Vec8f s_lms;

    apply_matrix_avx2(lab2lms, l_lab, a_lab, b_lab, l_lms, m_lms, s_lms);

    l_lms = exp(l_lms);
    m_lms = exp(m_lms);
    s_lms = exp(s_lms);

    apply_matrix_avx2(lms2rgb, l_lms, m_lms, s_lms, r, g, b);
}
