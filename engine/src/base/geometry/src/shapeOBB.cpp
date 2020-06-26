/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#include "build.h"
#include "shapeOBB.h"
#include "shapeUtils.h"

namespace base
{
    namespace shape
    {
        //---

        RTTI_BEGIN_TYPE_CLASS(OBB);
            RTTI_PROPERTY(m_position);
            RTTI_PROPERTY(m_edge1);
            RTTI_PROPERTY(m_edge2);
        RTTI_END_TYPE();

        //---

        OBB::OBB(const Vector3& pos, const Vector3& edge1, const Vector3& edge2, const Vector3& edge3)
        {
            m_position = pos;

            auto len1 = edge1.length();
            m_edge1 = edge1 / len1;
            m_edge1.w = len1;

            auto len2 = edge2.length();
            m_edge2 = edge2 / len2;
            m_edge2.w = len2;

            m_position.w = Dot(edge3, calcEdgeC());
        }

        OBB::OBB(const Box& box, const Matrix& transform)
        {
            m_position = transform.transformPoint(box.min);

            auto edge1 = transform.transformVector(Vector3(box.max.x - box.min.x, 0.0f, 0.0f));
            auto len1 = edge1.length();
            m_edge1 = edge1 / len1;
            m_edge1.w = len1;

            auto edge2 = transform.transformVector(Vector3(0.0f, box.max.y - box.min.y, 0.0f));
            auto len2 = edge2.length();
            m_edge2 = edge2 / len2;
            m_edge2.w = len2;

            auto edge3 = transform.transformVector(Vector3(0.0f, 0.0f, box.max.z - box.min.z));
            m_position.w = Dot(edge3, calcEdgeC());
        }

        Vector3 OBB::calcEdgeC() const
        {
            return Cross(m_edge1.xyz(), m_edge2.xyz());
        }

        void OBB::calcCorners(Vector3* outCorners) const
        {
            auto edgeA = this->edgeA() * m_edge1.w;
            auto edgeB = this->edgeB() * m_edge2.w;
            auto edgeC = calcEdgeC() * m_position.w;
            CalcBoxCorners(position(), edgeA, edgeB, edgeC, outCorners);
        }

        Vector3 OBB::calcCenter() const
        {
            auto ret = m_position.xyz();
            ret += m_edge1.xyz() * m_edge1.w * 0.5f;
            ret += m_edge2.xyz() * m_edge2.w * 0.5f;
            ret += calcEdgeC() * m_position.w * 0.5f;
            return ret;
        }

        float OBB::calcVolume() const
        {
            return m_edge1.w * m_edge2.w * m_position.w;
        }

        void OBB::calcCRC(base::CRC64& crc) const
        {
            crc << m_position.x;
            crc << m_position.y;
            crc << m_position.z;
            crc << m_position.w;
            crc << m_edge1.x;
            crc << m_edge1.y;
            crc << m_edge1.z;
            crc << m_edge1.w;
            crc << m_edge2.x;
            crc << m_edge2.y;
            crc << m_edge2.z;
            crc << m_edge2.w;
        }

        ShapePtr OBB::copy() const
        {
            auto ret = CreateSharedPtr<OBB>();
            ret->m_position = m_position;
            ret->m_edge1 = m_edge1;
            ret->m_edge2 = m_edge2;
            return ret;
        }

        Box OBB::calcBounds() const
        {

            auto& a = edgeA();
            auto& b = edgeA();
            auto c  = calcEdgeC();

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
                auto t = Dot(point, m_edge1.xyz());
                auto d = Dot(point, m_position.xyz());
                if (t <= d || t >= d + m_edge1.w)
                    return false;
            }

            {
                auto t = Dot(point, m_edge2.xyz());
                auto d = Dot(point, m_position.xyz());
                if (t <= d || t >= d + m_edge2.w)
                    return false;
            }

            {
                auto t = Dot(point, calcEdgeC());
                auto d = Dot(point, m_position.xyz());
                if (t <= d || t >= d + m_position.w)
                    return false;
            }

            return true;
        }

        bool OBB::intersect(const Vector3& origin, const Vector3& direction, float maxLength /*= VERY_LARGE_FLOAT*/, float* outEnterDistFromOrigin /*= nullptr*/, Vector3* outEntryPoint /*= nullptr*/, Vector3* outEntryNormal /*= nullptr*/) const
        {
            auto rel = m_position.xyz() - origin;

            auto edge3 = calcEdgeC();
            Vector3 dir(Dot(direction, m_edge1.xyz()), Dot(direction, m_edge2.xyz()), Dot(direction, edge3));
            Vector3 ori(Dot(rel, m_edge1.xyz()), Dot(rel, m_edge2.xyz()), Dot(rel, edge3));

            auto tMin = -VERY_LARGE_FLOAT;
            auto tMax = VERY_LARGE_FLOAT;
            auto axis = -1;
            if (dir.x != 0.0f)
            {
                auto t1 = ori.x / dir.x;
                auto t2 = (ori.x + m_edge1.w) / dir.x;
                if (t2 < t1) std::swap(t1, t2);

                tMin = t1;
                tMax = t2;
                axis = 0;
            }
            else if (ori.x <= 0.0f || ori.x >= m_edge1.w)
            {
                return false;
            }

            if (dir.y != 0.0f)
            {
                auto t1 = ori.y / dir.y;
                auto t2 = (ori.y + m_edge2.w) / dir.y;
                if (t2 < t1) std::swap(t1, t2);

                if (t1 > tMin)
                {
                    axis = 1;
                    tMin = t1;
                }

                if (t2 < tMax)
                    tMax = t2;
            }
            else if (ori.y <= 0.0f || ori.y >= m_edge1.w)
            {
                return false;
            }

            if (dir.z != 0.0f)
            {
                auto t1 = ori.z / dir.z;
                auto t2 = (ori.z + m_position.w) / dir.z;
                if (t2 < t1) std::swap(t1, t2);

                if (t1 > tMin)
                {
                    axis = 2;
                    tMin = t1;
                }

                if (t2 < tMax)
                    tMax = t2;
            }
            else if (ori.z <= 0.0f || ori.z >= m_position.w)
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
                    *outEntryNormal = (dir.x < 0.0f) ? m_edge1.xyz() : -m_edge1.xyz();
                else if (axis == 1)
                    *outEntryNormal = (dir.y < 0.0f) ? m_edge2.xyz() : -m_edge2.xyz();
                else
                    *outEntryNormal = (dir.z < 0.0f) ? edge3 : -edge3;
            }

            return true;
        }

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

        //---

    } // shape
} // base