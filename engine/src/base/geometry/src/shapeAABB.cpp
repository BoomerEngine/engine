/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#include "build.h"
#include "shapeAABB.h"
#include "shapeConvexHull.h"
#include "shapeUtils.h"

namespace base
{
    namespace shape
    {

        //---

        RTTI_BEGIN_TYPE_CLASS(AABB);
            RTTI_PROPERTY(m_min);
            RTTI_PROPERTY(m_max);
        RTTI_END_TYPE();

        //---

        AABB::AABB(const Box& box)
        {
            m_min = box.min;
            m_max = box.max;
        }

        AABB::AABB(const Vector3& boxMin, const Vector3& boxMax)
            : m_min(boxMin)
            , m_max(boxMax)
        {}

        Vector3 AABB::calcCenter() const
        {
            return Vector3((m_min.x + m_max.x) * 0.5f, (m_min.y + m_max.y) * 0.5f, (m_min.z + m_max.z) * 0.5f);
        }

        Vector3 AABB::calcSize() const
        {
            return m_max - m_min;
        }

        ShapePtr AABB::copy() const
        {
            auto ret = CreateSharedPtr<AABB>();
            ret->m_min = m_min;
            ret->m_max = m_max;
            return ret;
        }

        void AABB::calcCRC(base::CRC64& crc) const
        {
            crc << m_min.x;
            crc << m_min.y;
            crc << m_min.z;
            crc << m_max.x;
            crc << m_max.y;
            crc << m_max.z;
        }

        float AABB::calcVolume() const
        {
            return (m_max.x - m_min.x) * (m_max.y - m_min.y) * (m_max.z - m_min.z);
        }

        void AABB::calcCorners(Vector3* outCorners) const
        {
            CalcAlignedBoxCorners(m_min, m_max, outCorners);
        }

        bool AABB::contains(const Vector3& point) const
        {
            return (point.x >= m_min.x) && (point.y >= m_min.y) && (point.z >= m_min.z) &&
                (point.x <= m_max.x) && (point.y <= m_max.y) && (point.z <= m_max.z);
        }

        static uint8_t CompareLE(const Vector3& a, const Vector3& b)
        {
            return ((a.x <= b.x) ? 1 : 0) | ((a.y <= b.y) ? 2 : 0) | ((a.z <= b.z) ? 4 : 0);
        }

        Box AABB::calcBounds() const
        {
            return Box(m_min, m_max);
        }

        void AABB::buildFromMesh(const ISourceMeshInterface& mesh, float shinkBy /*= 0.0f*/, const Matrix* localToParent /*= nullptr*/)
        {
            Box box;

            mesh.processTriangles([&box, localToParent](const Vector3* vertices, uint32_t chunkIndex)
                                  {
                                      if (localToParent)
                                      {
                                          box.merge(localToParent->transformPoint(vertices[0]));
                                          box.merge(localToParent->transformPoint(vertices[1]));
                                          box.merge(localToParent->transformPoint(vertices[2]));
                                      }
                                      else
                                      {
                                          box.merge(vertices[0]);
                                          box.merge(vertices[1]);
                                          box.merge(vertices[2]);
                                      }
                                  });

            auto validShrink =  std::min<float>(shinkBy, 2.0f * box.extents().maxValue() - 0.01f);
            if (validShrink > 0.0f)
                box.extrude(-validShrink);

            m_min = box.min;
            m_max = box.max;
        }

        bool AABB::intersect(const Vector3& origin, const Vector3& direction, float maxLength /*= VERY_LARGE_FLOAT*/, float* outEnterDistFromOrigin /*= nullptr*/, Vector3* outEntryPoint /*= nullptr*/, Vector3* outEntryNormal /*= nullptr*/) const
        {
            if (outEntryNormal)
            {
                Plane planes[6];
                planes[0].n = -Vector3::EX();
                planes[1].n = Vector3::EX();
                planes[2].n = -Vector3::EY();
                planes[3].n = Vector3::EY();
                planes[4].n = -Vector3::EZ();
                planes[5].n = Vector3::EZ();

                planes[0].d = m_min.x;
                planes[1].d = -m_max.x;
                planes[2].d = m_min.y;
                planes[3].d = -m_max.y;
                planes[4].d = m_min.z;
                planes[5].d = -m_max.z;

                return IntersectPlaneList(planes, 6, origin, direction, maxLength, outEnterDistFromOrigin, outEntryPoint, outEntryNormal);
            }

            // r.dir is unit direction vector of ray
            auto fracX = 1.0f / direction.x;
            auto fracY = 1.0f / direction.y;
            auto fracZ = 1.0f / direction.z;

            // lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
            // r.org is origin of ray
            float t1 = (m_min.x - origin.x)*fracX;
            float t2 = (m_max.x - origin.x)*fracX;
            float t3 = (m_min.y - origin.y)*fracY;
            float t4 = (m_max.y - origin.y)*fracY;
            float t5 = (m_min.z - origin.z)*fracZ;
            float t6 = (m_max.z - origin.z)*fracZ;

            float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
            float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));
            if (tmax < 0)
                return false;

            if (tmin > tmax)
                return false;

            if (tmin > maxLength)
                return false;

            if (outEnterDistFromOrigin)
                *outEnterDistFromOrigin = tmin;

            if (outEntryPoint)
                *outEntryPoint = origin + (tmin * direction); // TODO: project on box

            return true;
        }

        void AABB::render(IShapeRenderer& renderer, ShapeRenderingMode mode /*= ShapeRenderingMode::Solid*/, ShapeRenderingQualityLevel qualityLevel /*= ShapeRenderingQualityLevel::Medium*/) const
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