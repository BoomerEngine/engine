/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#include "build.h"
#include "shapeCylinder.h"
#include "shapeUtils.h"

namespace base
{
    namespace shape
    {
        //---

        RTTI_BEGIN_TYPE_CLASS(Cylinder);
            RTTI_PROPERTY(m_positionAndRadius);
            RTTI_PROPERTY(m_normalAndHeight);
        RTTI_END_TYPE();

        //---

        Cylinder::Cylinder(const Vector3& pos1, const Vector3& pos2, float radius)
            : m_positionAndRadius(pos1.x, pos1.y, pos1.z, radius)
        {
            m_normalAndHeight = (pos2 - pos1).normalized();
            m_normalAndHeight.w = radius;
        }

        Cylinder::Cylinder(const Vector3& pos, const Vector3& normal, float radius, float height)
            : m_positionAndRadius(pos.x, pos.y, pos.z, radius)
            , m_normalAndHeight(normal.x, normal.y, normal.z, height)
        {}

        Vector3 Cylinder::calcCenter() const
        {
            return position() + normal() * (height() * 0.5f);
        }

        Vector3 Cylinder::calcPosition2() const
        {
            return position() + normal() * height();
        }

        void Cylinder::calcBaseVectors(Vector3& outU, Vector3& outV) const
        {
            CalcPerpendicularVectors(normal(), outU, outV);
        }

        float Cylinder::calcVolume() const
        {
            return m_positionAndRadius.w * m_positionAndRadius.w * m_normalAndHeight.w * PI;
        }

        void Cylinder::calcCRC(base::CRC64& crc) const
        {
            crc << m_normalAndHeight.x;
            crc << m_normalAndHeight.y;
            crc << m_normalAndHeight.z;
            crc << m_normalAndHeight.w;
            crc << m_positionAndRadius.x;
            crc << m_positionAndRadius.y;
            crc << m_positionAndRadius.z;
            crc << m_positionAndRadius.w;
        }

        ShapePtr Cylinder::copy() const
        {
            auto ret  = CreateSharedPtr<Cylinder>();
            ret->m_normalAndHeight = m_normalAndHeight;
            ret->m_positionAndRadius = m_positionAndRadius;
            return ret;
        }

