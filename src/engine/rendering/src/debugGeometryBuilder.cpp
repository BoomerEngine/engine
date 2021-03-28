/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#include "build.h"
#include "debugGeometry.h"
#include "debugGeometryBuilder.h"
#include "debugGeometryAssets.h"

#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/commandWriter.h"
#include "engine/atlas/include/dynamicImageAtlas.h"
#include "engine/atlas/include/dynamicGlyphAtlas.h"
#include "../../font/include/font.h"
#include "../../font/include/fontInputText.h"
#include "../../font/include/fontGlyphBuffer.h"

BEGIN_BOOMER_NAMESPACE()

//--

namespace prv
{
    class SphereBuilder : public ISingleton
    {
        DECLARE_SINGLETON(SphereBuilder);

    public:
        Array<Vector3> m_vertices;

        Array<uint16_t> m_solidIndices;
        Array<uint16_t> m_solidIndicesRev;
        Array<uint16_t> m_wireIndices;

        Array<uint16_t> m_boundaryIndices;

        HashMap<uint32_t, uint32_t> m_midPoints;

        static const uint32_t MAX_LEVELS = 3;

        SphereBuilder(uint32_t levels = 3)
        {
            m_vertices.pushBack(Vector3(0, 0, 1));
            m_vertices.pushBack(Vector3(-1, -1, 0).normalized());
            m_vertices.pushBack(Vector3(1, -1, 0).normalized());
            m_vertices.pushBack(Vector3(1, 1, 0).normalized());
            m_vertices.pushBack(Vector3(-1, 1, 0).normalized());

            tesselate(1, 2, 0, 0, true);
            tesselate(2, 3, 0, 0, true);
            tesselate(3, 4, 0, 0, true);
            tesselate(4, 1, 0, 0, true);
        }

        uint32_t mapMidPoint(uint32_t a, uint32_t b)
        {
            if (a > b)
                std::swap(a, b);

            auto key = (a << 16) | b;

            uint32_t id = 0;
            if (m_midPoints.find(key, id))
                return id;

            const auto& va = m_vertices[a];
            const auto& vb = m_vertices[b];

            const auto mid = (va + vb).normalized();

            id = m_vertices.size();
            m_vertices.pushBack(mid);

            m_midPoints[key] = id;
            return id;
        }

        void tesselate(uint32_t a, uint32_t b, uint32_t c, uint32_t level, bool boundary)
        {
            if (level == MAX_LEVELS)
            {
                m_solidIndices.pushBack(a);
                m_solidIndices.pushBack(b);
                m_solidIndices.pushBack(c);

                m_solidIndicesRev.pushBack(c);
                m_solidIndicesRev.pushBack(b);
                m_solidIndicesRev.pushBack(a);

                if (a < b || boundary)
                {
                    m_wireIndices.pushBack(a);
                    m_wireIndices.pushBack(b);
                }

                if (b < c)
                {
                    m_wireIndices.pushBack(b);
                    m_wireIndices.pushBack(c);
                }

                if (c < a)
                {
                    m_wireIndices.pushBack(c);
                    m_wireIndices.pushBack(a);
                }

                if (boundary)
                    m_boundaryIndices.pushBack(a);
            }
            else
            {
                const auto midAB = mapMidPoint(a, b);
                const auto midCA = mapMidPoint(c, a);

                tesselate(a, midAB, midCA, level + 1, boundary);

                const auto midBC = mapMidPoint(b, c);
                tesselate(midAB, b, midBC, level + 1, boundary);
                tesselate(midBC, c, midCA, level + 1, false);
                tesselate(midAB, midBC, midCA, level + 1, false);
            }
        }

        virtual void deinit() override
        {
            m_vertices.clear();
            m_solidIndices.clear();
            m_solidIndicesRev.clear();
            m_wireIndices.clear();
            m_boundaryIndices.clear();
            m_midPoints.clear();
        }
    };

    class CircleBuilder : public ISingleton
    {
        DECLARE_SINGLETON(CircleBuilder);

    public:
        static const uint32_t NUM_POINTS = 36 * 2; // 5 deg per slice

        Vector2 m_points[NUM_POINTS + 1]; // last == first

        CircleBuilder()
        {
            const auto delta = TWOPI / (float)NUM_POINTS;

            for (uint32_t i = 0; i < NUM_POINTS; ++i)
            {
                const auto angle = delta * i;
                m_points[i].x = std::cosf(angle);
                m_points[i].y = std::sinf(angle);
            }

            m_points[NUM_POINTS] = m_points[0];
        }
    };

} // prv

//--

DebugGeometryBuilderBase::DebugGeometryBuilderBase(DebugGeometryLayer layer)
    : m_layer(layer)
{}

void DebugGeometryBuilderBase::image(const DebugGeometryImage* img)
{
    m_imageId = img ? img->id() : 0;
    updateFlags();
}

void DebugGeometryBuilderBase::clear()
{
    m_verticesData.reset();
    m_indicesData.reset();
}

DebugGeometryChunkPtr DebugGeometryBuilderBase::buildChunk(gpu::ImageSampledView* customImageView, bool autoReset)
{
    return nullptr;
}

void DebugGeometryBuilderBase::updateFlags()
{
    m_flags = 0;
    if (m_imageId)
    {
        m_flags |= DebugVertex::FLAG_IMAGE;
        m_flags |= (m_imageId << 16);
    }
}

//--

DebugGeometryBuilder::DebugGeometryBuilder(DebugGeometryLayer layer, const Matrix& localToWorld)
    : DebugGeometryBuilderBase(layer)
    , m_localToWorldTrans(m_localToWorld)
{
    m_localToWorld = localToWorld;
    m_localToWorldSet = localToWorld != Matrix::IDENTITY();

    auto& lineVertex = m_verticesData.emplaceBack();
    lineVertex.f = DebugVertex::FLAG_LINE;

    auto& spriteVertex = m_verticesData.emplaceBack();
    spriteVertex.f = DebugVertex::FLAG_SPRITE;
}

DebugGeometryBuilder::~DebugGeometryBuilder()
{

}

void DebugGeometryBuilder::updateFlags()
{
    DebugGeometryBuilderBase::updateFlags();

    if (m_solidEdges)
        m_flags |= DebugVertex::FLAG_EDGES;
    if (m_solidShading)
        m_flags |= DebugVertex::FLAG_SHADE;
}

void DebugGeometryBuilder::localToWorld(const Matrix& localToWorld)
{
    m_localToWorld = localToWorld;
    m_localToWorldSet = localToWorld != Matrix::IDENTITY();
}

uint32_t DebugGeometryBuilder::appendVertex(const Vector2& pos, float z /*= 0.0f*/, float u /*= 0.0f*/, float v /*= 0.0f*/)
{
    return appendVertex(pos.xyz(z), u, v);
}

uint32_t DebugGeometryBuilder::appendVertex(const Vector3& pos, float u /*= 0.0f*/, float v /*= 0.0f*/)
{
    auto id = m_verticesData.size();

    auto* vt = m_verticesData.allocateUninitialized(1);
    writeVertex(vt, pos, u, v);

    if (m_localToWorldSet)
        vt->p = m_localToWorldTrans.transformPoint(vt->p);

    return id;
}

uint32_t DebugGeometryBuilder::appendVertex(float x, float y, float z, float u /*= 0.0f*/, float v /*= 0.0f*/)
{
    return appendVertex(Vector3(x, y, z), u, v);
}

void DebugGeometryBuilder::appendWire(uint32_t a, uint32_t b)
{
    auto* it = m_indicesData.allocateUninitialized(3);
    *it++ = 0;
    *it++ = a;
    *it++ = b;
}

void DebugGeometryBuilder::appendWireTriangle(uint32_t a, uint32_t b, uint32_t c)
{
    auto* it = m_indicesData.allocateUninitialized(9);
    *it++ = 0;
    *it++ = a;
    *it++ = b;
    *it++ = 0;
    *it++ = b;
    *it++ = c;
    *it++ = 0;
    *it++ = c;
    *it++ = a;
}

void DebugGeometryBuilder::appendWireQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    auto* it = m_indicesData.allocateUninitialized(12);
    *it++ = 0;
    *it++ = a;
    *it++ = b;
    *it++ = 0;
    *it++ = b;
    *it++ = c;
    *it++ = 0;
    *it++ = c;
    *it++ = d;
    *it++ = 0;
    *it++ = d;
    *it++ = a;
}

