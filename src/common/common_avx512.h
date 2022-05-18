#pragma once

#include "../common/common.h"
#include "../VCL2/vectorclass.h"
#include "../VCL2/vectormath_exp.h"

void rgb2lab_avx512(const Vec16f r, const Vec16f g, const Vec16f b, Vec16f& l_lab, Vec16f& a_lab, Vec16f& b_lab) noexcept;
void lab2rgb_avx512(const Vec16f l_lab, const Vec16f a_lab, const Vec16f b_lab, Vec16f& r, Vec16f& g, Vec16f& b) noexcept;
