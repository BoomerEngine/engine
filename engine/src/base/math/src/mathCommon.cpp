/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#include "build.h"

namespace base
{
    //--

    TYPE_TLS uint64_t GRandomState = 0;

#ifndef BUILD_AS_LIBS
    float Rand()
    {
        auto val = (uint32_t)StatelessNextUint64(GRandomState) >> 10; // 22 bits
        return (float)val / (float)0x3FFFFF;
    }

    uint8_t RandByte()
    {
        return (uint8_t)StatelessNextUint64(GRandomState);
    }

    uint32_t RandUint32()
    {
        return (uint32_t)StatelessNextUint64(GRandomState);
    }

    uint64_t RandUint64()
    {
        return StatelessNextUint64(GRandomState);
    }
#endif

    //--

    bool GetMostPerpendicularPlane(const Vector3 &forward, const Vector3 &axis, const Vector3 &point, Plane &outPlane)
    {
        auto cross = Cross(forward, axis).normalized();
        auto normal = Cross(cross, axis).normalized();
        outPlane = Plane(normal, point);
        return true;
    }

    bool CalcPlaneRayIntersection(const Vector3& planeNormal, float planeDistance, const Vector3& rayOrigin, const Vector3& rayDir, float rayLength/* = VERY_LARGE_FLOAT*/, float* outDistance, Vector3* outPosition)
    {
        // Calculate the intersection point
        auto distDot = Dot(planeNormal, rayDir);
        if (distDot < 0.0f)
        {
            auto distance = Dot(planeNormal, rayOrigin) + planeDistance;
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
        return CalcPlaneRayIntersection(planeNormal, -Dot(planeNormal, planePoint), rayOrigin, rayDir, rayLength, outDistance, outPosition);
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

    Vector2 NormalPart(const Vector2 &a, const Vector2 &normal)
    {
        return Dot(a,normal) * normal;
    }

    Vector2 TangentPart(const Vector2 &a, const Vector2 &normal)
    {
        return a - (Dot(a,normal) * normal);
    }

    Vector3 NormalPart(const Vector3 &a, const Vector3 &normal)
    {
        return Dot(a,normal) * normal;
    }

    Vector3 TangentPart(const Vector3 &a, const Vector3 &normal)
    {
        return a - (Dot(a,normal) * normal);
    }

    Vector4 NormalPart(const Vector4 &a, const Vector4 &normal)
    {
        return Dot(a,normal) * normal;
    }

    Vector4 TangentPart(const Vector4 &a, const Vector4 &normal)
    {
        return a - (Dot(a,normal) * normal);
    }

    Vector2 ClampLength(const Vector2& a, float maxLength)
    {
        auto len = a.length();
        if (len <= maxLength)
            return a;

        return a * (maxLength / len);
    }

    Vector3 ClampLength(const Vector3& a, float maxLength)
    {
        auto len = a.length();
        if (len <= maxLength)
            return a;

        return a * (maxLength / len);
    }

    Vector4 ClampLength(const Vector4& a, float maxLength)
    {
        auto len = a.length();
        if (len <= maxLength)
            return a;

        return a * (maxLength / len);
    }

    Vector2 SetLength(const Vector2& a, float maxLength)
    {
        auto len = a.length();
        if (len <= SMALL_EPSILON)
            return a;
        return a * (maxLength / len);
    }

    Vector3 SetLength(const Vector3& a, float maxLength)
    {
        auto len = a.length();
        if (len <= SMALL_EPSILON)
            return a;
        return a * (maxLength / len);
    }

    Vector4 SetLength(const Vector4& a, float maxLength)
    {
        auto len = a.length();
        if (len <= SMALL_EPSILON)
            return a;
        return a * (maxLength / len);
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

} // base