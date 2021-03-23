/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//--

bool GetMostPerpendicularPlane(const Vector3 &forward, const Vector3 &axis, const Vector3 &point, Plane &outPlane)
{
    auto cross = (forward ^ axis).normalized();
    auto normal = (cross ^ axis).normalized();
    outPlane = Plane(normal, point);
    return true;
}

bool CalcPlaneRayIntersection(const Vector3& planeNormal, float planeDistance, const Vector3& rayOrigin, const Vector3& rayDir, float rayLength/* = VERY_LARGE_FLOAT*/, float* outDistance, Vector3* outPosition)
{
    // Calculate the intersection point
    auto distDot = (planeNormal | rayDir);
    if (distDot < 0.0f)
    {
        auto distance = (planeNormal | rayOrigin) + planeDistance;
        auto realDistance = -distance / distDot;
        if (realDistance < rayLength)
        {
            if (outDistance)
                *outDistance = realDistance;

            if (outPosition)
                *outPosition = rayOrigin + (rayDir * realDistance);

            return true;
        }
    }

    // No intersection
    return false;
}

bool CalcPlaneRayIntersection(const Vector3& planeNormal, const Vector3& planePoint, const Vector3& rayOrigin, const Vector3& rayDir, float rayLength/*= VERY_LARGE_FLOAT*/, float* outDistance/* = nullptr*/, Vector3* outPosition/* = nullptr*/)
{
    return CalcPlaneRayIntersection(planeNormal, -(planeNormal | planePoint), rayOrigin, rayDir, rayLength, outDistance, outPosition);
}

Vector3 TriangleNormal(const Vector3 &a, const Vector3 &b, const Vector3 &c)
{
    return ((b-a) ^ (c-a)).normalized();
}

bool SafeTriangleNormal(const Vector3 &a, const Vector3 &b, const Vector3 &c, Vector3& outN)
{
    auto n = (b-a) ^ (c-a);
    if (n.normalize() <= NORMALIZATION_EPSILON)
        return false;

    outN = n;
    return true;
}

void CalcPerpendicularVectors(const Vector3& dir, Vector3& outU, Vector3& outV)
{
    auto axis = dir.largestAxis();
    auto& ref = (axis == 2) ? Vector3::EX() : Vector3::EZ();

    auto ndir = dir.normalized();

    auto temp = ref ^ ndir;
    outV = temp ^ ndir;
    outU = outV ^ ndir;
}

float CalcDistanceToEdge(const Vector3& point, const Vector3& a, const Vector3 &b, Vector3* outClosestPoint)
{
    auto edge = (b - a).normalized();
    auto ta = edge | a;
    auto tb = edge | b;
    auto p = edge | point;
    if (p >= ta && p <= tb)
    {
        auto projected = a + edge * (p - ta);
        if (outClosestPoint) *outClosestPoint = projected;
        return point.distance(projected);
    }
    else if (p < ta)
    {
        if (outClosestPoint) *outClosestPoint = a;
        return point.distance(a);
    }
    else
    {
        if (outClosestPoint) *outClosestPoint = b;
        return point.distance(b);
    }
}

float AngleDistance(float srcAngle, float srcTarget)
{
    float angle = AngleNormalize(srcAngle);
    float target = AngleNormalize(srcTarget);

    float delta = angle - target;
    if ( delta < -180.0f )
        return delta + 360.0f;
    else if ( delta > 180.0f )
        return delta - 360.0f;
    else
        return delta;
}

float AngleReach(float srcCurrent, float srcTarget, float speed)
{
    float target = AngleNormalize(srcTarget);
    float current = AngleNormalize(srcCurrent);

    // Calculate distance
    float delta = target - current;

    // Get shortest distance
    if (delta < -180.0f)
        delta += 360.0f;
    else if (delta > 180.0f)
        delta -= 360.0f;

    // Move
    if (delta > speed)
        return AngleNormalize(current + speed);
    else if ( delta < -speed )
        return AngleNormalize(current - speed);
    else
        return target;
}

//--

uint32_t NextPow2(uint32_t v)
{
    auto x = v;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

uint64_t NextPow2(uint64_t v)
{
    auto x = v;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return ++x;
}

uint8_t FloorLog2(uint64_t v)
{
    auto n = v;
    auto pos = 0;
    if (n >= 1ULL << 32) { n >>= 32; pos += 32; }
    if (n >= 1U << 16) { n >>= 16; pos += 16; }
    if (n >= 1U << 8) { n >>= 8; pos += 8; }
    if (n >= 1U << 4) { n >>= 4; pos += 4; }
    if (n >= 1U << 2) { n >>= 2; pos += 2; }
    if (n >= 1U << 1) { pos += 1; }
    return pos;
}

//--

END_BOOMER_NAMESPACE()
