/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\obb #]
***/

#include "build.h"
#include "obb.h"
#include "mathShapes.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(OBB);
    RTTI_PROPERTY(positionAndLength);
    RTTI_PROPERTY(edge1AndLength);
    RTTI_PROPERTY(edge2AndLength);
RTTI_END_TYPE();

//---

OBB::OBB(const Vector3& pos, const Vector3& edge1, const Vector3& edge2, const Vector3& edge3)
{
    positionAndLength = pos;

    auto len1 = edge1.length();
    edge1AndLength = edge1 / len1;
    edge1AndLength.w = len1;

    auto len2 = edge2.length();
    edge2AndLength = edge2 / len2;
    edge2AndLength.w = len2;

    positionAndLength.w = edge3 | edgeC();
}

OBB::OBB(const Box& box, const Matrix& transform)
{
    BaseTransformation t(transform);

    positionAndLength = t.transformPoint(box.min);

    auto edge1 = t.transformVector(Vector3(box.max.x - box.min.x, 0.0f, 0.0f));
    auto len1 = edge1.length();
    edge1AndLength = edge1 / len1;
    edge1AndLength.w = len1;

    auto edge2 = t.transformVector(Vector3(0.0f, box.max.y - box.min.y, 0.0f));
    auto len2 = edge2.length();
    edge2AndLength = edge2 / len2;
    edge2AndLength.w = len2;

    auto edge3 = t.transformVector(Vector3(0.0f, 0.0f, box.max.z - box.min.z));
    positionAndLength.w = edge3 | edgeC();
}

Vector3 OBB::edgeC() const
{
    return edge1AndLength.xyz() ^ edge2AndLength.xyz();
}

void OBB::corners(Vector3* outCorners) const
{
    auto a = edge1AndLength.xyz() * edge1AndLength.w;
    auto b = edge2AndLength.xyz() * edge2AndLength.w;
    auto c = edgeC() * positionAndLength.w;
    CalcBoxCorners(position(), a, b, c, outCorners);
}

Vector3 OBB::center() const
{
    auto ret = positionAndLength.xyz();
    ret += edge1AndLength.xyz() * edge1AndLength.w * 0.5f;
    ret += edge2AndLength.xyz() * edge2AndLength.w * 0.5f;
    ret += edgeC() * positionAndLength.w * 0.5f;
    return ret;
}

float OBB::volume() const
{
    return edge1AndLength.w * edge2AndLength.w * positionAndLength.w;
}

Box OBB::bounds() const
{
    const auto& a = edge1AndLength.xyz();
    const auto& b = edge2AndLength.xyz();
    auto c  = edgeC();

    Box box;
    box.min = Vector3::ZERO();
    box.max = Vector3::ZERO();
    box.merge(a);
    box.merge(b);
    box.merge(a+b);
    box.merge(c);
    box.merge(a+c);
    box.merge(b+c);
    box.merge(a+b+c);

    box.min += position();
    box.max += position();
    return box;
}

bool OBB::contains(const Vector3& point) const
{
    {
        auto t = point | edge1AndLength.xyz();
        auto d = point | positionAndLength.xyz();
        if (t <= d || t >= d + edge1AndLength.w)
            return false;
    }

    {
        auto t = point | edge2AndLength.xyz();
        auto d = point | positionAndLength.xyz();
        if (t <= d || t >= d + edge2AndLength.w)
            return false;
    }

    {
        auto t = point | edgeC();
        auto d = point | positionAndLength.xyz();
        if (t <= d || t >= d + positionAndLength.w)
            return false;
    }

    return true;
}

