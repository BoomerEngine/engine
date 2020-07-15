/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: simd\#]
***/

#pragma once

namespace base
{
    //--

#ifdef PLATFORM_SSE

    INLINE SIMDQuad::SIMDQuad()
    {
        quad = _mm_setzero_ps();
    }

    INLINE SIMDQuad::SIMDQuad(float x, float y, float z, float w)
    {
        quad = _mm_setr_ps(x, y, z, w);
    }

    INLINE SIMDQuad::SIMDQuad(uint32_t x, uint32_t y, uint32_t z, uint32_t w)
    {
        quadi = _mm_setr_epi32(x,y,z,w);
    }

    INLINE SIMDQuad::SIMDQuad(float s)
    {
        quad = _mm_set1_ps(s);
    }

    INLINE SIMDQuad::SIMDQuad(const float* ptr)
    {
        quad = _mm_loadu_ps(ptr);
    }

    INLINE SIMDQuad::SIMDQuad(const float* ptr, uint8_t alignment)
    {
        quad = _mm_load_ps(ptr);
    }

    INLINE SIMDQuad::SIMDQuad(const SIMDQuad& xyzPart, const SIMDQuad& wPart)
    {
        __m128 tmp = _mm_unpackhi_ps(xyzPart, wPart);
        quad = _mm_shuffle_ps(xyzPart, tmp, _MM_SHUFFLE(3, 0, 1, 0));
    }

    INLINE SIMDQuad::SIMDQuad(const SIMDQuad& x, const SIMDQuad& y, const SIMDQuad& z, const SIMDQuad& w)
    {
        __m128 p0 = _mm_unpacklo_ps(x, y); // x y x y
        __m128 p1 = _mm_unpackhi_ps(z, w); // z w z w
        quad = _mm_shuffle_ps(p0, p1, _MM_SHUFFLE(3, 2, 1, 0)); // w z y x
    }

    INLINE SIMDQuad::SIMDQuad(const SIMDQuad& src)
    {
        quad = src.quad;
    }

    INLINE SIMDQuad::SIMDQuad(const SIMDQuad& src, uint8_t component)
    {
        if (component == 0)
        {
            quad = _mm_shuffle_ps(src.quad, src.quad, _MM_SHUFFLE(0, 0, 0, 0));
        }
        else if (component == 1)
        {
            quad = _mm_shuffle_ps(src.quad, src.quad, _MM_SHUFFLE(1, 1, 1, 1));
        }
        else if (component == 2)
        {
            quad = _mm_shuffle_ps(src.quad, src.quad, _MM_SHUFFLE(2, 2, 2, 2));
        }
        else if (component == 3)
        {
            quad = _mm_shuffle_ps(src.quad, src.quad, _MM_SHUFFLE(3, 3, 3, 3));
        }
    }