void DebugGeometryBuilder::appendWirePoly(uint32_t firstPoint, const uint32_t* points, uint32_t numPoints)
{
    if (numPoints >= 2)
    {
        auto* it = m_indicesData.allocateUninitialized(3 * numPoints);

        auto last = numPoints - 1;
        for (uint32_t i = 0; i < numPoints; ++i)
        {
            *it++ = 0;
            *it++ = firstPoint + points[last];
            *it++ = firstPoint + points[i];
            last = i;
        }
    }
}

void DebugGeometryBuilder::appendWirePoly(uint32_t firstPoint, uint32_t numPoints)
{
    if (numPoints >= 2)
    {
        auto* it = m_indicesData.allocateUninitialized(3 * numPoints);

        auto last = numPoints - 1;
        for (uint32_t i = 0; i < numPoints; ++i)
        {
            *it++ = 0;
            *it++ = firstPoint + last;
            *it++ = firstPoint + i;
            last = i;
        }
    }
}

void DebugGeometryBuilder::appendWirePolyWithCenter(uint32_t firstPoint, uint32_t numPoints, uint32_t center)
{
    if (numPoints >= 3)
    {
        auto* it = m_indicesData.allocateUninitialized(6 * numPoints);

        auto last = numPoints - 1;
        for (uint32_t i = 0; i < numPoints; ++i)
        {
            *it++ = 0;
            *it++ = firstPoint + last;
            *it++ = firstPoint + i;

            *it++ = 0;
            *it++ = center;
            *it++ = firstPoint + i;

            last = i;
        }
    }
}

void DebugGeometryBuilder::appendSolidTriangle(uint32_t a, uint32_t b, uint32_t c)
{
    auto* it = m_indicesData.allocateUninitialized(3);
    *it++ = a;
    *it++ = b;
    *it++ = c;
}

void DebugGeometryBuilder::appendSolidQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    auto* it = m_indicesData.allocateUninitialized(6);
    *it++ = a;
    *it++ = b;
    *it++ = c;

    *it++ = a;
    *it++ = c;
    *it++ = d;
}

void DebugGeometryBuilder::appendSolidPoly(uint32_t firstPoint, const uint32_t* points, uint32_t numPoints, bool flip)
{
    if (numPoints >= 3)
    {
        auto* it = m_indicesData.allocateUninitialized(6 * numPoints);

        if (flip)
        {
            for (uint32_t i = 2; i < numPoints; ++i)
            {
                *it++ = firstPoint + points[i];
                *it++ = firstPoint + points[i - 1];
                *it++ = firstPoint + points[0];
            }
        }
        else
        {
            for (uint32_t i = 2; i < numPoints; ++i)
            {
                *it++ = firstPoint + points[0];
                *it++ = firstPoint + points[i - 1];
                *it++ = firstPoint + points[i];
            }
        }
    }
}

void DebugGeometryBuilder::appendSolidPoly(uint32_t firstPoint, uint32_t numPoints, bool flip)
{
    if (numPoints >= 3)
    {
        auto* it = m_indicesData.allocateUninitialized(6 * numPoints);

        if (flip)
        {
            for (uint32_t i = 2; i < numPoints; ++i)
            {
                *it++ = firstPoint + i;
                *it++ = firstPoint + i - 1;
                *it++ = firstPoint + 0;
            }
        }
        else
        {
            for (uint32_t i = 2; i < numPoints; ++i)
            {
                *it++ = firstPoint + 0;
                *it++ = firstPoint + i - 1;
                *it++ = firstPoint + i;
            }
        }
    }
}

void DebugGeometryBuilder::appendSolidPolyWithCenter(uint32_t firstPoint, uint32_t numPoints, uint32_t center, bool flip)
{
    if (numPoints >= 3)
    {
        auto* it = m_indicesData.allocateUninitialized(3 * numPoints);

        if (flip)
        {
            auto last = numPoints - 1;
            for (uint32_t i = 0; i < numPoints; ++i)
            {
                *it++ = firstPoint + i;
                *it++ = firstPoint + last;
                *it++ = center;
                last = i;
            }
        }
        else
        {
            auto last = numPoints - 1;
            for (uint32_t i = 0; i < numPoints; ++i)
            {
                *it++ = center;
                *it++ = firstPoint + last;
                *it++ = firstPoint + i;
                last = i;
            }
        }
    }
}

//--

void DebugGeometryBuilder::wire(const Vector3& start, const Vector3& end)
{
    auto a = appendVertex(start);
    auto b = appendVertex(end);
    appendWire(a, b);
}

void DebugGeometryBuilder::wire(const Vector3* pos, uint32_t count, bool closed)
{
    if (count > 2)
    {
        auto first = m_verticesData.size();

        for (auto i = 0; i < count; ++i)
            appendVertex(pos[i]);

        if (closed)
        {
            auto* it = m_indicesData.allocateUninitialized(3 * count);

            auto last = count - 1;
            for (auto i = 0; i < count; ++i)
            {
                *it++ = 0;
                *it++ = first + last;
                *it++ = first + i;
                last = i;
            }
        }
        else
        {
            auto* it = m_indicesData.allocateUninitialized(3 * (count-1));

            for (auto i = 1; i < count; ++i)
            {
                *it++ = 0;
                *it++ = first + i-1;
                *it++ = first + i;
            }
        }
    }
}

static bool CheckDirAndNormalize(const Vector3& start, const Vector3& end, Vector3& outDir, float& outLength)
{
    const auto dir = end - start;

    const auto sqLength = dir.squareLength();
    if (sqLength < SMALL_EPSILON * SMALL_EPSILON)
        return false;

    outLength = std::sqrtf(sqLength);
    outDir = dir / outLength;
    return true;
}

void DebugGeometryBuilder::wireArrow(const Vector3& start, const Vector3& end, float headPos /*= 0.8f*/, float headSize /*= 0.05f*/)
{
    Vector3 dir;
    float length = 0.0f;
    if (!CheckDirAndNormalize(start, end, dir, length))
        return;

    Vector3 u, v;
    CalcPerpendicularVectors(dir, u, v);

    u *= length * headSize;
    v *= length * headSize;

    auto head = start + dir * (length * headPos);

    const auto base = m_verticesData.size();
    appendVertex(start);
    appendVertex(end);
    appendVertex(head - u - v);
    appendVertex(head + u - v);
    appendVertex(head + u + v);
    appendVertex(head - u + v);

    appendWire(base + 0, base + 1);
    appendWire(base + 1, base + 2);
    appendWire(base + 1, base + 3);
    appendWire(base + 1, base + 4);
    appendWire(base + 1, base + 5);
    appendWire(base + 2, base + 3);
    appendWire(base + 3, base + 4);
    appendWire(base + 4, base + 5);
    appendWire(base + 5, base + 2);
}

void DebugGeometryBuilder::wireAxes(const Vector3& origin, const Vector3& x, const Vector3& y, const Vector3& z, float length /*= 0.5f*/)
{
    auto prevColor = m_color;

    color(Color::RED);
    wireArrow(origin, origin + x * length);
    color(Color::GREEN);
    wireArrow(origin, origin + y * length);
    color(Color::BLUE);
    wireArrow(origin, origin + z * length);

    m_color = prevColor;
}

void DebugGeometryBuilder::wireAxes(const Matrix& space, float length /*= 0.5f*/)
{
    auto o = space.translation();
    auto x = space.column(0).xyz().normalized();
    auto y = space.column(1).xyz().normalized();
    auto z = space.column(2).xyz().normalized();
    wireAxes(o, x, y, z, length);
}

void DebugGeometryBuilder::wireBrackets(const Box& box, float length /*= 0.1f*/)
{
    Vector3 corners[8];
    box.corners(corners);
    wireBrackets(corners, length);
}

void DebugGeometryBuilder::wireBrackets(const Vector3* corners, float length /*= 0.1f*/)
{
    // compute the direction vectors
    auto dirX = corners[1] - corners[0];
    auto dirY = corners[2] - corners[0];
    auto dirZ = corners[4] - corners[0];

    // compute maximum length
    auto maxLength = std::min<float>(dirX.length(), std::min<float>(dirY.length(), dirZ.length()));
    length = std::min<float>(maxLength, length);

    // normalize box directions
    dirX.normalize();
    dirY.normalize();
    dirZ.normalize();
    dirX *= length;
    dirY *= length;
    dirZ *= length;

    // draw the corners
    for (uint32_t i = 0; i < 8; i++)
    {
        auto base = m_verticesData.size();
        appendVertex(corners[i]);
        appendVertex((i & 1) ? (corners[i] - dirX) : (corners[i] + dirX));
        appendVertex((i & 2) ? (corners[i] - dirY) : (corners[i] + dirY));
        appendVertex((i & 4) ? (corners[i] - dirZ) : (corners[i] + dirZ));

        appendWire(base + 0, base + 1);
        appendWire(base + 0, base + 2);
        appendWire(base + 0, base + 3);
    }
}

