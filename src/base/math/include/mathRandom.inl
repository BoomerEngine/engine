/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

///--

ALWAYS_INLINE double IRandom::unit()
{
    const double INV_MAX_UINT32 = 1.0 / (double)(uint64_t)(1ULL << 32); // 1/4B
    return next() * INV_MAX_UINT32;
}

ALWAYS_INLINE Vector2 IRandom::unit2()
{
    const double INV_MAX_UINT32 = 1.0 / (double)(uint64_t)(1ULL << 32); // 1/4B
    return Vector2(next() * INV_MAX_UINT32, next() * INV_MAX_UINT32);
}

ALWAYS_INLINE Vector3 IRandom::unit3()
{
    const double INV_MAX_UINT32 = 1.0 / (double)(uint64_t)(1ULL << 32); // 1/4B
    return Vector3(next() * INV_MAX_UINT32, next() * INV_MAX_UINT32, next() * INV_MAX_UINT32);
}

ALWAYS_INLINE Vector4 IRandom::unit4()
{
    const double INV_MAX_UINT32 = 1.0 / (double)(uint64_t)(1ULL << 32); // 1/4B
    return Vector4(next() * INV_MAX_UINT32, next() * INV_MAX_UINT32, next() * INV_MAX_UINT32, next() * INV_MAX_UINT32);
}

ALWAYS_INLINE double IRandom::range(double min, double max)
{
    return unit() * (max - min) + min;
}

ALWAYS_INLINE uint32_t IRandom::range(uint32_t max)
{
    if (max <= 1)
        return 0;

    auto big = (uint64_t)next() * max;
    return (uint32_t)(big >> 32);
}

///--

template< typename T >
INLINE Vector2 RandRectPoint(T& rand, const Vector2& min, const Vector2& max)
{
    return RandRectPoint(rand.unit2(), min, max);
}

template< typename T >
INLINE Vector2 RandCirclePoint(T& rand, const Vector2& center, float radius)
{
    return RandCirclePoint(rand.unit2(), center, radius);
}

template< typename T >
INLINE Vector3 RandBoxPoint(T& rand, const Vector3& min, const Vector3& max)
{
    return RandBoxPoint(rand.unit3(), min, max);
}

template< typename T >
INLINE Vector3 RandBoxPoint(T& rand, const Box& box)
{
    return RandBoxPoint(rand.unit3(), box);
}

template< typename T >
INLINE Vector3 RandBoxPoint(T& rand, const OBB& box)
{
    return RandBoxPoint(rand.unit3(), box);
}

template< typename T >
INLINE Vector3 RandSpherePoint(T& rand)
{
    return RandSpherePoint(rand.unit2());
}

template< typename T >
INLINE Vector3 RandSpherePoint(T& rand, const Vector3& center, float radius)
{
    return RandSpherePoint(rand.unit2(), center, radius);
}

template< typename T >
INLINE Vector3 RandSpherePoint(T& rand, const Sphere& sphere)
{
    return RandSpherePoint(rand.unit2(), sphere);
}

template< typename T >
INLINE Vector3 RandSphereSurfacePoint(T& rand)
{
    return RandSphereSurfacePoint(rand.unit2());
}

template< typename T >
INLINE Vector3 RandSphereSurfacePoint(T& rand, const Vector3& center, float radius)
{
    return RandSphereSurfacePoint(rand.unit2(), center, radius);
}

template< typename T >
INLINE Vector3 RandSphereSurfacePoint(T& rand, const Sphere& sphere)
{
    return RandSphereSurfacePoint(rand.unit2(), sphere);
}

template< typename T >
INLINE Vector3 RandHemiSphereSurfacePoint(T& rand)
{
    return RandHemiSphereSurfacePoint(rand.unit2());
}

template< typename T >
INLINE Vector3 RandHemiSphereSurfacePoint(T& rand, const Vector3& N)
{
    return RandHemiSphereSurfacePoint(rand.unit2(), N);
}

template< typename T >
INLINE Vector3 RandHemiSphereSurfacePoint(T& rand, const Vector3& center, float radius, const Vector3& normal)
{
    return RandHemiSphereSurfacePoint(rand.unit2(), center, radius, normal);
}

template< typename T >
INLINE Vector3 RandTrianglePoint(T& rand)
{
    return RandHemiSphereSurfacePoint(rand.unit2());
}

template< typename T >
INLINE Vector3 RandTrianglePoint(T& rand, const Vector3& a, const Vector3& b, const Vector3& c)
{
    return RandHemiSphereSurfacePoint(rand.unit2(), a, b, c);
}

///--

END_BOOMER_NAMESPACE(base)