    INLINE SIMDQuad SIMDQuad::x() const
    {
        return _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(0, 0, 0, 0));
    }

    INLINE SIMDQuad SIMDQuad::y() const
    {
        return _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(1, 1, 1, 1));
    }

    INLINE SIMDQuad SIMDQuad::z() const
    {
        return _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(2, 2, 2, 2));
    }

    INLINE SIMDQuad SIMDQuad::w() const
    {
        return _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(3, 3, 3, 3));
    }

    INLINE SIMDQuad SIMDQuad::xyxy() const
    {
        return _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(1, 0, 1, 0));
    }

    INLINE SIMDQuad SIMDQuad::xy00() const
    {
        __m128 zero = _mm_setzero_ps();
        return _mm_shuffle_ps(zero, quad, _MM_SHUFFLE(0, 0, 1, 0));
    }

    INLINE SIMDQuad SIMDQuad::zwzw() const
    {
        return _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(3, 2, 3, 2));
    }

    INLINE SIMDQuad SIMDQuad::xxyy() const
    {
        return _mm_unpacklo_ps(quad, quad);
    }

    INLINE SIMDQuad SIMDQuad::zzww() const
    {
        return _mm_unpackhi_ps(quad, quad);
    }

    INLINE SIMDQuad SIMDQuad::_x000() const
    {
        auto p0 = _mm_setzero_ps();
        auto p1 = _mm_unpacklo_ps(quad, p0); // x 0 y 0
        return _mm_shuffle_ps(p1, p0, _MM_SHUFFLE_REV(0, 1, 2, 3)); // x 0 0 0
    }

    INLINE SIMDQuad SIMDQuad::_0y00() const
    {
        auto p0 = _mm_setzero_ps();
        auto p1 = _mm_unpacklo_ps(quad, p0); // x 0 y 0
        return _mm_shuffle_ps(p1, p0, _MM_SHUFFLE_REV(1, 2, 2, 3)); // 0 y 0 0
    }

    INLINE SIMDQuad SIMDQuad::_00z0() const
    {
        auto p0 = _mm_setzero_ps();
        auto p1 = _mm_unpackhi_ps(quad, p0); // z 0 w 0
        return _mm_shuffle_ps(p0, p1, _MM_SHUFFLE_REV(0, 0, 0, 1)); // 0 0 z 0
    }

    INLINE SIMDQuad SIMDQuad::_000w() const
    {
        auto p0 = _mm_setzero_ps();
        auto p1 = _mm_unpackhi_ps(quad, p0); // z 0 w 0
        return _mm_shuffle_ps(p0, p1, _MM_SHUFFLE_REV(0, 0, 1, 2)); // 0 0 0 w
    }

    INLINE SIMDQuad SIMDQuad::_xy00() const
    {
        return _mm_shuffle_ps(quad, _mm_setzero_ps(), _MM_SHUFFLE_REV(0, 1, 0, 0)); // x y 0 0
    }

    INLINE SIMDQuad SIMDQuad::_00xy() const
    {
        return _mm_shuffle_ps(_mm_setzero_ps(), quad, _MM_SHUFFLE_REV(0, 0, 0, 1)); // 0 0 x y
    }

    INLINE SIMDQuad SIMDQuad::_00zw() const
    {
        return _mm_shuffle_ps(_mm_setzero_ps(), quad, _MM_SHUFFLE_REV(0, 0, 2, 3)); // 0 0 z w
    }

    INLINE SIMDQuad SIMDQuad::_zw00() const
    {
        return _mm_shuffle_ps(quad, _mm_setzero_ps(), _MM_SHUFFLE_REV(2, 3, 0, 0)); // z w 0 0
    }

    INLINE SIMDQuad SIMDQuad::operator-() const
    {
        return _mm_sub_ps(_mm_setzero_ps(), quad);
    }

    INLINE SIMDQuad SIMDQuad::operator~() const
    {
        return _mm_xor_ps(quad, FullOnesBitMask[0]);
    }

    INLINE SIMDQuad SIMDQuad::operator*(const SIMDQuad& a) const
    {
        return _mm_mul_ps(quad, a);
    }

    INLINE SIMDQuad SIMDQuad::operator/(const SIMDQuad& a) const
    {
        return _mm_div_ps(quad, a);
    }

    INLINE SIMDQuad SIMDQuad::operator+(const SIMDQuad& a) const
    {
        return _mm_add_ps(quad, a);
    }

    INLINE SIMDQuad SIMDQuad::operator-(const SIMDQuad& a) const
    {
        return _mm_sub_ps(quad, a);
    }

    INLINE SIMDQuad SIMDQuad::operator*(float a) const
    {
        return _mm_mul_ps(quad, _mm_set1_ps(a));
    }

    INLINE SIMDQuad SIMDQuad::operator/(float a) const
    {
        return _mm_div_ps(quad, _mm_set1_ps(a));
    }

    INLINE SIMDQuad SIMDQuad::operator+(float a) const
    {
        return _mm_add_ps(quad, _mm_set1_ps(a));
    }

    INLINE SIMDQuad SIMDQuad::operator-(float a) const
    {
        return _mm_sub_ps(quad, _mm_set1_ps(a));
    }

    INLINE SIMDQuad& SIMDQuad::operator*=(const SIMDQuad& a)
    {
        quad = _mm_mul_ps(quad, a);
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator/=(const SIMDQuad& a)
    {
        quad = _mm_div_ps(quad, a);
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator+=(const SIMDQuad& a)
    {
        quad = _mm_add_ps(quad, a);
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator-=(const SIMDQuad& a)
    {
        quad = _mm_sub_ps(quad, a);
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator*=(float a)
    {
        quad = _mm_mul_ps(quad, _mm_set1_ps(a));
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator/=(float a)
    {
        quad = _mm_div_ps(quad, _mm_set1_ps(a));
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator+=(float a)
    {
        quad = _mm_add_ps(quad, _mm_set1_ps(a));
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator-=(float a)
    {
        quad = _mm_sub_ps(quad, _mm_set1_ps(a));
        return *this;
    }

    INLINE SIMDQuad SIMDQuad::operator|(const SIMDQuad& a) const
    {
        return _mm_or_ps(quad, a);
    }

    INLINE SIMDQuad SIMDQuad::operator&(const SIMDQuad& a) const
    {
        return _mm_and_ps(quad, a);
    }

    INLINE SIMDQuad SIMDQuad::operator^(const SIMDQuad& a) const
    {
        return _mm_xor_ps(quad, a);
    }

    INLINE SIMDQuad SIMDQuad::operator|(const uint32_t a) const
    {
        return _mm_or_si128(quadi, _mm_set1_epi32(a));
    }

    INLINE SIMDQuad SIMDQuad::operator&(const uint32_t a) const
    {
        return _mm_and_si128(quadi, _mm_set1_epi32(a));
    }

    INLINE SIMDQuad SIMDQuad::operator^(const uint32_t a) const
    {
        return _mm_xor_si128(quadi, _mm_set1_epi32(a));
    }

    INLINE SIMDQuad& SIMDQuad::operator|=(const SIMDQuad& a)
    {
        quad = _mm_or_ps(quad, a);
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator&=(const SIMDQuad& a)
    {
        quad = _mm_and_ps(quad, a);
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator^=(const SIMDQuad& a)
    {
        quad = _mm_xor_ps(quad, a);
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator|=(const uint32_t a)
    {
        quadi = _mm_or_si128(quadi, _mm_set1_epi32(a));
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator&=(const uint32_t a)
    {
        quadi = _mm_and_si128(quadi, _mm_set1_epi32(a));
        return *this;
    }

    INLINE SIMDQuad& SIMDQuad::operator^=(const uint32_t a)
    {
        quadi = _mm_xor_si128(quadi, _mm_set1_epi32(a));
        return *this;
    }

    INLINE void SIMDQuad::storeUnaligned(void* ptr) const
    {
        _mm_storeu_ps((float*)ptr, quad);
    }

    INLINE void SIMDQuad::storeAligned(void* ptr) const
    {
        _mm_store_ps((float*)ptr, quad);
    }

    INLINE void SIMDQuad::storeAlignedBypassCache(void* ptr) const
    {
        _mm_stream_ps((float*)ptr, quad);
    }

    INLINE SIMDQuad SIMDQuad::replicate(int componentIndex) const
    {
        switch (componentIndex)
        {
            case 0: return SIMDQuad(_mm_shuffle_ps(quad, quad, _MM_SHUFFLE_REV(0, 0, 0, 0)));
            case 1: return SIMDQuad(_mm_shuffle_ps(quad, quad, _MM_SHUFFLE_REV(1, 1, 1, 1)));
            case 2: return SIMDQuad(_mm_shuffle_ps(quad, quad, _MM_SHUFFLE_REV(2, 2, 2, 2)));
            case 3: return SIMDQuad(_mm_shuffle_ps(quad, quad, _MM_SHUFFLE_REV(3, 3, 3, 3)));
        }

        return SIMDQuad();
    }

    template<>
    INLINE SIMDQuad SIMDQuad::replicate<0>() const
    {
        return SIMDQuad(_mm_shuffle_ps(quad, quad, _MM_SHUFFLE_REV(0,0,0,0)));
    }

    template<>
    INLINE SIMDQuad SIMDQuad::replicate<1>() const
    {
        return SIMDQuad(_mm_shuffle_ps(quad, quad, _MM_SHUFFLE_REV(1, 1, 1, 1)));
    }

    template<>
    INLINE SIMDQuad SIMDQuad::replicate<2>() const
    {
        return SIMDQuad(_mm_shuffle_ps(quad, quad, _MM_SHUFFLE_REV(2, 2, 2, 2)));
    }

    template<>
    INLINE SIMDQuad SIMDQuad::replicate<3>() const
    {
        return SIMDQuad(_mm_shuffle_ps(quad, quad, _MM_SHUFFLE_REV(3, 3, 3, 3)));
    }

    INLINE SIMDQuad SIMDQuad::product2() const
    {
        auto ret = _mm_mul_ps(quad, _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(1, 1, 1, 1))); // ret = x * y
        return SIMDQuad(SIMDQuad(_mm_shuffle_ps(ret, ret, _MM_SHUFFLE(0, 0, 0, 0)))); // to scalar
    }

    INLINE SIMDQuad SIMDQuad::product3() const
    {
        auto ret = _mm_mul_ps(quad, _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(1, 1, 1, 1))); // ret = x * y
        ret = _mm_mul_ps(ret, _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(2, 2, 2, 2))); // ret = x * y * z
        return SIMDQuad(SIMDQuad(_mm_shuffle_ps(ret, ret, _MM_SHUFFLE(0, 0, 0, 0)))); // to scalar
    }

    INLINE SIMDQuad SIMDQuad::product4() const
    {
        auto ret = _mm_mul_ps(quad, _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(1, 1, 1, 1))); // ret = x * y
        ret = _mm_mul_ps(ret, _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(2, 2, 2, 2))); // ret = x * y * z
        ret = _mm_mul_ps(ret, _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(3, 3, 3, 3))); // ret = x * y * z * w
        return SIMDQuad(SIMDQuad(_mm_shuffle_ps(ret, ret, _MM_SHUFFLE(0, 0, 0, 0)))); // to scalar
    }

    INLINE SIMDQuad SIMDQuad::sqrt() const
    {
        return _mm_sqrt_ps(quad);
    }

    INLINE SIMDQuad SIMDQuad::rsqrt() const
    {
        return _mm_rsqrt_ps(quad);
    }

    INLINE SIMDQuad SIMDQuad::rcp() const
    {
        return _mm_rcp_ps(quad);
    }

    INLINE SIMDQuad SIMDQuad::round() const
    {
        return SIMDQuad(std::round((*this)[0]), std::round((*this)[1]), std::round((*this)[2]), std::round((*this)[3]));
    }

    INLINE SIMDQuad SIMDQuad::round_SSE41() const
    {
#ifdef PLATFORM_SSE41
       return SIMDQuad(_mm_round_ps(quad, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
#else
       return round();
#endif
    }

    INLINE SIMDQuad SIMDQuad::trunc() const
    {
        return SIMDQuad(std::trunc((*this)[0]), std::trunc((*this)[1]), std::trunc((*this)[2]), std::trunc((*this)[3]));
    }

    INLINE SIMDQuad SIMDQuad::trunc_SSE41() const
    {
#ifdef PLATFORM_SSE41
        return _mm_round_ps(quad, _MM_FROUND_TO_ZERO |_MM_FROUND_NO_EXC);
#else
        return trunc();
#endif
    }

    INLINE SIMDQuad SIMDQuad::ceil() const
    {
        return SIMDQuad(std::ceil((*this)[0]), std::ceil((*this)[1]), std::ceil((*this)[2]), std::ceil((*this)[3]));
    }

    INLINE SIMDQuad SIMDQuad::ceil_SSE41() const
    {
#ifdef PLATFORM_SSE41
        return _mm_round_ps(quad, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
#else
        return ceil();
#endif
    }

    INLINE SIMDQuad SIMDQuad::floor() const
    {
        return SIMDQuad(std::floor((*this)[0]), std::floor((*this)[1]), std::floor((*this)[2]), std::floor((*this)[3]));
    }

    INLINE SIMDQuad SIMDQuad::floor_SSE41() const
    {
#ifdef PLATFORM_SSE41
        return _mm_round_ps(quad, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
#else
        return floor();
#endif
    }

    INLINE SIMDQuad SIMDQuad::frac() const
    {
        return SIMDQuad((*this)[0] - std::trunc((*this)[0]),
                        (*this)[1] - std::trunc((*this)[1]),
                        (*this)[2] - std::trunc((*this)[2]),
                        (*this)[3] - std::trunc((*this)[3]));
    }

    INLINE SIMDQuad SIMDQuad::frac_SSE41() const
    {
#ifdef PLATFORM_SSE41
        auto p0 = _mm_round_ps(quad, _MM_FROUND_TO_ZERO |_MM_FROUND_NO_EXC);
        return _mm_sub_ps(quad, p0);
#else
        return frac();
#endif
    }

    INLINE SIMDQuad SIMDQuad::dot2(const SIMDQuad& a) const
    {
        return SIMDQuad(_mm_mul_ps(quad, a.quad)).sum2();
    }

    INLINE SIMDQuad SIMDQuad::dot2_SSE41(const base::SIMDQuad &b) const
    {
#ifdef PLATFORM_SSE41
        return _mm_dp_ps(quad, b.quad, 0x3F /* sum XY, write to all */);
#else
        return SIMDQuad(_mm_mul_ps(quad, b.quad)).sum2();
#endif
    }

    INLINE SIMDQuad SIMDQuad::dot3(const SIMDQuad& a) const
    {
        return SIMDQuad(_mm_mul_ps(quad, a.quad)).sum3();
    }

    INLINE SIMDQuad SIMDQuad::dot3_SSE41(const SIMDQuad& a) const
    {
#ifdef PLATFORM_SSE41
        return SIMDQuad(_mm_dp_ps(quad, a.quad, 0x7F /* sum XYZ, write to all */));
#else
        return SIMDQuad(_mm_mul_ps(quad, a.quad)).sum3();
#endif
    }

    INLINE SIMDQuad SIMDQuad::dot4(const SIMDQuad& a) const
    {
        return SIMDQuad(_mm_mul_ps(quad, a.quad)).sum4();
    }

    INLINE SIMDQuad SIMDQuad::dot4_SSE41(const SIMDQuad& a) const
    {
#ifdef PLATFORM_SSE41
        return SIMDQuad(_mm_dp_ps(quad, a.quad, 0xFF /* sum all, write to all */));
#else
        return SIMDQuad(_mm_mul_ps(quad, a.quad)).sum4();
#endif
    }

    INLINE SIMDQuad SIMDQuad::sum2() const
    {
        auto sum = _mm_add_ss(quad, _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(1, 1, 1, 1))); // sum = x + y
        return SIMDQuad(SIMDQuad(_mm_shuffle_ps(sum, sum, _MM_SHUFFLE(0, 0, 0, 0)))); // to scalar
    }

    INLINE SIMDQuad SIMDQuad::sum3() const
    {
        auto x = _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(0,0,0,0)); // X
        auto y = _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(1,1,1,1)); // Y
        auto z = _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(2,2,2,2)); // Z
        return SIMDQuad(_mm_add_ps(_mm_add_ps(x,y), z)).x();
    }

    INLINE SIMDQuad SIMDQuad::sum3_SSE3() const
    {
#ifdef PLATFORM_SSE3
        auto sum1 = _mm_add_ss(quad, _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(1, 1, 1, 1))); // sum = x + y
        auto sum2 = _mm_add_ss(sum1, _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(2, 2, 2, 2))); // sum = x + y + z
        return _mm_shuffle_ps(sum2, sum2, _MM_SHUFFLE(0, 0, 0, 0)); // to scalar
#else
        return sum3();
#endif
    }

    INLINE SIMDQuad SIMDQuad::sum4() const
    {
        auto x = _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(0,0,0,0)); // X
        auto y = _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(1,1,1,1)); // Y
        auto z = _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(2,2,2,2)); // Z
        auto w = _mm_shuffle_ps(quad, quad, _MM_SHUFFLE(3,3,3,3)); // W
        return SIMDQuad(_mm_add_ss(_mm_add_ss(x,y), _mm_add_ss(z,w))).x();
    }

    INLINE SIMDQuad SIMDQuad::sum4_SSE3() const
    {
#ifdef PLATFORM_SSE3
        return _mm_hadd_ps(_mm_hadd_ps(quad, quad), _mm_hadd_ps(quad, quad));
#else
        return sum4();
#endif
    }

    INLINE SIMDQuad SIMDQuad::length2() const
    {
        return dot2(*this).sqrt();
    }

    INLINE SIMDQuad SIMDQuad::length3() const
    {
        return dot3(*this).sqrt();
    }

    INLINE SIMDQuad SIMDQuad::length4() const
    {
        return dot4(*this).sqrt();
    }

    INLINE SIMDQuad SIMDQuad::squareLength2() const
    {
        return dot2(*this);
    }

    INLINE SIMDQuad SIMDQuad::squareLength3() const
    {
        return dot3(*this);
    }

    INLINE SIMDQuad SIMDQuad::squareLength4() const
    {
        return dot4(*this);
    }

    INLINE int SIMDQuad::intSum2() const
    {
        __m128i iq = _mm_cvtps_epi32(quad); // x y - -
        __m128i sum1 = _mm_add_epi32(iq, _mm_shuffle_epi32(iq, _MM_SHUFFLE(1, 1, 1, 1))); // x+y - - -
        return _mm_cvtsi128_si32(sum1);
    }

    INLINE int SIMDQuad::intSum3() const
    {
        __m128i iq = _mm_cvtps_epi32(quad); // x y z -
        __m128i sum1 = _mm_add_epi32(iq, _mm_shuffle_epi32(iq, _MM_SHUFFLE(1, 1, 1, 1))); // x+y - - -
        __m128i sum2 = _mm_add_epi32(sum1, _mm_shuffle_epi32(iq, _MM_SHUFFLE(2, 2, 2, 2))); // x+y+z - - -
        return _mm_cvtsi128_si32(sum1);
    }

    INLINE int SIMDQuad::intSum4() const
    {
        __m128i iq = _mm_cvtps_epi32(quad); // x y z w
        __m128i sum1 = _mm_add_epi32(iq, _mm_shuffle_epi32(iq, _MM_SHUFFLE_REV(1, 0, 3, 2))); // (x y z w) + (y x w z) = (x+y x+y z+w z+w)
        __m128i sum2 = _mm_add_epi32(sum1, _mm_shuffle_epi32(sum1, _MM_SHUFFLE_REV(2, 2, 0, 0))); // (x+y x+y z+w z+w) + (z+w z+w x+y x+y) = (x+y+z+w x+y+z+w x+y+z+w x+y+z+w)
        return _mm_cvtsi128_si32(sum2);
    }

    INLINE SIMDQuad SIMDQuad::invLength2() const
    {
        __m128 length = dot2(*this);
        __m128 recip = _mm_rsqrt_ss(length);  // "estimate" opcode
        const __m128 three = { 3, 3, 3, 3 }; // aligned costs for fast load
        const __m128 half = { 0.5,0.5,0.5,0.5 };
        __m128 halfrecip = _mm_mul_ss(half, recip);
        __m128 threeminus_xrr = _mm_sub_ss(three, _mm_mul_ss(length, _mm_mul_ss(recip, recip)));
        return SIMDQuad(_mm_mul_ss(halfrecip, threeminus_xrr)).x();
    }

    INLINE SIMDQuad SIMDQuad::invLength3() const
    {
        __m128 length = dot3(*this);
        __m128 recip = _mm_rsqrt_ss(length);  // "estimate" opcode
        const __m128 three = { 3, 3, 3, 3 }; // aligned costs for fast load
        const __m128 half = { 0.5,0.5,0.5,0.5 };
        __m128 halfrecip = _mm_mul_ss(half, recip);
        __m128 threeminus_xrr = _mm_sub_ss(three, _mm_mul_ss(length, _mm_mul_ss(recip, recip)));
        return SIMDQuad(_mm_mul_ss(halfrecip, threeminus_xrr)).x();
    }

    INLINE SIMDQuad SIMDQuad::invLength4() const
    {
        __m128 length = dot4(*this);
        __m128 recip = _mm_rsqrt_ss(length);  // "estimate" opcode
        const __m128 three = { 3, 3, 3, 3 }; // aligned costs for fast load
        const __m128 half = { 0.5,0.5,0.5,0.5 };
        __m128 halfrecip = _mm_mul_ss(half, recip);
        __m128 threeminus_xrr = _mm_sub_ss(three, _mm_mul_ss(length, _mm_mul_ss(recip, recip)));
        return SIMDQuad(_mm_mul_ss(halfrecip, threeminus_xrr)).x();
    }

    INLINE SIMDQuad SIMDQuad::normalized2() const
    {
        return Build_XY_ZW(_mm_mul_ps(quad, invLength2()), _mm_setzero_ps());
    }

    INLINE SIMDQuad SIMDQuad::normalized3() const
    {
        return Build_XYZ_W(_mm_mul_ps(quad, invLength3()), _mm_setzero_ps());
    }

    INLINE SIMDQuad SIMDQuad::normalized4() const
    {
        return _mm_mul_ps(quad, invLength4());
    }

    INLINE bool SIMDQuad::operator==(const SIMDQuad& v) const
    {
        return cmp(v).isAllSet();
    }

    INLINE bool SIMDQuad::operator!=(const SIMDQuad& v) const
    {
        return cmpNE(v).isAnySet();
    }

    INLINE bool SIMDQuad::isNear2(const SIMDQuad& v, const SIMDQuad& eps) const
    {
        return (*this - v).squareLength2().cmpL(eps).isAllMaskSet(Mask_X);
    }

    INLINE bool SIMDQuad::isNear3(const SIMDQuad& v, const SIMDQuad& eps) const
    {
        return (*this - v).squareLength3().cmpL(eps).isAllMaskSet(Mask_X);
    }

    INLINE bool SIMDQuad::isNear4(const SIMDQuad& v, const SIMDQuad& eps) const
    {
        return (*this - v).squareLength4().cmpL(eps).isAllMaskSet(Mask_X);
    }

    INLINE bool SIMDQuad::isNearZero2(const SIMDQuad& eps) const
    {
        return squareLength2().cmpL(eps).isAllMaskSet(Mask_X);
    }

    INLINE bool SIMDQuad::isNearZero3(const SIMDQuad& eps) const
    {
        return squareLength3().cmpL(eps).isAllMaskSet(Mask_X);
    }

    INLINE bool SIMDQuad::isNearZero4(const SIMDQuad& eps) const
    {
        return squareLength4().cmpL(eps).isAllMaskSet(Mask_X);
    }

    //---

    INLINE SIMDQuad SIMDQuad::maskG() const
    {
        return _mm_cmpgt_ps(quad, _mm_setzero_ps());
    }

    INLINE SIMDQuad SIMDQuad::maskGE() const
    {
        return _mm_cmpge_ps(quad, _mm_setzero_ps());
    }

    INLINE SIMDQuad SIMDQuad::maskL() const
    {
        return _mm_cmplt_ps(quad, _mm_setzero_ps());
    }

    INLINE SIMDQuad SIMDQuad::maskLE() const
    {
        return _mm_cmple_ps(quad, _mm_setzero_ps());
    }

    INLINE SIMDQuad SIMDQuad::cmp(const SIMDQuad& a) const
    {
        return _mm_cmpeq_ps(quad, a.quad);
    }

    INLINE SIMDQuad SIMDQuad::cmpNE(const SIMDQuad& a) const
    {
        return _mm_cmpneq_ps(quad, a);
    }

    INLINE SIMDQuad SIMDQuad::cmpL(const SIMDQuad& a) const
    {
        return _mm_cmplt_ps(quad, a.quad);
    }

    INLINE SIMDQuad SIMDQuad::cmpG(const SIMDQuad& a) const
    {
        return _mm_cmpge_ps(quad, a.quad);
    }

    INLINE SIMDQuad SIMDQuad::cmpLE(const SIMDQuad& a) const
    {
        return _mm_cmple_ps(quad, a.quad);
    }

    INLINE SIMDQuad SIMDQuad::cmpGE(const SIMDQuad& a) const
    {
        return _mm_cmpge_ps(quad, a.quad);
    }

    //--

    INLINE bool SIMDQuad::isAnySet() const
    {
        return 0 != (_mm_movemask_ps(quad));
    }

    INLINE bool SIMDQuad::isAllSet() const
    {
        return Mask_All != _mm_movemask_ps(quad);
    }

    INLINE bool SIMDQuad::isAnyMaskSet(MaskValue m) const
    {
        return 0 != (_mm_movemask_ps(quad) & m);
    }

    INLINE bool SIMDQuad::isAllMaskSet(MaskValue m) const
    {
        return m == (_mm_movemask_ps(quad) & m);
    }

    INLINE bool SIMDQuad::isNegative() const
    {
        return (_mm_movemask_ps(quad) & 1) != 0;
    }

    INLINE bool SIMDQuad::isPositive() const
    {
        return (_mm_comigt_ss(quad, _mm_setzero_ps()) & 1) != 0;
    }

    //--

    INLINE SIMDQuad Build_XYZ_W(const SIMDQuad& a, const SIMDQuad& b)
    {
        __m128 p0 = _mm_unpackhi_ps(a, b); // a.z b.z a.w b.w
        return SIMDQuad(_mm_shuffle_ps(a, p0, _MM_SHUFFLE_REV(0, 1, 0, 3))); // a.x a.y a.z b.w
    }

    INLINE SIMDQuad Build_XY_ZW(const SIMDQuad& a, const SIMDQuad& b)
    {
        return SIMDQuad(_mm_shuffle_ps(a, b, _MM_SHUFFLE_REV(0, 1, 2, 3))); // a.x a.y b.z b.w
    }

    INLINE SIMDQuad Build_X_YZW(const SIMDQuad& a, const SIMDQuad& b)
    {
        __m128 p0 = _mm_unpacklo_ps(a, b); // a.x b.x a.y b.y
        return SIMDQuad(_mm_shuffle_ps(p0, b, _MM_SHUFFLE_REV(0, 3, 2, 3))); // a.x b.y b.z b.w
    }

    INLINE SIMDQuad Build_XY_XY(const SIMDQuad& a, const SIMDQuad& b)
    {
        return SIMDQuad(_mm_shuffle_ps(a, b, _MM_SHUFFLE_REV(0, 1, 0, 1)));
    }

    INLINE SIMDQuad Build_ZW_ZW(const SIMDQuad& a, const SIMDQuad& b)
    {
        return SIMDQuad(_mm_shuffle_ps(a, b, _MM_SHUFFLE_REV(2, 3, 2, 3)));
    }

    INLINE SIMDQuad Build_XX_YY(const SIMDQuad& a, const SIMDQuad& b)
    {
        return SIMDQuad(_mm_unpacklo_ps(a, b));
    }

    INLINE SIMDQuad Build_ZZ_WW(const SIMDQuad& a, const SIMDQuad& b)
    {
        return SIMDQuad(_mm_unpackhi_ps(a, b));
    }

    template< uint8_t FirstX, uint8_t FirstY, uint8_t SecondZ, uint8_t SecondW >
    INLINE SIMDQuad Shuffle(const SIMDQuad& a, const SIMDQuad& b)
    {
        return SIMDQuad(_mm_shuffle_ps(a, b, _MM_SHUFFLE_REV(FirstX, FirstY, SecondZ, SecondW)));
    }

    template< uint8_t Code >
    INLINE SIMDQuad Shuffle(const SIMDQuad& a, const SIMDQuad& b)
    {
        return SIMDQuad(_mm_shuffle_ps(a, b, Code));
    }

    INLINE static SIMDQuad Select(const SIMDQuad& a, const SIMDQuad& b, const SIMDQuad& mask)
    {
        __m128 temp1 = _mm_andnot_ps(mask, a);
        __m128 temp2 = _mm_and_ps(b, mask);
        return _mm_or_ps(temp1, temp2);
    }

    INLINE static SIMDQuad Select_SSE41(const SIMDQuad& a, const SIMDQuad& b, const SIMDQuad& mask)
    {
#ifdef PLATFORM_PLATFORM_SSE41
        return _mm_blendv_ps(a, b, mask);
#else
        return Select(a, b, mask);
#endif
    }

    INLINE static SIMDQuad Select(const SIMDQuad& a, const SIMDQuad& b, uint8_t mask)
    {
        __m128 temp1 = _mm_andnot_ps(MaskQuads[mask], a);
        __m128 temp2 = _mm_and_ps(b, MaskQuads[mask]);
        return _mm_or_ps(temp1, temp2);
    }

    INLINE static SIMDQuad Select_SSE41(const SIMDQuad& a, const SIMDQuad& b, uint8_t mask)
    {
#ifdef PLATFORM_PLATFORM_SSE41
        return SIMDVector(_mm_blendv_ps(a, b, MaskQuads[mask]));
#else
        return Select(a, b, mask);
#endif
    }

#else

#endif

    //! Convert to normal 2D vector
    INLINE Vector2 SIMDQuad::toVector2() const
    {
        return Vector2((*this)[0], (*this)[1]);
    }

    //! Convert to normal 3D vector
    INLINE Vector3 SIMDQuad::toVector3() const
    {
        return Vector3((*this)[0], (*this)[1], (*this)[2]);
    }

    //! Convert to normal 4D vector
    INLINE Vector4 SIMDQuad::toVector4() const
    {
        return Vector4((*this)[0], (*this)[1], (*this)[2], (*this)[3]);
    }

    INLINE float& SIMDQuad::operator[](uint32_t index)
    {
        return ((float*)this)[index];
    }

    INLINE const float& SIMDQuad::operator[](uint32_t index) const
    {
        return ((const float*)this)[index];
    }

} // base