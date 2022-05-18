#pragma once

#include "../common/common.h"
#include "../VCL2/vectorclass.h"
#include "../VCL2/vectormath_exp.h"

void rgb2lab_sse2(const Vec4f r, const Vec4f g, const Vec4f b, Vec4f& l_lab, Vec4f& a_lab, Vec4f& b_lab) noexcept;
void lab2rgb_sse2(const Vec4f l_lab, const Vec4f a_lab, const Vec4f b_lab, Vec4f& r, Vec4f& g, Vec4f& b) noexcept;
