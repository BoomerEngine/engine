/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#include "build.h"
#include "mathRandom.h"
#include "box.h"
#include "sphere.h"
#include "obb.h"

namespace base
{

    //--

    static const double INV_MAX_UINT32 = 1.0 / (double)(uint64_t)(1ULL << 32); // 1/4B

    //--

    FastRandState::FastRandState(uint64_t seed /*= 0*/)
    {
        state = seed;
    }

    uint32_t Rand(FastRandState& state)
    {
        uint64_t z = (state.state + UINT64_C(0x9E3779B97F4A7C15));
        z = (z ^ (z >> 30))* UINT64_C(0xBF58476D1CE4E5B9);
        z = (z ^ (z >> 27))* UINT64_C(0x94D049BB133111EB);
        state.state = z ^ (z >> 31);
        return (uint32_t)state.state;
    }

    //--

#define MT_SIZE 624
#define MT_PERIOD 397
#define MT_DIFF MT_SIZE - MT_PERIOD
#define MT_MAGIC 0x9908b0df

#define M32(x) (0x80000000 & x) // 32nd MSB
#define L31(x) (0x7FFFFFFF & x) // 31 LSBs

#define UNROLL(expr) \
        y = M32(state.MT[i]) | L31(state.MT[i+1]); \
        state.MT[i] = state.MT[expr] ^ (y >> 1) ^ (((int(y) << 31) >> 31) & MT_MAGIC); \
        ++i;

    static void GenerateMTNumbers(MTRandState& state)
    {
       /* uint32_t i = 0;
        uint32_t y;

        while (i < MT_DIFF)
        {
            UNROLL(i + MT_PERIOD);
            UNROLL(i + MT_PERIOD);
        }

        while (i < MT_SIZE - 1)
        {
            UNROLL(i - MT_DIFF);
            UNROLL(i - MT_DIFF);
            UNROLL(i - MT_DIFF);
            UNROLL(i - MT_DIFF);
            UNROLL(i - MT_DIFF);
            UNROLL(i - MT_DIFF);
            UNROLL(i - MT_DIFF);
            UNROLL(i - MT_DIFF);
            UNROLL(i - MT_DIFF);
            UNROLL(i - MT_DIFF);
            UNROLL(i - MT_DIFF);
        }

        {
            y = M32(state.MT[MT_SIZE - 1]) | L31(state.MT[0]);
            state.MT[MT_SIZE - 1] = state.MT[MT_PERIOD - 1] ^ (y >> 1) ^ (((int32_t(y) << 31) >> 31) & MT_MAGIC);
        }

        for (size_t i = 0; i < MT_SIZE; ++i)
        {
            y = state.MT[i];
            y ^= y >> 11;
            y ^= y << 7 & 0x9d2c5680;
            y ^= y << 15 & 0xefc60000;
            y ^= y >> 18;
            state.MT_TEMPERED[i] = y;
        }
        */
        state.index = 0;
    }

    MTRandState::MTRandState(uint32_t seed /*= 0*/)
    {
        MT[0] = seed;
        index = MT_SIZE;

        for (uint32_t i = 1; i < MT_SIZE; ++i)
            MT[i] = 0x6c078965 * (MT[i - 1] ^ MT[i - 1] >> 30) + i;
    }

    uint32_t Rand(MTRandState& state)
    {
        if (state.index == MT_SIZE)
            GenerateMTNumbers(state);

        return state.MT_TEMPERED[state.index++];
    }

    //--

    Vector2 RandRectPoint(const Vector2& rand, const Vector2& min, const Vector2& max)
    {
        return min + rand * (max - min);
    }

    Vector2 RandCirclePoint(const Vector2& rand, const Vector2& center, float radius)
    {
        auto r = std::sqrtf(rand.x);
        auto a = rand.x * TWOPI;
        return Vector2(r * std::cosf(a), r * std::sinf(a));
    }

    Vector3 RandBoxPoint(const Vector3& rand, const Vector3& min, const Vector3& max)
    {
        return min + rand * (max - min);
    }

    Vector3 RandBoxPoint(const Vector3& rand, const Box& box)
    {
        return box.min + rand * box.extents();
    }

    Vector3 RandBoxPoint(const Vector3& rand, const OBB& box)
    {
        Vector3 ret = box.position();
        ret += box.edge1AndLength.xyz() * rand.x * box.edge1AndLength.w;
        ret += box.edge2AndLength.xyz() * rand.y * box.edge2AndLength.w;
        ret += box.edgeC() * rand.z * box.positionAndLength.w;
        return ret;
    }

    Vector3 RandSpherePoint(const Vector3& rand)
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

    Vector3 RandSpherePoint(const Vector3& rand, const Vector3& center, float radius)
    {
        return center + (RandSpherePoint(rand) * radius);
    }

    Vector3 RandSpherePoint(const Vector3& rand, const Sphere& sphere)
    {
        return sphere.position() + (RandSpherePoint(rand) * sphere.radius());
    }

    Vector3 RandSphereSurfacePoint(const Vector2& rand)
    {
        auto theta = TWOPI * rand.x;
        auto phi = std::acosf(1.0f - 2.0f * rand.y);
        return Vector3(std::sinf(phi) * std::cosf(theta), std::sinf(phi) * std::sinf(theta), std::cosf(phi));
    }

    Vector3 RandSphereSurfacePoint(const Vector2& rand, const Vector3& center, float radius)
    {
        return center + (RandSphereSurfacePoint(rand) * radius);
    }

    Vector3 RandSphereSurfacePoint(const Vector2& rand, const Sphere& sphere)
    {
        return sphere.position() + (RandSphereSurfacePoint(rand) * sphere.radius());
    }

    Vector3 RandHemiSphereSurfacePoint(const Vector2& rand)
    {
        auto azimuthal = TWOPI * rand.x;

        auto xyproj = std::sqrtf(1 - rand.y * rand.y);
        return Vector3(xyproj * std::cosf(azimuthal), xyproj * std::sinf(azimuthal), rand.y);
    }

    Vector3 RandHemiSphereSurfacePoint(const Vector2& rand, const Vector3& normal)
    {
        auto spherePoint = RandSphereSurfacePoint(rand);
        if (Dot(spherePoint, normal) < 0.0f)
            spherePoint = -spherePoint;
        return spherePoint;
    }

    Vector3 RandHemiSphereSurfacePoint(const Vector2& rand, const Vector3& center, float radius, const Vector3& normal)
    {
        return center + (RandHemiSphereSurfacePoint(rand, normal) * radius);
    }

    Vector3 RandTrianglePoint(const Vector2& rand)
    {
        auto su0 = std::sqrtf(rand.x);
        auto su1 = rand.y * su0;
        return Vector3(1.0f - su0, su1, su0 - su1);
    }
    
    Vector3 RandTrianglePoint(const Vector2& rand, const Vector3& a, const Vector3& b, const Vector3& c)
    {
        auto pos = RandTrianglePoint(rand);
        return (a * pos.x) + (b * pos.y) + (c * pos.z);
    }

    //--

} // base