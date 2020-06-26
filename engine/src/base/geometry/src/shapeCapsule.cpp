/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#include "build.h"
#include "shapeCapsule.h"
#include "shapeCylinder.h"
#include "shapeSphere.h"

namespace base
{
    namespace shape
    {
        //---

        RTTI_BEGIN_TYPE_CLASS(Capsule);
            RTTI_PROPERTY(m_positionAndRadius);
            RTTI_PROPERTY(m_normalAndHeight);
        RTTI_END_TYPE();

        //---

        Capsule::Capsule(const Vector3& pos1, const Vector3& pos2, float radius)
            : m_positionAndRadius(pos1.x, pos1.y, pos1.z, radius)
        {
            m_normalAndHeight = (pos2 - pos1).normalized();
            m_normalAndHeight.w = radius;
        }

        Capsule::Capsule(const Vector3& pos, const Vector3& normal, float radius, float height)
            : m_positionAndRadius(pos.x, pos.y, pos.z, radius)
            , m_normalAndHeight(normal.x, normal.y, normal.z, height)
        {}

        Vector3 Capsule::calcCenter() const
        {
            return position() + normal() * (height() * 0.5f);
        }

        Vector3 Capsule::calcPosition2() const
        {
            return position() + normal() * height();
        }

        float Capsule::calcVolume() const
        {
            auto ret = m_positionAndRadius.w * m_positionAndRadius.w * m_normalAndHeight.w * PI;
            ret += m_positionAndRadius.w * m_positionAndRadius.w * m_positionAndRadius.w * (PI * 4.0f / 3.0f);
            return ret;
        }

        Box Capsule::calcBounds() const
        {
            auto& pa = position();
            auto pb = calcPosition2();

            auto a = pb - pa;
            auto aDot = Dot(a,a);
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

        void Capsule::calcCRC(base::CRC64& crc) const
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

        ShapePtr Capsule::copy() const
        {
            auto ret = CreateSharedPtr<Capsule>();
            ret->m_normalAndHeight = m_normalAndHeight;
            ret->m_positionAndRadius = m_positionAndRadius;
            return ret;
        }

        bool Capsule::contains(const Vector3& point) const
        {
            auto d = Dot(point, normal());
            auto p = Dot(position(), normal());

            auto t = position() - point;
            auto sqRadius = radius() * radius();

            if (d >= p &&  d <= p + height())
            {
                auto tProj = normal() * Dot(t, normal());
                if (t.squareDistance(tProj) <= sqRadius)
                    return true;
            }

            if (t.squareLength() <= sqRadius)
                return true;

            auto t2 = calcPosition2() - point;
            if (t2.squareLength() <= sqRadius)
                return true;

            return false;
        }

        bool Capsule::intersect(const Vector3& origin, const Vector3& direction, float maxLength, float* outEnterDistFromOrigin, Vector3* outEntryPoint, Vector3* outEntryNormal) const
        {
            auto dist = maxLength;

            auto ret = Cylinder(position(), normal(), radius(), height()).intersect(origin, direction, dist, &dist, outEntryPoint, outEntryNormal);
            ret |= Sphere(position(), radius()).intersect(origin, direction, dist, &dist, outEntryPoint, outEntryNormal);
            ret |= Sphere(calcPosition2(), radius()).intersect(origin, direction, dist, &dist, outEntryPoint, outEntryNormal);

            if (outEnterDistFromOrigin)
                *outEnterDistFromOrigin = dist;

            return ret;
        }

        //---

    } // shape
} // base