void DebugGeometryBuilder::wireBox(const Box& box)
{
    Vector3 corners[8];
    box.corners(corners);
    wireBox(corners);
}

void DebugGeometryBuilder::wireBox(const Vector3& boxMin, const Vector3& boxMax)
{
    Vector3 corners[8];
    Box(boxMin, boxMax).corners(corners);
    wireBox(corners);
}

void DebugGeometryBuilder::wireBox(const Vector3* corners)
{
    const auto base = m_verticesData.size();
    for (uint32_t i = 0; i < 8; ++i)
        appendVertex(corners[i]);

    auto* it = m_indicesData.allocateUninitialized(12 * 3);

    const uint16_t indices[] = { 0,4,1,5,2,6,3,7,0,1,1,3,3,2,2,0,4,5,5,7,7,6,6,4 };
    for (uint32_t i = 0; i < ARRAY_COUNT(indices); i += 2)
    {
        *it++ = 0;
        *it++ = indices[i] + base;
        *it++ = indices[i+1] + base;
    }
}

void DebugGeometryBuilder::wireSphere(const Vector3& center, float radius)
{
    if (radius > 0.0f)
    {
        auto a = center;
        a.z -= radius;

        auto b = center;
        b.z += radius;

        wireCapsule(a, b, radius);
    }
}

void DebugGeometryBuilder::wireHemiSphere(const Vector3& center, const Vector3& normal, float radius)
{

}

void DebugGeometryBuilder::wireCapsule(const Vector3& a, const Vector3& b, float radius)
{
    uint32_t vtop = 0, vbottom = 0;
    bool hasCenter = false;
    if (pushCapsule(a, b, radius, vtop, vbottom, hasCenter))
    {
        const auto& data = prv::SphereBuilder::GetInstance();
        pushWireIndices(vtop, data.m_wireIndices.typedData(), data.m_wireIndices.size());
        pushWireIndices(vbottom, data.m_wireIndices.typedData(), data.m_wireIndices.size());

        if (hasCenter)
            pushWireCylinder(vtop, data.m_boundaryIndices.typedData(), vbottom, data.m_boundaryIndices.typedData(), data.m_boundaryIndices.size());
    }
}

void DebugGeometryBuilder::wireCylinder(const Vector3& a, const Vector3& b, float radiusA, float radiusB, bool capA, bool capB)
{
    uint32_t vtop = 0, vbottom = 0;
    uint32_t count = 0;
    bool vtopPoint = false, vbottomPoint = false;

    pushCylinder(a, b, radiusA, radiusB, vtop, vbottom, vtopPoint, vbottomPoint, count);

    if (vtopPoint && vbottomPoint)
        appendWire(vtop, vbottom);
    else if (vtopPoint)
        pushWireCone(vtop, vbottom, count);
    else if (vbottomPoint)
        pushWireCone(vbottom, vtop, count);
    else
        pushWireCylinder(vtop, vbottom, count);

    if (capA && !vtopPoint)
    {
        auto vtopCenter = m_verticesData.size();
        appendVertex(a);
        appendWirePolyWithCenter(vtop, count, vtopCenter);
    }

    if (capB && !vbottomPoint)
    {
        auto vbottomCenter = m_verticesData.size();
        appendVertex(b);
        appendWirePolyWithCenter(vbottom, count, vbottomCenter);
    }
}

void DebugGeometryBuilder::wireGrid(const Vector3& o, const Vector3& u, const Vector3& v, uint32_t count)
{
    if (count >= 2)
    {
        const float step = 1.0f / (count - 1);

        // U dir
        auto vMin = o;
        auto vMax = o + v;
        auto hStep = u * step;
        for (uint32_t i = 0; i < count; ++i)
        {
            auto base = m_verticesData.size();
            appendVertex(vMin + hStep * i);
            appendVertex(vMax + hStep * i);
            appendWire(base + 0, base + 1);
        }

        // V dir
        auto hMin = o;
        auto hMax = o + u;
        auto vStep = v * step;
        for (uint32_t i = 0; i < count; ++i)
        {
            auto base = m_verticesData.size();
            appendVertex(hMin + vStep * i);
            appendVertex(hMax + vStep * i);
            appendWire(base + 0, base + 1);
        }
    }
}

void DebugGeometryBuilder::wireGrid(const Vector3& o, const Vector3& n, float size, uint32_t count)
{
    Vector3 u, v;
    CalcPerpendicularVectors(n, u, v);

    u *= size;
    v *= size;

    Vector3 base = o - (u * 0.5f) - (v * 0.5f);
    wireGrid(base, u, v, count);
}

void DebugGeometryBuilder::wireCircle(const Vector3& center, const Vector3& normal, float radius, bool outlineOnly)
{
    Vector3 u, v;
    CalcPerpendicularVectors(normal, u, v);

    wireEllipse(center, u, v, 0.0f, radius, outlineOnly);
}

void DebugGeometryBuilder::wireCircle(const Vector3& center, const Vector3& normal, float radius, float startAngle, float endAngle, bool outlineOnly)
{
    Vector3 u, v;
    CalcPerpendicularVectors(normal, u, v);

    wireEllipse(center, u, v, 0.0f, radius, startAngle, endAngle, outlineOnly);
}

void DebugGeometryBuilder::wireCircleCut(const Vector3& center, const Vector3& normal, float innerRadius, float outerRadius, bool outlineOnly)
{
    Vector3 u, v;
    CalcPerpendicularVectors(normal, u, v);

    wireEllipse(center, u, v, innerRadius, outerRadius, outlineOnly);
}

void DebugGeometryBuilder::wireCircleCut(const Vector3& center, const Vector3& normal, float innerRadius, float outerRadius, float startAngle, float endAngle, bool outlineOnly)
{
    Vector3 u, v;
    CalcPerpendicularVectors(normal, u, v);

    wireEllipse(center, u, v, innerRadius, outerRadius, startAngle, endAngle, outlineOnly);
}

void DebugGeometryBuilder::wireEllipse(const Vector3& o, const Vector3& u, const Vector3& v, float innerRadius, float outerRadius, bool outlineOnly)
{
    if (innerRadius > outerRadius)
        std::swap(innerRadius, outerRadius);

    if (outerRadius > 0.0f)
    {
        uint32_t outFirst = 0;
        uint32_t outCount = 0;
        pushCircle(o, u * outerRadius, v * outerRadius, outFirst, outCount);

        if (innerRadius == 0.0f)
        {
            if (outlineOnly)
            {
                auto* it = m_indicesData.allocateUninitialized(3 * outCount);

                auto last = outCount - 1;
                for (uint32_t i = 0; i < outCount; ++i)
                {
                    *it++ = 0;
                    *it++ = outFirst + last;
                    *it++ = outFirst + i;
                    last = i;
                }
            }
            else
            {
                auto* it = m_indicesData.allocateUninitialized(6 * outCount);

                auto center = m_verticesData.size();
                appendVertex(o);

                auto last = outCount - 1;
                for (uint32_t i = 0; i < outCount; ++i)
                {
                    *it++ = 0;
                    *it++ = outFirst + last;
                    *it++ = outFirst + i;
                    *it++ = 0;
                    *it++ = center;
                    *it++ = outFirst + i;
                    last = i;
                }
            }
        }
        else
        {
            uint32_t inFirst = 0;
            uint32_t inCount = 0;
            pushCircle(o, u * innerRadius, v * innerRadius, inFirst, inCount);

            if (outlineOnly)
            {
                auto* it = m_indicesData.allocateUninitialized(6 * outCount);

                auto last = outCount - 1;
                for (uint32_t i = 0; i < outCount; ++i)
                {
                    *it++ = 0;
                    *it++ = outFirst + last;
                    *it++ = outFirst + i;
                    *it++ = 0;
                    *it++ = inFirst + last;
                    *it++ = inFirst + i;
                    last = i;
                }
            }
            else
            {
                auto* it = m_indicesData.allocateUninitialized(9 * outCount);

                auto last = outCount - 1;
                for (uint32_t i = 0; i < outCount; ++i)
                {
                    *it++ = 0;
                    *it++ = outFirst + last;
                    *it++ = outFirst + i;
                    *it++ = 0;
                    *it++ = inFirst + last;
                    *it++ = inFirst + i;
                    *it++ = 0;
                    *it++ = outFirst + i;
                    *it++ = inFirst + i;
                    last = i;
                }
            }
        }
    }
}

