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

    ///--

    template< typename T >
    ALWAYS_INLINE double RandOne(T& state)
    {
        const double INV_MAX_UINT32 = 1.0 / (double)(uint64_t)(1ULL << 32); // 1/4B
        return (double)Rand(state) * INV_MAX_UINT32;
    }

    template< typename T >
    ALWAYS_INLINE double RandRange(T& state, double min, double max)
    {
        return min + RandOne(state) * (max - min);
    }

    template< typename T >
    ALWAYS_INLINE uint32_t RandMax(T& state, uint32_t max)
    {
        if (max <= 1)
            return 0;

        return (uint32_t)(((uint64_t)Rand(state) * max) >> 32);
    }

    ///--

    // get random 2D unit point
    template< typename T >
    INLINE Vector2 RandUnit2(T& rand)
    {
        return Vector2(RandOne(rand), RandOne(rand));
    }

    // get random 3D unit point
    template< typename T >
    INLINE Vector3 RandUnit3(T& rand)
    {
        return Vector3(RandOne(rand), RandOne(rand), RandOne(rand));
    }

    // get random 4D unit point
    template< typename T >
    INLINE Vector4 RandUnit4(T& rand)
    {
        return Vector4(RandOne(rand), RandOne(rand), RandOne(rand), RandOne(rand));
    }

    template< typename T >
    INLINE Vector2 RandRectPoint(T& rand, const Vector2& min, const Vector2& max)
    {
        return RandRectPoint(RandUnit2(rand), min, max);
    }

    template< typename T >
    INLINE Vector2 RandCirclePoint(T& rand, const Vector2& center, float radius)
    {
        return RandCirclePoint(RandUnit2(rand), center, radius);
    }

    template< typename T >
    INLINE Vector3 RandBoxPoint(T& rand, const Vector3& min, const Vector3& max)
    {
        return RandBoxPoint(RandUnit3(rand), min, max);
    }

    template< typename T >
    INLINE Vector3 RandBoxPoint(T& rand, const Box& box)
    {
        return RandBoxPoint(RandUnit3(rand), box);
    }

    template< typename T >
    INLINE Vector3 RandBoxPoint(T& rand, const OBB& box)
    {
        return RandBoxPoint(RandUnit3(rand), box);
    }

    template< typename T >
    INLINE Vector3 RandSpherePoint(T& rand)
    {
        return RandSpherePoint(RandUnit2(rand));
    }

    template< typename T >
    INLINE Vector3 RandSpherePoint(T& rand, const Vector3& center, float radius)
    {
        return RandSpherePoint(RandUnit2(rand), center, radius);
    }

    template< typename T >
    INLINE Vector3 RandSpherePoint(T& rand, const Sphere& sphere)
    {
        return RandSpherePoint(RandUnit2(rand), sphere);
    }

    template< typename T >
    INLINE Vector3 RandSphereSurfacePoint(T& rand)
    {
        return RandSphereSurfacePoint(RandUnit2(rand));
    }

    template< typename T >
    INLINE Vector3 RandSphereSurfacePoint(T& rand, const Vector3& center, float radius)
    {
        return RandSphereSurfacePoint(RandUnit2(rand), center, radius);
    }

    template< typename T >
    INLINE Vector3 RandSphereSurfacePoint(T& rand, const Sphere& sphere)
    {
        return RandSphereSurfacePoint(RandUnit2(rand), sphere);
    }

    template< typename T >
    INLINE Vector3 RandHemiSphereSurfacePoint(T& rand)
    {
        return RandHemiSphereSurfacePoint(RandUnit2(rand));
    }

    template< typename T >
    INLINE Vector3 RandHemiSphereSurfacePoint(T& rand, const Vector3& N)
    {
        return RandHemiSphereSurfacePoint(RandUnit2(rand), N);
    }

    template< typename T >
    INLINE Vector3 RandHemiSphereSurfacePoint(T& rand, const Vector3& center, float radius, const Vector3& normal)
    {
        return RandHemiSphereSurfacePoint(RandUnit2(rand), center, radius, normal);
    }

    template< typename T >
    INLINE Vector3 RandTrianglePoint(T& rand)
    {
        return RandHemiSphereSurfacePoint(RandUnit2(rand));
    }

    template< typename T >
    INLINE Vector3 RandTrianglePoint(T& rand, const Vector3& a, const Vector3& b, const Vector3& c)
    {
        return RandHemiSphereSurfacePoint(RandUnit2(rand), a, b, c);
    }

    ///--

} // base