        Box Cylinder::calcBounds() const
        {
            auto& pa = position();
            auto pb  = calcPosition2();

            auto a  = pb - pa;
            auto aDot  = Dot(a,a);
            auto ex  = radius() * sqrtf(1.0f - a.x * a.x / aDot);
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

        void Cylinder::buildFromMesh(const ISourceMeshInterface& mesh, uint8_t axis, float shinkBy /*= 0.0f*/, const Matrix* localToParent /*=nullptr*/)
        {
            // TODO
        }

        bool Cylinder::contains(const Vector3& point) const
        {
            auto d = Dot(point, normal());
            auto p = Dot(position(), normal());

            if (d <= p || d >= p + height())
                return false;

            auto t = position() - point;
            auto sqRadius = radius() * radius();
            auto tProj = normal() * Dot(t, normal());
            return t.squareDistance(tProj) < sqRadius;
        }

        bool Cylinder::intersect(const Vector3& origin, const Vector3& direction, float maxLength, float* outEnterDistFromOrigin, Vector3* outEntryPoint, Vector3* outEntryNormal) const
        {
            auto t1 = Cross(normal(), origin - position());
            auto t2 = Cross(normal(), direction);
            if (t1 == t2)
                return false;

            float tMin = 0.0f, tMax = 0.0f;
            auto a = Dot(t2, t2);
            auto b = 2 * Dot(t1, t2);
            auto c = Dot(t1, t1) - (radius() * radius());
            if (!SolveQuadraticEquation(a, b, c, tMin, tMax))
                return false;

            auto o = Dot(position(), normal());
            auto s = Dot(origin, normal());
            auto v = Dot(direction, normal());

            auto tMinOrg = tMin;
            if (v == 0)
            {
                if (s <= o || s >= o + height() )
                    return false;
            }
            else
            {
                auto x1 = (o - s) / v;
                auto x2 = (o + height() - s) / v;
                tMin = std::max<float>(tMin, std::min<float>(x1, x2));
                tMax = std::min<float>(tMax, std::max<float>(x1, x2));
            }

            if (tMin >= tMax || tMax < 0.0f)
                return false;

            if (tMin >= maxLength)
                return false;

            if (outEnterDistFromOrigin)
                *outEnterDistFromOrigin = tMin;

            if (outEntryPoint)
                *outEntryPoint = origin + (direction * tMin);

            if (outEntryNormal)
            {
                auto pos = origin + (direction * tMin);
                auto proj = Dot(pos - position(), normal());
                if (tMin > tMinOrg)
                {
                    if (proj+proj < height())
                        *outEntryNormal = -this->normal();
                    else
                        *outEntryNormal = normal();
                }
                else
                {
                    auto center = position() + proj * normal();
                    *outEntryNormal = (pos - center).normalized();
                }
            }

            return true;
        }

        static const uint32_t MAX_CYL_SIDES = 32;

        static uint32_t GetNumVertices(ShapeRenderingQualityLevel qualityLevel)
        {
            switch (qualityLevel)
            {
                case ShapeRenderingQualityLevel::Low: return MAX_CYL_SIDES / 4;
                case ShapeRenderingQualityLevel::High: return MAX_CYL_SIDES;
            }

            return MAX_CYL_SIDES / 2;
        }

        void Cylinder::render(IShapeRenderer& renderer, ShapeRenderingMode mode /*= ShapeRenderingMode::Solid*/, ShapeRenderingQualityLevel qualityLevel /*= ShapeRenderingQualityLevel::Medium*/) const
        {
            Vector3 circleVertices[MAX_CYL_SIDES+1];

            // calculate vertices
            auto numPoints = GetNumVertices(qualityLevel);
            CalcCircleVertices(position(), normal(), radius(), numPoints, circleVertices);

            // upload bottom
            auto bottomBase = renderer.addVertices(circleVertices, numPoints);

            // move to top
            auto shift = normal() * height();
            for (uint32_t i=0; i<=numPoints; ++i)
                circleVertices[i] += shift;

            // upload top
            auto topBase = renderer.addVertices(circleVertices, numPoints);

            // create sides
            uint32_t prevTop = topBase;
            uint32_t prevBottom = bottomBase;
            uint32_t curTop = topBase + 1;
            uint32_t curBottom = bottomBase + 1;
            if (mode == ShapeRenderingMode::Solid)
            {
                for (uint32_t i = 1; i < numPoints; ++i)
                {
                    uint32_t indices[6];
                    indices[0] = prevBottom;
                    indices[1] = prevTop;
                    indices[2] = curTop;
                    indices[3] = prevBottom;
                    indices[4] = curTop;
                    indices[5] = curBottom;
                    renderer.addIndices(0, indices, 6);

                    prevTop = curTop;
                    prevBottom = curBottom;
                    curTop += 1;
                    curBottom += 1;
                }
            }
            else if (mode == ShapeRenderingMode::Wire)
            {
                for (uint32_t i = 1; i < numPoints; ++i)
                {
                    uint32_t indices[8];
                    indices[0] = prevBottom;
                    indices[1] = prevTop;
                    indices[2] = prevTop;
                    indices[3] = curTop;
                    indices[4] = curTop;
                    indices[5] = curBottom;
                    indices[6] = curBottom;
                    indices[7] = prevBottom;
                    renderer.addIndices(0, indices, 8);

                    prevTop = curTop;
                    prevBottom = curBottom;
                    curTop += 1;
                    curBottom += 1;
                }
            }
        }

        //---

    } // shape
} // base