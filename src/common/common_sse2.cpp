#include "common_sse2.h"

void apply_matrix_sse2(const float matrix[3][3], const Vec4f input0, const Vec4f input1, const Vec4f input2, Vec4f& output0, Vec4f& output1, Vec4f& output2) noexcept
{
    output0 = Vec4f(matrix[0][0]) * input0 + Vec4f(matrix[0][1]) * input1 + Vec4f(matrix[0][2]) * input2;
    output1 = Vec4f(matrix[1][0]) * input0 + Vec4f(matrix[1][1]) * input1 + Vec4f(matrix[1][2]) * input2;
    output2 = Vec4f(matrix[2][0]) * input0 + Vec4f(matrix[2][1]) * input1 + Vec4f(matrix[2][2]) * input2;
}

void rgb2lab_sse2(const Vec4f r, const Vec4f g, const Vec4f b, Vec4f& l_lab, Vec4f& a_lab, Vec4f& b_lab) noexcept
{
    Vec4f l_lms;
    Vec4f m_lms;
    Vec4f s_lms;

    apply_matrix_sse2(rgb2lms, r, g, b, l_lms, m_lms, s_lms);

    const auto zero{ zero_4f() };
    const auto c{ Vec4f(-1024.0f) };

    l_lms = select(l_lms > zero, log(l_lms), c);
    m_lms = select(m_lms > zero, log(m_lms), c);
    s_lms = select(s_lms > zero, log(s_lms), c);

    apply_matrix_sse2(lms2lab, l_lms, m_lms, s_lms, l_lab, a_lab, b_lab);
}

void lab2rgb_sse2(const Vec4f l_lab, const Vec4f a_lab, const Vec4f b_lab, Vec4f& r, Vec4f& g, Vec4f& b) noexcept
{
    Vec4f l_lms;
    Vec4f m_lms;
    Vec4f s_lms;

    apply_matrix_sse2(lab2lms, l_lab, a_lab, b_lab, l_lms, m_lms, s_lms);

    l_lms = exp(l_lms);
    m_lms = exp(m_lms);
    s_lms = exp(s_lms);

    apply_matrix_sse2(lms2rgb, l_lms, m_lms, s_lms, r, g, b);
}
