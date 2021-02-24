/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: simd\#]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//--

enum MaskValue
{
    Mask_None = 0,

    Mask_X = 1,
    Mask_Y = 2,
    Mask_Z = 4,
    Mask_W = 8,

    Mask_XY = 3,
    Mask_XZ = 5,
    Mask_XW = 9,
    Mask_YZ = 6,
    Mask_YW = 10,
    Mask_ZW = 12,

    Mask_XYZ = 7,
    Mask_XYW = 11,
    Mask_XZW = 13,
    Mask_YZW = 14,

    Mask_All = 15,
};

//---

class SIMDQuad;

extern BASE_MATH_API const SIMDQuad FullOnesBitMask[1];
extern BASE_MATH_API const SIMDQuad BitMasks[16];
extern BASE_MATH_API const SIMDQuad MaskQuads[16];

extern BASE_MATH_API const SIMDQuad ZeroVector[1];
extern BASE_MATH_API const SIMDQuad OnesVector[1];
extern BASE_MATH_API const SIMDQuad EXVector[1];
extern BASE_MATH_API const SIMDQuad EYVector[1];
extern BASE_MATH_API const SIMDQuad EZVector[1];
extern BASE_MATH_API const SIMDQuad EWVector[1];

extern BASE_MATH_API const SIMDQuad ZeroValue[1];
extern BASE_MATH_API const SIMDQuad OneValue[1];
extern BASE_MATH_API const SIMDQuad EpsilonValue[1];
extern BASE_MATH_API const SIMDQuad EpsilonSquaredValue[1];

extern BASE_MATH_API const SIMDQuad SignBitVecto[1];
extern BASE_MATH_API const SIMDQuad SignBitVector3[1];
extern BASE_MATH_API const SIMDQuad SignMaskVector[1];
extern BASE_MATH_API const SIMDQuad SignMaskVector3[1];


//---