void DebugGeometryBuilder::wireEllipse(const Vector3& o, const Vector3& u, const Vector3& v, float innerRadius, float outerRadius, float startAngle, float endAngle, bool outlineOnly)
{
    if (innerRadius > outerRadius)
        std::swap(innerRadius, outerRadius);

    if (outerRadius > 0.0f)
    {
        uint32_t outFirst = 0;
        uint32_t outCount = 0;
        pushCircle(o, u * outerRadius, v * outerRadius, startAngle, endAngle, outFirst, outCount);

        if (innerRadius == 0.0f)
        {
            auto center = m_verticesData.size();
            appendVertex(o);

            if (outlineOnly)
            {
                auto* it = m_indicesData.allocateUninitialized(3 + 3 * outCount);

                {
                    *it++ = 0;
                    *it++ = center;
                    *it++ = outFirst;
                }

                for (uint32_t i = 1; i < outCount; ++i)
                {
                    *it++ = 0;
                    *it++ = outFirst + i - 1;
                    *it++ = outFirst + i;
                }

                {
                    *it++ = 0;
                    *it++ = outFirst + outCount - 1;
                    *it++ = center;
                }
            }
            else
            {
                auto* it = m_indicesData.allocateUninitialized(3 + 6 * (outCount - 1));
                
                {
                    *it++ = 0;
                    *it++ = center;
                    *it++ = outFirst;
                }

                for (uint32_t i = 1; i < outCount; ++i)
                {
                    *it++ = 0;
                    *it++ = center;
                    *it++ = outFirst + i;
                    *it++ = 0;
                    *it++ = outFirst + i - 1;
                    *it++ = outFirst + i;
                }
            }
        }
        else
        {
            uint32_t inFirst = 0;
            uint32_t inCount = 0;
            pushCircle(o, u * innerRadius, v * innerRadius, startAngle, endAngle, inFirst, inCount);

            if (outlineOnly)
            {
                auto* it = m_indicesData.allocateUninitialized(6 + 6 * outCount);

                {
                    *it++ = 0;
                    *it++ = inFirst;
                    *it++ = outFirst;
                }

                for (uint32_t i = 1; i < outCount; ++i)
                {
                    *it++ = 0;
                    *it++ = outFirst + i - 1;
                    *it++ = outFirst + i;
                    *it++ = 0;
                    *it++ = inFirst + i - 1;
                    *it++ = inFirst + i;
                }

                {
                    *it++ = 0;
                    *it++ = inFirst + outCount - 1;
                    *it++ = outFirst + outCount - 1;
                }
            }
            else
            {
                auto* it = m_indicesData.allocateUninitialized(3 + 9 * outCount);

                {
                    *it++ = 0;
                    *it++ = inFirst;
                    *it++ = outFirst;
                }

                for (uint32_t i = 1; i < outCount; ++i)
                {
                    *it++ = 0;
                    *it++ = outFirst + i - 1;
                    *it++ = outFirst + i;
                    *it++ = 0;
                    *it++ = inFirst + i - 1;
                    *it++ = inFirst + i;
                    *it++ = 0;
                    *it++ = inFirst + i;
                    *it++ = outFirst + i;
                }
            }
        }
    }
}

//--

void DebugGeometryBuilder::solidBox(const Box& box)
{
    Vector3 corners[8];
    box.corners(corners);
    solidBox(corners);
}

void DebugGeometryBuilder::solidBox(const Vector3& boxMin, const Vector3& boxMax)
{
    Vector3 corners[8];
    Box(boxMin, boxMax).corners(corners);
    solidBox(corners);
}

void DebugGeometryBuilder::solidBox(const Vector3* corners)
{
    const auto base = m_verticesData.size();
    for (uint32_t i = 0; i < 8; ++i)
        appendVertex(corners[i]);

    const uint16_t indices[] = { 2,1,0,2,3,1, 6,4,5,7,6,5, 4,2,0,6,2,4, 3,5,1,7,5,3, 6,3,2,6,7,3,  1,4,0,5,4,1 };

    auto* it = m_indicesData.allocateUninitialized(ARRAY_COUNT(indices));
    for (auto id : indices)
        *it++ = base + id;
}

void DebugGeometryBuilder::solidSphere(const Vector3& center, float radius)
{
    if (radius > 0.0f)
    {
        auto a = center;
        a.z -= radius;

        auto b = center;
        b.z += radius;

        solidCapsule(a, b, radius);
    }
}

void DebugGeometryBuilder::solidHemiSphere(const Vector3& center, const Vector3& normal, float radius)
{

}

void DebugGeometryBuilder::solidCapsule(const Vector3& a, const Vector3& b, float radius)
{
    uint32_t vtop = 0, vbottom = 0;
    bool hasCenter = false;
    if (pushCapsule(a, b, radius, vtop, vbottom, hasCenter))
    {
        const auto& data = prv::SphereBuilder::GetInstance();

        pushSolidIndices(vtop, data.m_solidIndices.typedData(), data.m_solidIndices.size());
        pushSolidIndices(vbottom, data.m_solidIndicesRev.typedData(), data.m_solidIndices.size());

        if (hasCenter)
            pushSolidCylinder(vtop, data.m_boundaryIndices.typedData(), vbottom, data.m_boundaryIndices.typedData(), data.m_boundaryIndices.size());
    }
}

void DebugGeometryBuilder::solidCylinder(const Vector3& a, const Vector3& b, float radiusA, float radiusB, bool capA /*= true*/, bool capB /*= true*/)
{
    uint32_t vtop = 0, vbottom = 0;
    uint32_t count = 0;
    bool vtopPoint = false, vbottomPoint = false;

    pushCylinder(a, b, radiusA, radiusB, vtop, vbottom, vtopPoint, vbottomPoint, count);

    if (vtopPoint && vbottomPoint)
        appendWire(vtop, vbottom);
    else if (vtopPoint)
        pushSolidCone(vtop, vbottom, count, vbottom);
    else if (vbottomPoint)
        pushSolidCone(vbottom, vtop, count, vtop);
    else
        pushSolidCylinder(vtop, vbottom, count, vtop, vbottom, true);

    if (capA && !vtopPoint)
    {
        auto vtopCenter = m_verticesData.size();
        appendVertex(a);
        appendSolidPolyWithCenter(vtop, count, vtopCenter);
    }

    if (capB && !vbottomPoint)
    {
        auto vbottomCenter = m_verticesData.size();
        appendVertex(b);
        appendSolidPolyWithCenter(vbottom, count, vbottomCenter);
    }
}

void DebugGeometryBuilder::solidArrow(const Vector3& start, const Vector3& end, float radius /*= 0.02f*/, float headPos /*= 0.8f*/, float headSizeMult /*= 2.0f*/)
{
    if (radius == 0.0f)
    {
        wireArrow(start, end, headPos);
    }
    else
    {
        Vector3 dir;
        float length = 0.0f;
        if (!CheckDirAndNormalize(start, end, dir, length))
            return;

        const auto size = (radius > 0) ? radius : (length * -radius);

        solidCylinder(start, start + dir * length * headPos, size, size);
        solidCylinder(start + dir * length * headPos, end, headSizeMult*size, 0.0f);
    }
}

void DebugGeometryBuilder::solidCircle(const Vector3& center, const Vector3& normal, float radius)
{
    Vector3 u, v;
    CalcPerpendicularVectors(normal, u, v);

    solidEllipse(center, u, v, 0.0f, radius);
}

void DebugGeometryBuilder::solidCircle(const Vector3& center, const Vector3& normal, float radius, float startAngle, float endAngle)
{
    Vector3 u, v;
    CalcPerpendicularVectors(normal, u, v);

    solidEllipse(center, u, v, 0.0f, radius, startAngle, endAngle);
}

void DebugGeometryBuilder::solidCircleCut(const Vector3& center, const Vector3& normal, float innerRadius, float outerRadius)
{
    Vector3 u, v;
    CalcPerpendicularVectors(normal, u, v);

    solidEllipse(center, u, v, innerRadius, outerRadius);
}

void DebugGeometryBuilder::solidCircleCut(const Vector3& center, const Vector3& normal, float innerRadius, float outerRadius, float startAngle, float endAngle)
{
    Vector3 u, v;
    CalcPerpendicularVectors(normal, u, v);

    solidEllipse(center, u, v, innerRadius, outerRadius, startAngle, endAngle);
}

