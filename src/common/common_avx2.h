#pragma once

#include "../common/common.h"
#include "../VCL2/vectorclass.h"
#include "../VCL2/vectormath_exp.h"

void rgb2lab_avx2(const Vec8f r, const Vec8f g, const Vec8f b, Vec8f& l_lab, Vec8f& a_lab, Vec8f& b_lab) noexcept;
void lab2rgb_avx2(const Vec8f l_lab, const Vec8f a_lab, const Vec8f b_lab, Vec8f& r, Vec8f& g, Vec8f& b) noexcept;