/// 128-bit simd quad
TYPE_ALIGN(16, class) BASE_MATH_API SIMDQuad
{
public:
    INLINE SIMDQuad();
    INLINE SIMDQuad(float x, float y, float z, float w);
    INLINE SIMDQuad(uint32_t x, uint32_t y, uint32_t z, uint32_t w);
    INLINE SIMDQuad(float s);
    INLINE SIMDQuad(const float* ptr); // unaligned
    INLINE SIMDQuad(const float* ptr, uint8_t alignment); // aligned
    INLINE SIMDQuad(const SIMDQuad& xyzPart, const SIMDQuad& wPart);
    INLINE SIMDQuad(const SIMDQuad& x, const SIMDQuad& y, const SIMDQuad& z, const SIMDQuad& w);
    INLINE SIMDQuad(const SIMDQuad& src);
    INLINE SIMDQuad(const SIMDQuad& src, uint8_t component);

    INLINE SIMDQuad x() const; // replicate
    INLINE SIMDQuad y() const; // replicate
    INLINE SIMDQuad z() const; // replicate
    INLINE SIMDQuad w() const; // replicate
    INLINE SIMDQuad xyxy() const;
    INLINE SIMDQuad xy00() const;
    INLINE SIMDQuad zwzw() const;
    INLINE SIMDQuad xxyy() const;
    INLINE SIMDQuad zzww() const;
    INLINE SIMDQuad _x000() const;
    INLINE SIMDQuad _0y00() const;
    INLINE SIMDQuad _00z0() const;
    INLINE SIMDQuad _000w() const;
    INLINE SIMDQuad _00xy() const;
    INLINE SIMDQuad _00zw() const;
    INLINE SIMDQuad _xy00() const;
    INLINE SIMDQuad _zw00() const;

    INLINE SIMDQuad operator-() const; // negation
    INLINE SIMDQuad operator~() const; // bitwise not

    INLINE SIMDQuad operator*(const SIMDQuad& a) const;
    INLINE SIMDQuad operator/(const SIMDQuad& a) const;
    INLINE SIMDQuad operator+(const SIMDQuad& a) const;
    INLINE SIMDQuad operator-(const SIMDQuad& a) const;
    INLINE SIMDQuad operator*(float a) const;
    INLINE SIMDQuad operator/(float a) const;
    INLINE SIMDQuad operator+(float a) const;
    INLINE SIMDQuad operator-(float a) const;

    INLINE SIMDQuad& operator*=(const SIMDQuad& a);
    INLINE SIMDQuad& operator/=(const SIMDQuad& a);
    INLINE SIMDQuad& operator+=(const SIMDQuad& a);
    INLINE SIMDQuad& operator-=(const SIMDQuad& a);
    INLINE SIMDQuad& operator*=(float a);
    INLINE SIMDQuad& operator/=(float a);
    INLINE SIMDQuad& operator+=(float a);
    INLINE SIMDQuad& operator-=(float a);

    INLINE SIMDQuad operator|(const SIMDQuad& a) const;
    INLINE SIMDQuad operator&(const SIMDQuad& a) const;
    INLINE SIMDQuad operator^(const SIMDQuad& a) const;
    INLINE SIMDQuad operator|(uint32_t a) const;
    INLINE SIMDQuad operator&(uint32_t a) const;
    INLINE SIMDQuad operator^(uint32_t a) const;

    INLINE SIMDQuad& operator|=(const SIMDQuad& a);
    INLINE SIMDQuad& operator&=(const SIMDQuad& a);
    INLINE SIMDQuad& operator^=(const SIMDQuad& a);
    INLINE SIMDQuad& operator|=(uint32_t a);
    INLINE SIMDQuad& operator&=(uint32_t a);
    INLINE SIMDQuad& operator^=(uint32_t a);

    INLINE bool operator==(const SIMDQuad& v) const;
    INLINE bool operator!=(const SIMDQuad& v) const;

    INLINE SIMDQuad& operator=(const SIMDQuad& v);

    INLINE float& operator[](uint32_t index); // component access - NOTE: goes through memory
    INLINE const float& operator[](uint32_t index) const;  // component access - NOTE: goes through memory

    //---

    //! store vector values to unaligned memory, taints the cache
    INLINE void storeUnaligned(void* ptr) const;

    //! store vector values to aligned memory, taints the cache
    INLINE void storeAligned(void* ptr) const;

    //! store vector values to aligned memory, DOES NOT TOUCH THE CACHE
    INLINE void storeAlignedBypassCache(void* ptr) const;

    //! replicate single component to all components, return new vector
    //! NOTE: this function is slow and uses switch() so if the component index is know compile time use the x() y() z() or w()
    INLINE SIMDQuad replicate(int componentIndex) const;

    //! replicate single component to all components, return new vector
    template< uint8_t Component >
    INLINE SIMDQuad replicate() const;

    //--

    //! round components to nearest whole number, return new vector
    INLINE SIMDQuad round() const;

    //! round components toward zero (truncate the fractional part)
    INLINE SIMDQuad trunc() const;

    //! round components toward positive infinity, return new vector
    INLINE SIMDQuad ceil() const;

    //! round components toward negative infinity, return new vector
    INLINE SIMDQuad floor() const;

    //! get the fractional part of the number
    INLINE SIMDQuad frac() const;

    //! calculate dot product of 2 vectors, take only 2 components into account, result is replicated over all components
    INLINE SIMDQuad dot2(const SIMDQuad& b) const;

    //! calculate dot product of 2 vectors, take only 3 components into account, result is replicated over all components
    INLINE SIMDQuad dot3(const SIMDQuad& b) const;

    //! calculate dot product of 2 vectors, take all components into account, result is replicated over all components
    INLINE SIMDQuad dot4(const SIMDQuad& b) const;

    //! (SSE41) round components to nearest whole number, return new vector
    INLINE SIMDQuad round_SSE41() const;

    //! (SSE41) round components toward zero (truncate the fractional part)
    INLINE SIMDQuad trunc_SSE41() const;

    //! (SSE41) round components toward positive infinity, return new vector
    INLINE SIMDQuad ceil_SSE41() const;

    //! (SSE41) round components toward negative infinity, return new vector
    INLINE SIMDQuad floor_SSE41() const;

    //! (SSE41) compute the fraction (1 - floor(x))
    INLINE SIMDQuad frac_SSE41() const;

    //! (SSE41) calculate dot product of 2 vectors, take only 2 components into account, result is replicated over all components
    INLINE SIMDQuad dot2_SSE41(const SIMDQuad& b) const;

    //! (SSE41) calculate dot product of 2 vectors, take only 3 components into account, result is replicated over all components
    INLINE SIMDQuad dot3_SSE41(const SIMDQuad& b) const;

    //! (SSE41) calculate dot product of 2 vectors, take all components into account, result is replicated over all components
    INLINE SIMDQuad dot4_SSE41(const SIMDQuad& b) const;

    //! calculate sum of 2 components
    INLINE SIMDQuad sum2() const;

    //! calculate sum of 3 components
    INLINE SIMDQuad sum3() const;

    //! calculate sum of 4 components
    INLINE SIMDQuad sum4() const;

    //! (SSE3) calculate sum of 3 components
    INLINE SIMDQuad sum3_SSE3() const;

    //! (SSE3) calculate sum of 4 components
    INLINE SIMDQuad sum4_SSE3() const;

    //! compute square root of all components
    INLINE SIMDQuad sqrt() const;

    //! compute one over square root of all components
    INLINE SIMDQuad rsqrt() const;

    //! compute 1/x of all components
    INLINE SIMDQuad rcp() const;

    //! sum of the 2 components converted to integers
    INLINE int intSum2() const;

    //! sum of the 3 components converted to integers
    INLINE int intSum3() const;

    //! sum of the all components converted to integers
    INLINE int intSum4() const;

    //! calculate length of vector (2 components only), result is replicated over all components
    INLINE SIMDQuad length2() const;

    //! calculate length of vector (3 components only), result is replicated over all components
    INLINE SIMDQuad length3() const;

    //! calculate length of vector (all components), result is replicated over all components
    INLINE SIMDQuad length4() const;

    //! calculate square length of vector (2 components only), result is replicated over all components
    INLINE SIMDQuad squareLength2() const;

    //! calculate square length of vector (3 components only), result is replicated over all components
    INLINE SIMDQuad squareLength3() const;

    //! calculate square length of vector (all components), result is replicated over all components
    INLINE SIMDQuad squareLength4() const;

    //! calculate 1/length of vector (2 components only), result is replicated over all components
    //! NOTE: result if undefined if length is 0
    INLINE SIMDQuad invLength2() const;

    //! calculate 1/length of vector (3 components only), result is replicated over all components
    //! NOTE: result if undefined if length is 0
    INLINE SIMDQuad invLength3() const;

    //! calculate 1/length of vector (all components), result is replicated over all components
    //! NOTE: result if undefined if length is 0
    INLINE SIMDQuad invLength4() const;

    //! calculate product of 2 components (x*y)
    INLINE SIMDQuad product2() const;

    //! calculate product of 3 components (x*y*z)
    INLINE SIMDQuad product3() const;

    //! calculate product of 4 components (x*y*z*w)
    INLINE SIMDQuad product4() const;

    //! returns normalized vector, assumes vector has non zero length (2 components only), rest of the components are set to 0
    //! NOTE: If the vector length is zero the operation is not defined
    INLINE SIMDQuad normalized2() const;

    //! returns normalized vector, assumes vector has non zero length (3 components only), rest of the components are set to 0
    //! NOTE: If the vector length is zero the operation is not defined
    INLINE SIMDQuad normalized3() const;

    //! returns normalized vector, assumes vector has non zero length
    //! NOTE: If the vector length is zero the operation is not defined
    INLINE SIMDQuad normalized4() const;

    //--

    //! compare components to be greater than 0
    INLINE SIMDQuad maskG() const;

    //! compare components to be greater or equal to 0
    INLINE SIMDQuad maskGE() const;

    //! compare components to be less than 0
    INLINE SIMDQuad maskL() const;

    //! compare components to be less or equal to 0
    INLINE SIMDQuad maskLE() const;

    //--

    // check if 2 first components are near given values
    INLINE bool isNear2(const SIMDQuad& v, const SIMDQuad& eps = EpsilonValue[0]) const;

    // check if 3 first components are near given values
    INLINE bool isNear3(const SIMDQuad& v, const SIMDQuad& eps = EpsilonValue[0]) const;

    // check if all components are near given values
    INLINE bool isNear4(const SIMDQuad& v, const SIMDQuad& eps = EpsilonValue[0]) const;

    // check if 2 first components are near zero
    INLINE bool isNearZero2(const SIMDQuad& eps = EpsilonValue[0]) const;

    // check if 3 first components are near zero
    INLINE bool isNearZero3(const SIMDQuad& eps = EpsilonValue[0]) const;

    // check if all components are near zero
    INLINE bool isNearZero4(const SIMDQuad& eps = EpsilonValue[0]) const;

    //--

    // compute mask for components that are equal
    INLINE SIMDQuad cmp(const SIMDQuad& a) const;

    // compute mask for components that are not equal
    INLINE SIMDQuad cmpNE(const SIMDQuad& a) const;

    // compute mask for components that are less than in other vector
    INLINE SIMDQuad cmpL(const SIMDQuad& a) const;

    // compute mask for components that are greater than in other vector
    INLINE SIMDQuad cmpG(const SIMDQuad& a) const;

    // compute mask for components that are less or equal to the ones in the other vector
    INLINE SIMDQuad cmpLE(const SIMDQuad& a) const;

    // compute mask for components that are greater or equal to the ones in the other vector
    INLINE SIMDQuad cmpGE(const SIMDQuad& a) const;

    //--

    //! checks if any given components are non zero, usually used with the cmpXX or maskXX
    INLINE bool isAnyMaskSet(MaskValue m) const;

    //! Checks if all given components are non zero, usually used with the cmpXX or maskXX
    INLINE bool isAllMaskSet(MaskValue m) const;

    //! checks if any of the four components is non zero, usually used with the cmpXX or maskXX
    INLINE bool isAnySet() const;

    //! checks if all four components are non zero, usually used with the cmpXX or maskXX
    INLINE bool isAllSet() const;

    //! check if value is negative
    INLINE bool isNegative() const;

    //! check if value is positive
    INLINE bool isPositive() const;

    //--

    //! convert to normal 2D vector
    INLINE Vector2 toVector2() const;

    //! convert to normal 3D vector
    INLINE Vector3 toVector3() const;

    //! convert to normal 4D vector
    INLINE Vector4 toVector4() const;

public:
#ifdef PLATFORM_SSE
    INLINE SIMDQuad(__m128 val) : quad(val) {}
    INLINE SIMDQuad(__m128i val) : quadi(val) {}
    INLINE operator __m128() const { return quad; }
    INLINE operator __m128i() const { return quadi; }
#endif

#if defined(PLATFORM_SSE)
    union
    {
        __m128  quad;
        __m128i quadi;
    };
#else
    union
    {
        float floats[4];
        uint32_t uints[4];
    };
#endif
};

