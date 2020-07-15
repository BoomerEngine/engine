/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#pragma once

namespace base
{
    //----------------------------------------
    //-- Very simple random number generator

    struct FastRandState
    {
        uint64_t state = 0;

        BASE_MATH_API FastRandState(uint64_t seed = 0);
    };

    extern BASE_MATH_API void RandInit(FastRandState& state, uint32_t seed = 0);

    extern BASE_MATH_API uint32_t Rand(FastRandState& state); // 0 - 0xFFFFFFFF

    //----------------------------------------
    //-- Mersenne-Twister random number generator

    struct MTRandState
    {
        static const uint32_t SIZE = 624;

        uint32_t MT[SIZE];
        uint32_t MT_TEMPERED[SIZE];
        uint32_t index = SIZE;

        BASE_MATH_API MTRandState(uint32_t seed = 0);
    };

    extern BASE_MATH_API void RandInit(MTRandState& state, uint32_t seed = 0);

    extern BASE_MATH_API uint32_t Rand(MTRandState& state); //  0 - 0xFFFFFFFF

    //----------------------------------------
    //-- Common stuff

    template< typename T >
    ALWAYS_INLINE double RandOne(T& state); // [0,1>

    template< typename T >
    ALWAYS_INLINE double RandRange(T& state, double min, double max); // [min,max>

    template< typename T >
    ALWAYS_INLINE uint32_t RandMax(T& state, uint32_t max); // [0, max-1]

    //---

    // convert 2 uniform random variables to a random 2D point in a rectangle with uniform distribution
    extern BASE_MATH_API Vector2 RandRectPoint(const Vector2& rand, const Vector2& min, const Vector2& max);

    // convert 2 uniform random variables to a random 2D point in a circle with uniform distribution, NOTE: circle boundary is excluded
    extern BASE_MATH_API Vector2 RandCirclePoint(const Vector2& rand, const Vector2& center, float radius);

    // convert 3 uniform random variables to a random 3D point inside a box
    extern BASE_MATH_API Vector3 RandBoxPoint(const Vector3& rand, const Vector3& min, const Vector3& max);

    // convert 3 uniform random variables to a random 3D point inside a box
    extern BASE_MATH_API Vector3 RandBoxPoint(const Vector3& rand, const Box& box);

    // convert 3 uniform random variables to a random 3D point inside a box
    extern BASE_MATH_API Vector3 RandBoxPoint(const Vector3& rand, const OBB& box);

    // convert 3 uniform random variables to a random 3D point inside an sphere
    extern BASE_MATH_API Vector3 RandSpherePoint(const Vector3& rand);

    // convert 3 uniform random variables to a random 3D point inside a sphere
    extern BASE_MATH_API Vector3 RandSpherePoint(const Vector3& rand, const Vector3& center, float radius);

    // convert 3 uniform random variables to a random 3D point inside a sphere
    extern BASE_MATH_API Vector3 RandSpherePoint(const Vector3& rand, const Sphere& sphere);

    // convert 2 (TWO) uniform random variables to a random 3D point ON A SURFACE of the unit sphere
    extern BASE_MATH_API Vector3 RandSphereSurfacePoint(const Vector2& rand);

    // convert 2 (TWO) uniform random variables to a random 3D point ON A SURFACE of the sphere
    extern BASE_MATH_API Vector3 RandSphereSurfacePoint(const Vector2& rand, const Vector3& center, float radius);

    // convert 3 uniform random variables to a random 3D point inside a sphere
    extern BASE_MATH_API Vector3 RandSphereSurfacePoint(const Vector2& rand, const Sphere& sphere);

    // convert 2 (TWO) uniform random variables to a random 3D point on a unit hemi-sphere at 0,0,0 and with N=0,0,1 (standard tangent-space hemisphere)
    extern BASE_MATH_API Vector3 RandHemiSphereSurfacePoint(const Vector2& rand);

    // convert 2 (TWO) uniform random variables to a random 3D point on a unit hemi-sphere at 0,0,0 with specified N
    extern BASE_MATH_API Vector3 RandHemiSphereSurfacePoint(const Vector2& rand, const Vector3& N);