void DebugGeometryBuilder::solidEllipse(const Vector3& o, const Vector3& u, const Vector3& v, float innerRadius, float outerRadius)
{
    if (innerRadius > outerRadius)
        std::swap(innerRadius, outerRadius);

    if (outerRadius > 0.0f)
    {
        uint32_t outFirst = 0;
        uint32_t outCount = 0;
        pushCircle(o, u * outerRadius, v * outerRadius, outFirst, outCount);

        if (innerRadius == 0.0f)
        {
            auto* it = m_indicesData.allocateUninitialized(3 * outCount);

            auto center = m_verticesData.size();
            appendVertex(o);

            auto last = outCount - 1;
            for (uint32_t i = 0; i < outCount; ++i)
            {
                *it++ = center;
                *it++ = outFirst + last;
                *it++ = outFirst + i;
                last = i;
            }
        }
        else
        {
            uint32_t inFirst = 0;
            uint32_t inCount = 0;
            pushCircle(o, u * innerRadius, v * innerRadius, inFirst, inCount);

            auto* it = m_indicesData.allocateUninitialized(6 * outCount);

            auto last = outCount - 1;
            for (uint32_t i = 0; i < outCount; ++i)
            {
                *it++ = outFirst + last;
                *it++ = outFirst + i;
                *it++ = inFirst + i;

                *it++ = outFirst + last;
                *it++ = inFirst + i;
                *it++ = inFirst + last;
                last = i;
            }
        }
    }
}

void DebugGeometryBuilder::solidEllipse(const Vector3& o, const Vector3& u, const Vector3& v, float innerRadius, float outerRadius, float startAngle, float endAngle)
{
    if (innerRadius > outerRadius)
        std::swap(innerRadius, outerRadius);

    if (outerRadius > 0.0f)
    {
        uint32_t outFirst = 0;
        uint32_t outCount = 0;
        pushCircle(o, u * outerRadius, v * outerRadius, startAngle, endAngle, outFirst, outCount);

        if (innerRadius == 0.0f)
        {
            auto center = m_verticesData.size();
            appendVertex(o);


            auto* it = m_indicesData.allocateUninitialized(3 + (outCount - 1));

            for (uint32_t i = 1; i < outCount; ++i)
            {
                *it++ = center;
                *it++ = outFirst + i - 1;
                *it++ = outFirst + i;
            }
        }
        else
        {
            uint32_t inFirst = 0;
            uint32_t inCount = 0;
            pushCircle(o, u * innerRadius, v * innerRadius, startAngle, endAngle, inFirst, inCount);

            auto* it = m_indicesData.allocateUninitialized(6 * (outCount-1));

            for (uint32_t i = 1; i < outCount; ++i)
            {
                *it++ = outFirst + i - 1;
                *it++ = outFirst + i;
                *it++ = inFirst + i;

                *it++ = outFirst + i - 1;
                *it++ = inFirst + i;
                *it++ = inFirst + i - 1;
            }
        }
    }
}

//--

void DebugGeometryBuilder::sprite(const Vector3& o)
{
    auto base = m_verticesData.size();
    appendVertex(o);

    auto* it = m_indicesData.allocateUninitialized(3);
    *it++ = 1;
    *it++ = base;
    *it++ = base;
}

void DebugGeometryBuilder::sprites(const Vector3* pos, uint32_t count)
{
    auto base = m_verticesData.size();
    for (uint32_t i=0; i<count; ++i)
        appendVertex(pos[i]);

    auto* it = m_indicesData.allocateUninitialized(3 * count);
    for (uint32_t i = 0; i < count; ++i)
    {
        *it++ = 1;
        *it++ = base + i;
        *it++ = base + i;
    }
}

//--

bool DebugGeometryBuilder::pushCapsule(const Vector3& a, const Vector3& b, float radius, uint32_t& outVTop, uint32_t& outVBottom, bool& outHasCenter)
{
    const auto& data = prv::SphereBuilder::GetInstance();

    Vector3 dir;
    float length = 0.0f;
    if (!CheckDirAndNormalize(a, b, dir, length))
        return false;

    Vector3 u, v;
    CalcPerpendicularVectors(dir, u, v);

    u *= radius;
    v *= radius;

    float capHeight = 0.0f;
    float cylHalfHeight = 0.0f;

    const auto halfHeight = length * 0.5f;
    if (halfHeight > radius)
    {
        capHeight = radius;
        cylHalfHeight = halfHeight - radius;
        outHasCenter = true;
    }
    else
    {
        capHeight = halfHeight;
        cylHalfHeight = 0.0f;
        outHasCenter = false;
    }

    // push vertices
    outVTop = m_verticesData.size();
    outVBottom = m_verticesData.size() + data.m_vertices.size();
    {
        auto* vt = m_verticesData.allocateUninitialized(data.m_vertices.size() * 2);
        auto* vb = vt + data.m_vertices.size();

        const auto ot = b - (dir * capHeight);
        const auto ob = a + (dir * capHeight);

        for (const auto& t : data.m_vertices)
        {
            auto dxdy = (t.x * u) + (t.y * v);
            auto dz = dir * (t.z * capHeight);

            writeVertex(vt++, ot + dz + dxdy);
            writeVertex(vb++, ob - dz + dxdy);
        }
    }

    if (m_localToWorldSet)
    {
        auto* v = m_verticesData.typedData() + outVTop;
        auto* ve = m_verticesData.typedData() + m_verticesData.size();
        while (v < ve)
        {
            v->p = m_localToWorldTrans.transformPoint(v->p);
            ++v;
        }
    }

    return true;
}

void DebugGeometryBuilder::pushCylinder(const Vector3& a, const Vector3& b, float radiusA, float radiusB, uint32_t& outVTop, uint32_t& outVBottom, bool& outTPoint, bool& outBPoint, uint32_t& outCount)
{
    Vector3 dir;
    float length = 0.0f;
    if (!CheckDirAndNormalize(a, b, dir, length))
    {
        outVTop = m_verticesData.size();
        outVBottom = m_verticesData.size();
        outTPoint = true;
        outBPoint = true;
        outCount = 0;

        appendVertex(a);
        return;
    }

    Vector3 u, v;
    CalcPerpendicularVectors(dir, u, v);

    const auto& data = prv::SphereBuilder::GetInstance();
    outCount = data.m_boundaryIndices.size();

    // top
    {
        outVTop = m_verticesData.size();
        outTPoint = (radiusA <= 0.0f);

        if (outTPoint)
        {
            auto* vt = m_verticesData.allocateUninitialized(1);
            writeVertex(vt, a);
        }
        else
        {
            auto tu = u * radiusA;
            auto tv = v * radiusA;

            auto* vt = m_verticesData.allocateUninitialized(outCount);
            for (auto id : data.m_boundaryIndices)
            {
                const auto& v = data.m_vertices[id];
                writeVertex(vt++, a + (tu * v.x) + (tv * v.y), v.x, v.y);
            }
        }
    }

    // bottom
    {
        outVBottom = m_verticesData.size();
        outBPoint = (radiusB <= 0.0f);

        if (outBPoint)
        {
            auto* vb = m_verticesData.allocateUninitialized(1);
            writeVertex(vb, b);
        }
        else
        {
            auto bu = u * radiusB;
            auto bv = v * radiusB;

            auto* vb = m_verticesData.allocateUninitialized(outCount);
            for (auto id : data.m_boundaryIndices)
            {
                const auto& v = data.m_vertices[id];
                writeVertex(vb++, b + (bu * v.x) + (bv * v.y), v.x, v.y);
            }
        }
    }

    if (m_localToWorldSet)
    {
        auto* v = m_verticesData.typedData() + outVTop;
        auto* ve = m_verticesData.typedData() + m_verticesData.size();
        while (v < ve)
        {
            v->p = m_localToWorldTrans.transformPoint(v->p);
            ++v;
        }
    }
}

void DebugGeometryBuilder::pushCircle(const Vector3& o, const Vector3& u, const Vector3& v, uint32_t& outFirst, uint32_t& outCount)
{
    const auto& data = prv::CircleBuilder::GetInstance();

    outCount = prv::CircleBuilder::NUM_POINTS;
    outFirst = m_verticesData.size();

    auto* vt = m_verticesData.allocateUninitialized(outCount);
    const auto* sc = data.m_points;

    for (uint32_t i = 0; i < outCount; ++i, ++sc)
        writeVertex(vt++, o + (u * sc->x) + (v * sc->y), sc->x, sc->y);
}

