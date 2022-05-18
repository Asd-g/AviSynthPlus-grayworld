#include "common_avx512.h"

void apply_matrix_avx512(const float matrix[3][3], const Vec16f input0, const Vec16f input1, const Vec16f input2, Vec16f& output0, Vec16f& output1, Vec16f& output2) noexcept
{
    output0 = Vec16f(matrix[0][0]) * input0 + Vec16f(matrix[0][1]) * input1 + Vec16f(matrix[0][2]) * input2;
    output1 = Vec16f(matrix[1][0]) * input0 + Vec16f(matrix[1][1]) * input1 + Vec16f(matrix[1][2]) * input2;
    output2 = Vec16f(matrix[2][0]) * input0 + Vec16f(matrix[2][1]) * input1 + Vec16f(matrix[2][2]) * input2;
}

void rgb2lab_avx512(const Vec16f r, const Vec16f g, const Vec16f b, Vec16f& l_lab, Vec16f& a_lab, Vec16f& b_lab) noexcept
{
    Vec16f l_lms;
    Vec16f m_lms;
    Vec16f s_lms;

    apply_matrix_avx512(rgb2lms, r, g, b, l_lms, m_lms, s_lms);

    const Vec16f zero{ Vec16f(0.0f) };
    const Vec16f c{ Vec16f(-1024.0f) };

    l_lms = select(l_lms > zero, log(l_lms), c);
    m_lms = select(m_lms > zero, log(m_lms), c);
    s_lms = select(s_lms > zero, log(s_lms), c);

    apply_matrix_avx512(lms2lab, l_lms, m_lms, s_lms, l_lab, a_lab, b_lab);
}

void lab2rgb_avx512(const Vec16f l_lab, const Vec16f a_lab, const Vec16f b_lab, Vec16f& r, Vec16f& g, Vec16f& b) noexcept
{
    Vec16f l_lms;
    Vec16f m_lms;
    Vec16f s_lms;

    apply_matrix_avx512(lab2lms, l_lab, a_lab, b_lab, l_lms, m_lms, s_lms);

    l_lms = exp(l_lms);
    m_lms = exp(m_lms);
    s_lms = exp(s_lms);

    apply_matrix_avx512(lms2rgb, l_lms, m_lms, s_lms, r, g, b);
}
