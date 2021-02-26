/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\matrix33 #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_STRUCT(Plane);
    RTTI_BIND_NATIVE_COMPARE(Plane);
    RTTI_TYPE_TRAIT().zeroInitializationValid().noConstructor().noDestructor().fastCopyCompare();
    RTTI_PROPERTY(n);
    RTTI_PROPERTY(d);
RTTI_END_TYPE();

//--

PlaneSide Plane::testPoint(const Vector3 &point, float epsilon/*=std::numeric_limits<float>::epsilon()*/) const
{
    auto dist = distance(point);
    if (dist < -epsilon)
        return SIDE_Negative;
    else if (dist > epsilon)
        return SIDE_Positive;
    else
        return SIDE_OnPlane;
}

PlaneSide Plane::testBox(const Box &box, float epsilon/*=std::numeric_limits<float>::epsilon()*/) const
{
    // Calculate box extents
    float minX = (n.x < 0.0f) ? (box.min.x) : (box.max.x);
    float maxX = (n.x < 0.0f) ? (box.max.x) : (box.min.x);
    float minY = (n.y < 0.0f) ? (box.min.y) : (box.max.y);
    float maxY = (n.y < 0.0f) ? (box.max.y) : (box.min.y);
    float minZ = (n.z < 0.0f) ? (box.min.z) : (box.max.z);
    float maxZ = (n.z < 0.0f) ? (box.max.z) : (box.min.z);

    // Calculate distance projection
    float minDist = minX * n.x + minY * n.y + minZ * n.z + d;
    float maxDist = maxX * n.x + maxY * n.y + maxZ * n.z + d;

    // Return state
    int side = 0;
    if (minDist >= epsilon)
        side = SIDE_Positive;
    if (maxDist < -epsilon)
        side |= SIDE_Negative;
    return (PlaneSide)side;
}

bool Plane::contains(const Vector3& point, float epsilon) const
{
    auto dist = Dot(n, point) - d;
    return (dist >= -epsilon) && (dist <= epsilon);
}

bool Plane::intersect(const Vector3& origin, const Vector3& direction, float maxLength /*= VERY_LARGE_FLOAT*/, float* outEnterDistFromOrigin /*= nullptr*/, Vector3* outEntryPoint /*= nullptr*/, Vector3* outEntryNormal /*= nullptr*/) const
{
    auto dot = -Dot(n, direction);
    if (dot > 0.0f)
    {
        auto dist = distance(origin) / dot;
        if (dist >= maxLength)
            return false;

        if (outEnterDistFromOrigin)
            *outEnterDistFromOrigin = dist;

        if (outEntryPoint)
            *outEntryPoint = project(origin + (direction * dist));

        if (outEntryNormal)
            *outEntryNormal = n;

        return true;
    }

    return false;
}

//--

END_BOOMER_NAMESPACE()