bool DebugGeometryBuilder::pushCircle(const Vector3& o, const Vector3& u, const Vector3& v, float startAngle, float endAngle, uint32_t& outFirst, uint32_t& outCount)
{
    const auto& data = prv::CircleBuilder::GetInstance();

    if (startAngle > endAngle)
        std::swap(startAngle, endAngle);

    if (startAngle == endAngle)
        return false;

    const auto angleToSeg = (float)prv::CircleBuilder::NUM_POINTS / 360.0f;

    // start segment and fraction within in
    float startSegFrac = startAngle * angleToSeg;
    int startSeg = std::floorf(startSegFrac);
    startSegFrac = startSegFrac - startSeg;
    bool hasStartFrac = startSegFrac > 0.01f;
    if (hasStartFrac)
        startSeg += 1;

    // final segment and fraction within it
    float endSegFrac = endAngle * angleToSeg;
    int endSeg = std::floorf(endSegFrac);
    endSegFrac = endSegFrac - endSeg;
    bool hasEndFrac = endSegFrac > 0.01f;

    // make positive
    while (startSeg < 0 || endSeg < 0)
    {
        startSeg += prv::CircleBuilder::NUM_POINTS;
        endSeg += prv::CircleBuilder::NUM_POINTS;
    }

    // count segments
    outCount = std::max<int>(0, 1 + (endSeg - startSeg));
    if (hasStartFrac) outCount += 1;
    if (hasEndFrac) outCount += 1;

    outFirst = m_verticesData.size();
    auto* vt = m_verticesData.allocateUninitialized(outCount);

    // start fraction
    if (hasStartFrac)
    {
        const auto a = data.m_points[(startSeg + prv::CircleBuilder::NUM_POINTS - 1) % prv::CircleBuilder::NUM_POINTS];
        const auto b = data.m_points[startSeg % prv::CircleBuilder::NUM_POINTS];
        const auto p = LinearInterpolation(startSegFrac).lerp(a, b);
        writeVertex(vt++, o + (u * p.x) + (v * p.y), p.x, p.y);
    }

    // segments
    for (int i = startSeg; i <= endSeg; ++i)
    {
        const auto p = data.m_points[i % prv::CircleBuilder::NUM_POINTS];
        writeVertex(vt++, o + (u * p.x) + (v * p.y), p.x, p.y);
    }

    // end fraction
    if (hasEndFrac)
    {
        const auto a = data.m_points[endSeg % prv::CircleBuilder::NUM_POINTS];
        const auto b = data.m_points[(endSeg+1) % prv::CircleBuilder::NUM_POINTS];
        const auto p = LinearInterpolation(endSegFrac).lerp(a, b);
        writeVertex(vt++, o + (u * p.x) + (v * p.y), p.x, p.y);
    }

    return true;
}

void DebugGeometryBuilder::pushSolidIndices(uint32_t first, const uint16_t* indices, const uint32_t count, bool flip)
{
    auto* it = m_indicesData.allocateUninitialized(count);

    if (flip)
    {
        for (auto i = 0; i < count; i += 3)
        {
            *it++ = first + indices[i + 2];
            *it++ = first + indices[i + 1];
            *it++ = first + indices[i + 0];
        }
    }
    else
    {
        for (auto i = 0; i < count; ++i)
            *it++ = first + indices[i];
    }
}

void DebugGeometryBuilder::pushWireIndices(uint32_t first, const uint16_t* indices, const uint32_t count)
{
    auto* it = m_indicesData.allocateUninitialized((count/2) * 3);
    for (auto i = 0; i < count; i += 2)
    {
        *it++ = 0;
        *it++ = first + indices[i + 0];
        *it++ = first + indices[i + 1];
    }
}

void DebugGeometryBuilder::pushWireCylinder(uint32_t tfirst, const uint16_t* tindices, uint32_t bfirst, const uint16_t* bindices, uint32_t count)
{
    if (count > 2)
    {
        auto* it = m_indicesData.allocateUninitialized(count * 9);

        auto last = count - 1;
        for (auto i = 0; i < count; ++i)
        {
            *it++ = 0;
            *it++ = tfirst + tindices[last];
            *it++ = bfirst + bindices[last];
            *it++ = 0;
            *it++ = tfirst + tindices[last];
            *it++ = tfirst + tindices[i];
            *it++ = 0;
            *it++ = bfirst + bindices[last];
            *it++ = bfirst + bindices[i];
            last = i;
        }
    }
}

void DebugGeometryBuilder::pushWireCylinder(uint32_t tfirst, uint32_t bfirst, uint32_t count)
{
    if (count > 2)
    {
        auto* it = m_indicesData.allocateUninitialized(count * 9);

        auto last = count - 1;
        for (auto i = 0; i < count; ++i)
        {
            *it++ = 0;
            *it++ = tfirst + last;
            *it++ = bfirst + last;
            *it++ = 0;
            *it++ = tfirst + last;
            *it++ = tfirst + i;
            *it++ = 0;
            *it++ = bfirst + last;
            *it++ = bfirst + i;
            last = i;
        }
    }
}

void DebugGeometryBuilder::pushWireCone(uint32_t top, uint32_t bfirst, uint32_t count)
{
    if (count > 2)
    {
        auto* it = m_indicesData.allocateUninitialized(count * 6);

        auto last = count - 1;
        for (auto i = 0; i < count; ++i)
        {
            *it++ = 0;
            *it++ = top;
            *it++ = bfirst + last;
            *it++ = 0;
            *it++ = bfirst + last;
            *it++ = bfirst + i;
            last = i;
        }
    }
}

void DebugGeometryBuilder::pushSolidCylinder(uint32_t tfirst, const uint16_t* tindices, uint32_t bfirst, const uint16_t* bindices, uint32_t count, bool flip)
{
    auto* it = m_indicesData.allocateUninitialized(6 * count);

    auto last = count - 1;

    if (flip)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            *it++ = tfirst + tindices[last];
            *it++ = tfirst + tindices[i];
            *it++ = bfirst + bindices[i];

            *it++ = tfirst + tindices[last];
            *it++ = bfirst + bindices[i];
            *it++ = bfirst + bindices[last];

            last = i;
        }
    }
    else
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            *it++ = bfirst + bindices[i];
            *it++ = tfirst + tindices[i];
            *it++ = tfirst + tindices[last];

            *it++ = bfirst + bindices[last];
            *it++ = bfirst + bindices[i];
            *it++ = tfirst + tindices[last];

            last = i;
        }
    }
}

void DebugGeometryBuilder::pushSolidCylinder(uint32_t tfirst, uint32_t bfirst, uint32_t count, uint32_t topCenter, uint32_t bottomCenter, bool flip)
{
    if (count > 2)
    {
        auto* it = m_indicesData.allocateUninitialized(6 * count);

        if (flip)
        {
            auto last = count - 1;
            for (uint32_t i = 0; i < count; ++i)
            {
                *it++ = tfirst + last;
                *it++ = tfirst + i;
                *it++ = bfirst + i;

                *it++ = tfirst + last;
                *it++ = bfirst + i;
                *it++ = bfirst + last;

                last = i;
            }
        }
        else
        {
            auto last = count - 1;
            for (uint32_t i = 0; i < count; ++i)
            {
                *it++ = bfirst + i;
                *it++ = tfirst + i;
                *it++ = tfirst + last;

                *it++ = bfirst + last;
                *it++ = bfirst + i;
                *it++ = tfirst + last;

                last = i;
            }
        }
    }
}

void DebugGeometryBuilder::pushSolidCone(uint32_t top, uint32_t bfirst, uint32_t count, uint32_t centerVertex, bool flip)
{
    if (count > 2)
    {
        auto* it = m_indicesData.allocateUninitialized(3 * count);

        auto last = count - 1;

        if (flip)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                *it++ = bfirst + i;
                *it++ = bfirst + last;
                *it++ = top;
                last = i;
            }
        }
        else
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                *it++ = top;
                *it++ = bfirst + last;
                *it++ = bfirst + i;
                last = i;
            }
        }
    }
}

//--

DebugGeometryBuilderScreen::DebugGeometryBuilderScreen()
    : DebugGeometryBuilderBase(DebugGeometryLayer::Screen)
{
    m_fontStyle.size = 24;
    m_flags |= DebugVertex::FLAG_SCREEN;
    fontStyle(DebugGeometryFontStyle::Mono);
}

void DebugGeometryBuilderScreen::updateFlags()
{
    DebugGeometryBuilderBase::updateFlags();

    m_flags |= DebugVertex::FLAG_SCREEN;
}

void DebugGeometryBuilderScreen::fontStyle(const Font* font)
{
    m_font = font;

    if (!m_font)
        fontStyle(m_fontType);
}

void DebugGeometryBuilderScreen::fontStyle(DebugGeometryFontStyle font)
{
    static const auto monoFontPtr = LoadFontFromDepotPath("/engine/interface/fonts/cour.ttf");
    m_fontType = font;
    m_font = monoFontPtr;
}