//---

//! build new vector, take XYZ from first argument and W from the second argument
INLINE static SIMDQuad Build_XYZ_W(const SIMDQuad& a, const SIMDQuad& b);

//! build new vector, take XY from first argument and ZW from the second argument
INLINE static SIMDQuad Build_XY_ZW(const SIMDQuad& a, const SIMDQuad& b);

//! build new vector, take X from first argument and YZW from the second argument
INLINE static SIMDQuad Build_X_YZW(const SIMDQuad& a, const SIMDQuad& b);

//! build new vector, take Xs from first argument and Ys from the second argument (unpacklo)
INLINE static SIMDQuad Build_XX_YY(const SIMDQuad& a, const SIMDQuad& b);

//! build new vector, take Zs from first argument and Ws from the second argument (unpackhi)
INLINE static SIMDQuad Build_ZZ_WW(const SIMDQuad& a, const SIMDQuad& b);

//! build new vector, take only XY from both vectors
INLINE static SIMDQuad Build_XY_XY(const SIMDQuad& a, const SIMDQuad& b);

//! build new vector, take only ZW from both vectors
INLINE static SIMDQuad Build_ZW_ZW(const SIMDQuad& a, const SIMDQuad& b);

//! shuffle by selecting components
template< uint8_t FirstX, uint8_t FirstY, uint8_t SecondZ, uint8_t SecondW >
INLINE static SIMDQuad Shuffle(const SIMDQuad& a, const SIMDQuad& b);

//! shuffle with direct core
template< uint8_t Code >
INLINE static SIMDQuad Shuffle(const SIMDQuad& a, const SIMDQuad& b);

//! select using mask
INLINE static SIMDQuad Select(const SIMDQuad& a, const SIMDQuad& b, const SIMDQuad& mask);

//! select using mask
INLINE static SIMDQuad Select_SSE41(const SIMDQuad& a, const SIMDQuad& b, const SIMDQuad& mask);

//! select using static mask
INLINE static SIMDQuad Select(const SIMDQuad& a, const SIMDQuad& b, uint8_t mask);

//! select using static mask
INLINE static SIMDQuad Select_SSE41(const SIMDQuad& a, const SIMDQuad& b, uint8_t mask);

//---

END_BOOMER_NAMESPACE(base)
