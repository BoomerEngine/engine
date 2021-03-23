/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

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

INLINE Vector2 RandomBase::RandRectPoint(const Vector2& rand, const Vector2& min, const Vector2& max)
{
    return min + rand * (max - min);
}

INLINE Vector2 RandomBase::RandCirclePoint(const Vector2& rand, const Vector2& center, float radius)
{
    auto r = std::sqrtf(rand.x);
    auto a = rand.x * TWOPI;
    return Vector2(r * std::cosf(a), r * std::sinf(a));
}

INLINE Vector3 RandomBase::RandBoxPoint(const Vector3& rand, const Vector3& min, const Vector3& max)
{
    return min + rand * (max - min);
}

INLINE Vector3 RandomBase::RandBoxPoint(const Vector3& rand, const Box& box)
{
    return box.min + rand * box.extents();
}

INLINE Vector3 RandomBase::RandBoxPoint(const Vector3& rand, const OBB& box)
{
    Vector3 ret = box.position();
    ret += box.edge1AndLength.xyz() * rand.x * box.edge1AndLength.w;
    ret += box.edge2AndLength.xyz() * rand.y * box.edge2AndLength.w;
    ret += box.edgeC() * rand.z * box.positionAndLength.w;
    return ret;
}

INLINE Vector3 RandomBase::RandSpherePoint(const Vector3& rand)
{
    auto theta = rand.x * 2.0 * PI;
    auto phi = std::acosf(2.0 * rand.y - 1.0);
    auto r = std::cbrtf(rand.z);
    auto sinTheta = std::sinf(theta);
    auto cosTheta = std::cosf(theta);
    auto sinPhi = std::sinf(phi);
    auto cosPhi = std::cosf(phi);
    return Vector3(r * sinPhi * cosTheta, r * sinPhi * sinTheta, r * cosPhi);
}

INLINE Vector3 RandomBase::RandSpherePoint(const Vector3& rand, const Vector3& center, float radius)
{
    return center + (RandSpherePoint(rand) * radius);
}

INLINE Vector3 RandomBase::RandSpherePoint(const Vector3& rand, const Sphere& sphere)
{
    return sphere.position() + (RandSpherePoint(rand) * sphere.radius());
}

INLINE Vector3 RandomBase::RandSphereSurfacePoint(const Vector2& rand)
{
    auto theta = TWOPI * rand.x;
    auto phi = std::acosf(1.0f - 2.0f * rand.y);
    return Vector3(std::sinf(phi) * std::cosf(theta), std::sinf(phi) * std::sinf(theta), std::cosf(phi));
}

INLINE Vector3 RandomBase::RandSphereSurfacePoint(const Vector2& rand, const Vector3& center, float radius)
{
    return center + (RandSphereSurfacePoint(rand) * radius);
}

INLINE Vector3 RandomBase::RandSphereSurfacePoint(const Vector2& rand, const Sphere& sphere)
{
    return sphere.position() + (RandSphereSurfacePoint(rand) * sphere.radius());
}

INLINE Vector3 RandomBase::RandHemiSphereSurfacePoint(Vector2 rand)
{
    auto azimuthal = TWOPI * rand.x;

    auto xyproj = std::sqrtf(1 - rand.y * rand.y);
    return Vector3(xyproj * std::cosf(azimuthal), xyproj * std::sinf(azimuthal), rand.y);
}

INLINE Vector3 RandomBase::RandSphereSurfacePointFast(Vector2 rand)
{
    auto azimuthal = TWOPI * rand.x;

    // map [0,1) to [0,1) or [-1,0) with half probability
    auto y = (2.0f * rand.y);
    if (rand.y < 0.5f)
        y += -2.0f; // [1-2) -> [-1,0)

    auto xyproj = std::sqrtf(1 - y * y); // cancels sign change
    return Vector3(xyproj * std::cosf(azimuthal), xyproj * std::sinf(azimuthal), y);
}

INLINE Vector3 RandomBase::RandHemiSphereSurfacePoint(const Vector2& rand, const Vector3& normal)
{
    auto spherePoint = RandSphereSurfacePoint(rand);
    if ((spherePoint | normal) < 0.0f)
        spherePoint = -spherePoint;
    return spherePoint;
}

INLINE Vector3 RandomBase::RandHemiSphereSurfacePoint(const Vector2& rand, const Vector3& center, float radius, const Vector3& normal)
{
    return center + (RandHemiSphereSurfacePoint(rand, normal) * radius);
}

INLINE Vector3 RandomBase::RandTrianglePoint(const Vector2& rand)
{
    auto su0 = std::sqrtf(rand.x);
    auto su1 = rand.y * su0;
    return Vector3(1.0f - su0, su1, su0 - su1);
}

INLINE Vector3 RandomBase::RandTrianglePoint(const Vector2& rand, const Vector3& a, const Vector3& b, const Vector3& c)
{
    auto pos = RandTrianglePoint(rand);
    return (a * pos.x) + (b * pos.y) + (c * pos.z);
}

///--

INLINE Vector2 IRandom::rectPoint(const Vector2& min, const Vector2& max)
{
    return RandRectPoint(unit2(), min, max);
}

INLINE Vector2 IRandom::circlePoint(const Vector2& center, float radius)
{
    return RandCirclePoint(unit2(), center, radius);
}

INLINE Vector3 IRandom::boxPoint(const Vector3& min, const Vector3& max)
{
    return RandBoxPoint(unit3(), min, max);
}

INLINE Vector3 IRandom::boxPoint(const Box& box)
{
    return RandBoxPoint(unit3(), box);
}

INLINE Vector3 IRandom::boxPoint(const OBB& box)
{
    return RandBoxPoint(unit3(), box);
}

INLINE Vector3 IRandom::spherePoint()
{
    return RandSpherePoint(unit3());
}

INLINE Vector3 IRandom::spherePoint(const Vector3& center, float radius)
{
    return RandSpherePoint(unit3(), center, radius);
}

INLINE Vector3 IRandom::spherePoint(const Sphere& sphere)
{
    return RandSpherePoint(unit3(), sphere);
}

INLINE Vector3 IRandom::sphereSurfacePoint()
{
    return RandSphereSurfacePoint(unit2());
}

INLINE Vector3 IRandom::sphereSurfacePoint(const Vector3& center, float radius)
{
    return RandSphereSurfacePoint(unit2(), center, radius);
}

INLINE Vector3 IRandom::sphereSurfacePoint(const Sphere& sphere)
{
    return RandSphereSurfacePoint(unit2(), sphere);
}

INLINE Vector3 IRandom::hemiSphereSurfacePoint()
{
    return RandHemiSphereSurfacePoint(unit2());
}

INLINE Vector3 IRandom::hemiSphereSurfacePoint(const Vector3& N)
{
    return RandHemiSphereSurfacePoint(unit2(), N);
}

INLINE Vector3 IRandom::hemiSphereSurfacePoint(const Vector3& center, float radius, const Vector3& normal)
{
    return RandHemiSphereSurfacePoint(unit2(), center, radius, normal);
}

INLINE Vector3 IRandom::trianglePoint()
{
    return RandHemiSphereSurfacePoint(unit2());
}

INLINE Vector3 IRandom::trianglePoint(const Vector3& a, const Vector3& b, const Vector3& c)
{
    return RandTrianglePoint(unit2(), a, b, c);
}

///--

END_BOOMER_NAMESPACE()