uint32_t DebugGeometryBuilderScreen::appendVertex(int x, int y, float u /*= 0.0f*/, float v /*= 0.0f*/)
{
    return appendVertex((float)x + 0.5f, (float)y + 0.5f, u, v);
}

uint32_t DebugGeometryBuilderScreen::appendVertex(float x, float y, float u /*= 0.0f*/, float v /*= 0.0f*/)
{
    auto* vt = m_verticesData.allocateUninitialized(1);
    vt->c = m_color;
    vt->p.x = x;
    vt->p.y = x;
    vt->p.z = 0.5f;
    vt->s = m_selectable;
    vt->t.x = u;
    vt->t.y = v;
    vt->w = m_size;
    vt->f = m_flags;
    return 0;
}

uint32_t DebugGeometryBuilderScreen::appendVertex(const Point& pt, float u /*= 0.0f*/, float v /*= 0.0f*/)
{
    return appendVertex((float)pt.x, (float)pt.y, u, v);
}

uint32_t DebugGeometryBuilderScreen::appendVertex(const Vector2& pt, float u /*= 0.0f*/, float v /*= 0.0f*/)
{
    return appendVertex(pt.x, pt.y, u, v);
}

void DebugGeometryBuilderScreen::rect(const Rect& r, float u0/* = 0.0f*/, float v0 /*= 0.0f*/, float u1 /*= 1.0f*/, float v1 /*= 1.0f*/)
{
    rect2(r.min.x, r.min.y, r.max.x, r.max.y, u0, v0, u1, v1);
}

void DebugGeometryBuilderScreen::rect(const Point& min, const Point& max, float u0/* = 0.0f*/, float v0 /*= 0.0f*/, float u1 /*= 1.0f*/, float v1 /*= 1.0f*/)
{
    rect2(min.x, min.y, max.x, max.y, u0, v0, u1, v1);
}

void DebugGeometryBuilderScreen::rect(int x, int y, int w, int h, float u0/* = 0.0f*/, float v0 /*= 0.0f*/, float u1 /*= 1.0f*/, float v1 /*= 1.0f*/)
{
    rect2(x, y, x + w, y + h, u0, v0, u1, v1);
}

void DebugGeometryBuilderScreen::rect2(int x0, int y0, int x1, int y1, float u0/* = 0.0f*/, float v0 /*= 0.0f*/, float u1 /*= 1.0f*/, float v1 /*= 1.0f*/)
{
    auto b = m_verticesData.size();

    auto* vt = m_verticesData.allocateUninitialized(4);
    writeVertex(vt++, x0 + 0.5f, y0 + 0.5f, u0, v0);
    writeVertex(vt++, x1 + 0.5f, y0 + 0.5f, u1, v0);
    writeVertex(vt++, x1 + 0.5f, y1 + 0.5f, u1, v1);
    writeVertex(vt++, x0 + 0.5f, y1 + 0.5f, u0, v1);

    auto* it = m_indicesData.allocateUninitialized(6);
    *it++ = b + 0;
    *it++ = b + 1;
    *it++ = b + 2;
    *it++ = b + 0;
    *it++ = b + 3;
    *it++ = b + 2;
}

void DebugGeometryBuilderScreen::frame(const Rect& r)
{
    frame2(r.min.x, r.min.y, r.max.x, r.max.y);
}

void DebugGeometryBuilderScreen::frame(const Point& min, const Point& max)
{
    frame2(min.x, min.y, max.x, max.y);
}

void DebugGeometryBuilderScreen::frame(int x, int y, int w, int h)
{
    frame2(x, y, x + w, y + h);
}

void DebugGeometryBuilderScreen::frame2(int x0, int y0, int x1, int y1)
{
    auto b = m_verticesData.size();

    auto color = m_color;
    auto flags = m_flags;
    auto halfWidth = m_size * 0.5f;
    m_flags &= ~DebugVertex::FLAG_IMAGE;
    if (m_size < 1.0f)
    {
        halfWidth = 1.0f;
        m_color.a = (uint8_t)(m_color.a * m_size);
    }

    auto fx0 = x0 + 0.5f;
    auto fy0 = y0 + 0.5f;
    auto fx1 = x1 - 0.5f;
    auto fy1 = y1 - 0.5f;

    auto dx = halfWidth;
    auto dy = halfWidth;

    auto* vt = m_verticesData.allocateUninitialized(8);
    writeVertex(vt++, fx0 - dx, fy0 - dy, 0.0f, 0.0f);
    writeVertex(vt++, fx0 + dx, fy0 + dy, 1.0f, 0.0f);

    writeVertex(vt++, fx1 + dx, fy0 - dy, 0.0f, 0.0f);
    writeVertex(vt++, fx1 - dx, fy0 + dy, 1.0f, 0.0f);

    writeVertex(vt++, fx1 + dx, fy1 + dy, 0.0f, 0.0f);
    writeVertex(vt++, fx1 - dx, fy1 - dy, 1.0f, 0.0f);

    writeVertex(vt++, fx0 - dx, fy1 + dy, 0.0f, 0.0f);
    writeVertex(vt++, fx0 + dx, fy1 - dy, 1.0f, 0.0f);

    uint16_t indices[] = {
        0,1,3, 0,3,2,
        2,3,5, 2,5,4,
        4,5,7, 4,7,6,
        6,7,1, 6,1,0,
    };

    auto* it = m_indicesData.allocateUninitialized(ARRAY_COUNT(indices));
    for (const auto i : indices)
        *it++ = b + (i % 8);

    m_color = color;
    m_flags = flags;
}

void DebugGeometryBuilderScreen::line(const Vector2& a, const Vector2& b)
{
    line(a.x, a.y, b.x, b.y);
}

void DebugGeometryBuilderScreen::line(float x0, float y0, float x1, float y1)
{
    float dx = x1 - x0;
    float dy = y1 - y0;

    float len = sqrtf(dx * dx + dy * dy);
}

void DebugGeometryBuilderScreen::lines(const Vector2* points, uint32_t numPoints, bool closed /*= false*/, Color* colors /*= nullptr*/)
{
    lines((float*)points, numPoints, sizeof(Vector2), closed, colors);
}

struct DebugLineCache : public ISingleton
{
    DECLARE_SINGLETON(DebugLineCache);

public:
    struct Point
    {
        float x, y;

        float pdx = 0.0f, pdy = 0.0f; // normalized direction to PREV point
        float ndx = 0.0f, ndy = 0.0f; // normalized direction to NEXT point

        float tx = 0.0f, ty = 0.0f;
    };

    Array<Point> points;

    DebugLineCache()
    {
        points.reserve(1024);
    }

    static inline const float LINE_SEGMENT_DIST = 0.5f;

    void buildSegments(const float* coords, uint32_t numPoints, uint32_t pointStride, bool closed /*= false*/, float width)
    {
        points.reset();
        points.reserve(numPoints);

        // copy first point always
        float px, py;
        {
            auto& p = points.emplaceBack();
            p.x = coords[0] + 0.5f;
            p.y = coords[1] + 0.5f;
            coords = OffsetPtr(coords, pointStride);
            px = p.x;
            py = p.y;
        }

        // copy next points only if we are far away enough
        for (uint32_t i = 1; i < numPoints; ++i)
        {
            float x = coords[0] + 0.5f;
            float y = coords[1] + 0.5f;
            coords = OffsetPtr(coords, pointStride);

            float dx = x - px;
            float dy = y - py;
            float len = std::sqrtf(dx * dx + dy * dy);
            if (len > LINE_SEGMENT_DIST)
            {
                len = 1.0f / len;
                dx *= len;
                dy *= len;

                points.back().ndx = dx;
                points.back().ndy = dy;

                auto& p = points.emplaceBack();
                p.x = x;
                p.y = y;
                p.pdx = dx;
                p.pdy = dy;

                px = x;
                py = y;
            }
        }

        // closed loop required gluing tangents
        if (closed)
        {
            auto& first = points.front();
            auto& last = points.back();

            float dx = first.x - last.x;
            float dy = first.y - last.y;
            float len = std::sqrtf(dx * dx + dy * dy);
            if (len > LINE_SEGMENT_DIST)
            {
                len = 1.0f / len;
                dx *= len;
                dy *= len;

                first.pdx = dx;
                first.pdy = dy;
                last.ndx = dx;
                last.ndy = dy;
            }
        }

        // calculate actual tangents
        for (auto& p : points)
        {
            auto dx = p.pdx + p.ndx;
            auto dy = p.pdy + p.ndy;

            float len = std::sqrtf(dx * dx + dy * dy);
            if (len > 0.0f)
            {
                len = 1.0f / len;
                dx *= len;
                dy *= len;

                p.ty = -dx * width;
                p.tx = dy * width;
            }
        }
    }

public:
    virtual void deinit() override
    {
        points.clear();
    }
};