bool OBB::intersect(const Vector3& origin, const Vector3& direction, float maxLength /*= VERY_LARGE_FLOAT*/, float* outEnterDistFromOrigin /*= nullptr*/, Vector3* outEntryPoint /*= nullptr*/, Vector3* outEntryNormal /*= nullptr*/) const
{
    auto rel = positionAndLength.xyz() - origin;

    auto edge3 = edgeC();
    Vector3 dir(direction | edge1AndLength.xyz(), direction | edge1AndLength.xyz(), direction | edge3);
    Vector3 ori(rel | edge1AndLength.xyz(), rel | edge1AndLength.xyz(), rel | edge3);

    auto tMin = -VERY_LARGE_FLOAT;
    auto tMax = VERY_LARGE_FLOAT;
    auto axis = -1;
    if (dir.x != 0.0f)
    {
        auto t1 = ori.x / dir.x;
        auto t2 = (ori.x + edge1AndLength.w) / dir.x;
        if (t2 < t1) std::swap(t1, t2);

        tMin = t1;
        tMax = t2;
        axis = 0;
    }
    else if (ori.x <= 0.0f || ori.x >= edge1AndLength.w)
    {
        return false;
    }

    if (dir.y != 0.0f)
    {
        auto t1 = ori.y / dir.y;
        auto t2 = (ori.y + edge2AndLength.w) / dir.y;
        if (t2 < t1) std::swap(t1, t2);

        if (t1 > tMin)
        {
            axis = 1;
            tMin = t1;
        }

        if (t2 < tMax)
            tMax = t2;
    }
    else if (ori.y <= 0.0f || ori.y >= edge2AndLength.w)
    {
        return false;
    }

    if (dir.z != 0.0f)
    {
        auto t1 = ori.z / dir.z;
        auto t2 = (ori.z + positionAndLength.w) / dir.z;
        if (t2 < t1) std::swap(t1, t2);

        if (t1 > tMin)
        {
            axis = 2;
            tMin = t1;
        }

        if (t2 < tMax)
            tMax = t2;
    }
    else if (ori.z <= 0.0f || ori.z >= positionAndLength.w)
        return false;

    if (tMax < 0 || tMax < tMin)
        return false;

    if (tMin >= maxLength)
        return false;

    if (outEnterDistFromOrigin)
        *outEnterDistFromOrigin = tMin;

    if (outEntryPoint)
        *outEntryPoint = origin + (direction * tMin); // TODO: project on box

    if (outEntryNormal)
    {
        if (axis == 0)
            *outEntryNormal = (dir.x < 0.0f) ? edge1AndLength.xyz() : -edge1AndLength.xyz();
        else if (axis == 1)
            *outEntryNormal = (dir.y < 0.0f) ? edge2AndLength.xyz() : -edge1AndLength.xyz();
        else
            *outEntryNormal = (dir.z < 0.0f) ? edge3 : -edge3;
    }

    return true;
}

#if 0
void OBB::render(IShapeRenderer& renderer, ShapeRenderingMode mode /*= ShapeRenderingMode::Solid*/, ShapeRenderingQualityLevel qualityLevel /*= ShapeRenderingQualityLevel::Medium*/) const
{
    Vector3 corners[8];
    calcCorners(corners);

    auto baseVertex = renderer.addVertices(corners, 8);

    if (mode == ShapeRenderingMode::Solid)
    {
        const uint32_t solidIndices[] = {
            0, 1, 2, 1, 3, 2, // top
            5, 4, 6, 5, 6, 7, // bottom
            0, 2, 4, 4, 2, 6, // left
            1, 5, 3, 3, 5, 7, // right
            2, 3, 6, 3, 7, 6, // front
            0, 4, 1, 1, 4, 5 // back
        };

        renderer.addIndices(baseVertex, solidIndices, ARRAY_COUNT(solidIndices));
    }
    else
    {
        const uint32_t wireIndices[] = {
            0, 1, 1, 3, 3, 2, 2, 0, // top
            5, 4, 4, 6, 6, 7, 7, 5, // bottom
            0, 2, 2, 6, 6, 4, 4, 0, // left
            1, 5, 5, 3, 3, 7, 7, 1, // right
            2, 3, 3, 7, 7, 6, 6, 2, // front
            0, 4, 4, 5, 5, 1, 1, 0, // back
        };

        renderer.addIndices(baseVertex, wireIndices, ARRAY_COUNT(wireIndices));
    }
}
#endif

//---

END_BOOMER_NAMESPACE()