    // convert 2 (TWO) uniform random variables to a random 3D point on a hemi-sphere facing given direction
    extern BASE_MATH_API Vector3 RandHemiSphereSurfacePoint(const Vector2& rand, const Vector3& center, float radius, const Vector3& normal);

    // convert 2 (TWO) uniform random variables to a random point inside a unit triangle (basically we return the barycentric coordinates)
    extern BASE_MATH_API Vector3 RandTrianglePoint(const Vector2& rand);

    // convert 2 (TWO) uniform random variables to a random point inside a triangle
    extern BASE_MATH_API Vector3 RandTrianglePoint(const Vector2& rand, const Vector3& a, const Vector3& b, const Vector3& c);

    //---

    // get random 2D unit point
    template< typename T >
    INLINE Vector2 RandUnit2(T& rand);

    // get random 3D unit point
    template< typename T >
    INLINE Vector3 RandUnit3(T& rand);

    // get random 4D unit point
    template< typename T >
    INLINE Vector4 RandUnit4(T& rand);

    // get random 2D point in a rectangle with uniform distribution
    template< typename T >
    INLINE Vector2 RandRectPoint(T& rand, const Vector2& min, const Vector2& max);

    // get random 2D point in a circle with uniform distribution, NOTE: circle boundary is excluded
    template< typename T >
    INLINE Vector2 RandCirclePoint(T& rand, const Vector2& center, float radius);

    // convert 3 uniform random variables to a random 3D point inside a box
    template< typename T >
    INLINE Vector3 RandBoxPoint(T& rand, const Vector3& min, const Vector3& max);

    // convert 3 uniform random variables to a random 3D point inside a box
    template< typename T >
    INLINE Vector3 RandBoxPoint(T& rand, const Box& box);

    // convert 3 uniform random variables to a random 3D point inside a box
    template< typename T >
    INLINE Vector3 RandBoxPoint(T& rand, const OBB& box);

    // convert 3 uniform random variables to a random 3D point inside an sphere
    template< typename T >
    INLINE Vector3 RandSpherePoint(T& rand);

    // convert 3 uniform random variables to a random 3D point inside a sphere
    template< typename T >
    INLINE Vector3 RandSpherePoint(T& rand, const Vector3& center, float radius);

    // convert 3 uniform random variables to a random 3D point inside a sphere
    template< typename T >
    INLINE Vector3 RandSpherePoint(T& rand, const Sphere& sphere);

    // convert 2 (TWO) uniform random variables to a random 3D point ON A SURFACE of the unit sphere
    template< typename T >
    INLINE Vector3 RandSphereSurfacePoint(T& rand);

    // convert 2 (TWO) uniform random variables to a random 3D point ON A SURFACE of the sphere
    template< typename T >
    INLINE Vector3 RandSphereSurfacePoint(T& rand, const Vector3& center, float radius);

    // convert 3 uniform random variables to a random 3D point inside a sphere
    template< typename T >
    INLINE Vector3 RandSphereSurfacePoint(T& rand, const Sphere& sphere);

    // convert 2 (TWO) uniform random variables to a random 3D point on a unit hemi-sphere at 0,0,0 and with N=0,0,1 (standard tangent-space hemisphere)
    template< typename T >
    INLINE Vector3 RandHemiSphereSurfacePoint(T& rand);

    // convert 2 (TWO) uniform random variables to a random 3D point on a unit hemi-sphere at 0,0,0 with specified N
    template< typename T >
    INLINE Vector3 RandHemiSphereSurfacePoint(T& rand, const Vector3& N);

    // convert 2 (TWO) uniform random variables to a random 3D point on a hemi-sphere facing given direction
    template< typename T >
    INLINE Vector3 RandHemiSphereSurfacePoint(T& rand, const Vector3& center, float radius, const Vector3& normal);

    // convert 2 (TWO) uniform random variables to a random point inside a unit triangle (basically we return the barycentric coordinates)
    template< typename T >
    INLINE Vector3 RandTrianglePoint(T& rand);

    // convert 2 (TWO) uniform random variables to a random point inside a triangle
    template< typename T >
    INLINE Vector3 RandTrianglePoint(T& rand, const Vector3& a, const Vector3& b, const Vector3& c);

    //---

} // base