void DebugGeometryBuilderScreen::lines(const float* coords, uint32_t numPoints, uint32_t pointStride, bool closed /*= false*/, Color* colors /*= nullptr*/)
{
    if (numPoints < 2)
        return;

    auto b = m_verticesData.size();
    auto bb = b;

    auto color = m_color;
    auto flags = m_flags;
    auto halfWidth = m_size * 0.5f;
    m_flags &= ~DebugVertex::FLAG_IMAGE;
    if (m_size < 1.0f)
    {
        halfWidth = 1.0f;
        m_color.a = (uint8_t)(m_color.a * m_size);
    }

    // build the line segments
    auto& lc = DebugLineCache::GetInstance();
    lc.buildSegments(coords, numPoints, pointStride, closed, m_size);

    // generate vertices
    auto* vt = m_verticesData.allocateUninitialized(2 * lc.points.size());
    {
        for (const auto& p : lc.points)
        {
            writeVertex(vt++, p.x - p.tx, p.y - p.ty, 0.0f, 0.0f);
            writeVertex(vt++, p.x + p.tx, p.y + p.ty, 1.0f, 0.0f);
        }
    }
    
    // generate indices
    {
        auto numSegs = lc.points.size() - 1;
        auto* it = m_indicesData.allocateUninitialized(6 * numSegs);
        for (uint32_t i = 0; i < numSegs; ++i)
        {
            *it++ = b + 0;
            *it++ = b + 1;
            *it++ = b + 3;
            *it++ = b + 0;
            *it++ = b + 3;
            *it++ = b + 2;
            b += 2;
        }
    }

    // close the loop
    if (closed)
    {
        auto* it = m_indicesData.allocateUninitialized(6);
        *it++ = b + 0;
        *it++ = b + 1;
        *it++ = bb + 1;
        *it++ = b + 0;
        *it++ = bb + 1;
        *it++ = bb + 0;
    }

    m_color = color;
    m_flags = flags;
}

void DebugGeometryBuilderScreen::clearText()
{
    m_glyphs.reset();
}

Rect DebugGeometryBuilderScreen::calcTextBounds() const
{
    Rect textBounds;
    m_glyphs.bounds(textBounds);
    return textBounds;
}

void DebugGeometryBuilderScreen::appendText(StringView txt)
{
    if (m_font)
        m_font->renderText(m_fontStyle, txt, m_glyphs, m_color);
}

void DebugGeometryBuilderScreen::appendNewLine()
{
    if (m_font)
    {
        const auto currentDescender = m_font->relativeDescender() * m_fontStyle.size;
        m_glyphs.newLine(currentDescender);
    }
}

Rect DebugGeometryBuilderScreen::renderText(int x, int y, int alignX /*= -1*/, int alignY /*= -1*/)
{
    static InplaceArray<int, 1024> glyphIds;
    glyphIds.reset();

    const auto numGlyphs = m_glyphs.m_glyphs.size();
    GetService<DebugGeometryAssetService>()->glyphAtlas()->mapGlyphs(m_glyphs, glyphIds);

    Rect textBounds;
    if (m_glyphs.bounds(textBounds))
    {
        x -= textBounds.min.x;
        y -= textBounds.min.y;
            
        if (alignX == 0)
            x -= textBounds.width() / 2;
        else if (alignX > 0)
            x -= textBounds.width();

        if (alignY == 0)
            y -= textBounds.height() / 2;
        else if (alignY > 0)
            y -= textBounds.height();
    }

    m_verticesData.reserve(m_verticesData.size() + numGlyphs * 4);
    m_indicesData.reserve(m_indicesData.size() + numGlyphs * 6);

    const auto flags = DebugVertex::FLAG_SCREEN | DebugVertex::FLAG_GLYPH;

    for (auto i : m_glyphs.m_glyphs.indexRange())
    {
        const auto& g = m_glyphs.m_glyphs[i];

        auto glyphId = glyphIds[i];
        if (glyphId)
        {
            float sizeX = g.glyph->size().x;
            float sizeY = g.glyph->size().y;
            float offsetX = x + g.pos.x + 0.5f;
            float offsetY = y + g.pos.y + 0.5f;
            float endX = offsetX + sizeX;
            float endY = offsetY + sizeY;

            // create vertices
            auto b = m_verticesData.size();
            {
                auto* vt = m_verticesData.allocateUninitialized(4);
                vt->c = g.color;
                vt->f = flags | (glyphId << 16);
                vt->s = m_selectable;
                vt->w = 1.0f;
                vt->p.x = offsetX;
                vt->p.y = offsetY;
                vt->p.z = 1.0f;
                vt->t.x = 0.0f;
                vt->t.y = 0.0f;
                vt++;

                vt->c = g.color;
                vt->f = flags | (glyphId << 16);
                vt->s = m_selectable;
                vt->w = 1.0f;
                vt->p.x = endX;
                vt->p.y = offsetY;
                vt->p.z = 1.0f;
                vt->t.x = 1.0f;
                vt->t.y = 0.0f;
                vt++;

                vt->c = g.color;
                vt->f = flags | (glyphId << 16);
                vt->s = m_selectable;
                vt->w = 1.0f;
                vt->p.x = endX;
                vt->p.y = endY;
                vt->p.z = 1.0f;
                vt->t.x = 1.0f;
                vt->t.y = 1.0f;
                vt++;

                vt->c = g.color;
                vt->f = flags | (glyphId << 16);
                vt->s = m_selectable;
                vt->w = 1.0f;
                vt->p.x = offsetX;
                vt->p.y = endY;
                vt->p.z = 1.0f;
                vt->t.x = 0.0f;
                vt->t.y = 1.0f;
                vt++;
            }

            // create indices
            {
                auto* it = m_indicesData.allocateUninitialized(6);
                *it++ = b + 0;
                *it++ = b + 1;
                *it++ = b + 2;
                *it++ = b + 0;
                *it++ = b + 2;
                *it++ = b + 3;
            }
        }
    }

    if (!textBounds.empty())
    {
        textBounds.min.x += x;
        textBounds.max.x += x;
        textBounds.min.y += y;
        textBounds.max.y += y;
    }

    m_glyphs.reset();

    return textBounds;
}

void DebugGeometryBuilderScreen::shift(float dx, float dy)
{
    for (auto& p : m_verticesData)
    {
        p.p.x += dx;
        p.p.y += dy;
    }
}

Rect DebugGeometryBuilderScreen::calcBounds() const
{
    if (m_verticesData.empty())
        return Rect();

    float minX = m_verticesData.front().p.x;
    float maxX = m_verticesData.front().p.x;
    float minY = m_verticesData.front().p.y;
    float maxY = m_verticesData.front().p.y;

    for (const auto v : m_verticesData)
    {
        minX = std::min<float>(minX, v.p.x);
        minY = std::min<float>(minY, v.p.y);
        maxX = std::max<float>(maxX, v.p.x);
        maxY = std::max<float>(maxY, v.p.y);
    }

    int localMinX = (int)std::floorf(minX);
    int localMinY = (int)std::floorf(minY);
    int localMaxX = (int)std::ceilf(maxX);
    int localMaxY = (int)std::ceilf(maxY);

    return Rect(localMinX, localMinY, localMaxX, localMaxY);
}

Point DebugGeometryBuilderScreen::shiftToAlign(const Rect& bounds, int alignX /*= 0*/, int alignY /*= 0*/)
{
    const auto localBounds = calcBounds();
    if (localBounds.empty())
        return Point(0, 0);

    if (bounds.empty())
        return localBounds.size();

    int dx = 0;
    if (alignX < 0)
        dx = bounds.min.x - localBounds.min.x;
    else if (alignX == 0)
        dx = bounds.min.x - localBounds.min.x + (localBounds.width() - bounds.width()) / 2;
    else
        dx = bounds.max.x - localBounds.max.x;

    int dy = 0;
    if (alignY < 0)
        dy = bounds.min.y - localBounds.min.y;
    else if (alignY == 0)
        dy = bounds.min.y - localBounds.min.y + (localBounds.height() - bounds.height()) / 2;
    else
        dy = bounds.max.y - localBounds.max.y;

    if (dx || dy)
    {
        float fdx = (float)dx;
        float fdy = (float)dy;

        for (auto& p : m_verticesData)
        {
            p.p.x += fdx;
            p.p.y += fdy;
        }
    }

    return localBounds.size();
}

//--

END_BOOMER_NAMESPACE()
