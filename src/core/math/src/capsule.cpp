/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\capsule #]
***/

#include "build.h"
#include "capsule.h"
#include "cylinder.h"
#include "sphere.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(Capsule);
    RTTI_PROPERTY(positionAndRadius);
    RTTI_PROPERTY(normalAndHeight);
RTTI_END_TYPE();

//---

Capsule::Capsule(const Vector3& pos1, const Vector3& pos2, float radius)
    : positionAndRadius(pos1.x, pos1.y, pos1.z, radius)
{
    normalAndHeight = (pos2 - pos1).normalized();
    normalAndHeight.w = radius;
}

Capsule::Capsule(const Vector3& pos, const Vector3& normal, float radius, float height)
    : positionAndRadius(pos.x, pos.y, pos.z, radius)
    , normalAndHeight(normal.x, normal.y, normal.z, height)
{}

Vector3 Capsule::center() const
{
    return position() + normal() * (height() * 0.5f);
}

Vector3 Capsule::position2() const
{
    return position() + normal() * height();
}

float Capsule::volume() const
{
    auto ret = positionAndRadius.w * positionAndRadius.w * normalAndHeight.w * PI;
    ret += positionAndRadius.w * positionAndRadius.w * positionAndRadius.w * (PI * 4.0f / 3.0f);
    return ret;
}

Box Capsule::bounds() const
{
    auto& pa = position();
    auto pb = position2();

    auto a = pb - pa;
    auto aDot = a | a;
    auto ex = radius() * sqrtf(1.0f - a.x * a.x / aDot);
    auto ey = radius() * sqrtf(1.0f - a.y * a.y / aDot);
    auto ez = radius() * sqrtf(1.0f - a.z * a.z / aDot);

    Box box;
    box.min.x = std::min<float>(pa.x - ex, pb.x - ex);
    box.min.y = std::min<float>(pa.y - ey, pb.y - ey);
    box.min.z = std::min<float>(pa.z - ez, pb.z - ez);
    box.max.x = std::max<float>(pa.x + ex, pb.x + ex);
    box.max.y = std::max<float>(pa.y + ey, pb.y + ey);
    box.max.z = std::max<float>(pa.z + ez, pb.z + ez);
    return box;
}

bool Capsule::contains(const Vector3& point) const
{
    auto d = point | normal();
    auto p = position() | normal();

    auto t = position() - point;
    auto sqRadius = radius() * radius();

    if (d >= p &&  d <= p + height())
    {
        auto tProj = normal() * (t | normal());
        if (t.squareDistance(tProj) <= sqRadius)
            return true;
    }

    if (t.squareLength() <= sqRadius)
        return true;

    auto t2 = position2() - point;
    if (t2.squareLength() <= sqRadius)
        return true;

    return false;
}

bool Capsule::intersect(const Vector3& origin, const Vector3& direction, float maxLength, float* outEnterDistFromOrigin, Vector3* outEntryPoint, Vector3* outEntryNormal) const
{
    auto dist = maxLength;
    auto ret = false;

    if (height() > 0.0f)
    {
        ret |= Cylinder(position(), normal(), radius(), height()).intersect(origin, direction, dist, &dist, outEntryPoint, outEntryNormal);
        ret |= Sphere(position(), radius()).intersect(origin, direction, dist, &dist, outEntryPoint, outEntryNormal);
        ret |= Sphere(position2(), radius()).intersect(origin, direction, dist, &dist, outEntryPoint, outEntryNormal);
    }
    else
    {
        ret |= Sphere(center(), radius()).intersect(origin, direction, dist, &dist, outEntryPoint, outEntryNormal);
    }

    if (outEnterDistFromOrigin)
        *outEnterDistFromOrigin = dist;

    return ret;
}

//---

END_BOOMER_NAMESPACE()
