/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#include "build.h"
#include "shapeSphere.h"
#include "shapeCapsule.h"
#include "shapeUtils.h"

namespace base
{
    namespace shape
    {
        //---

        RTTI_BEGIN_TYPE_CLASS(Sphere);
            RTTI_PROPERTY(m_positionAndRadius);
        RTTI_END_TYPE();

        //---

        Sphere::Sphere(const Vector3& pos, float radius)
            : m_positionAndRadius(pos.x, pos.y, pos.z, radius)
        {}

        float Sphere::calcVolume() const
        {
            return m_positionAndRadius.w * m_positionAndRadius.w * m_positionAndRadius.w * (PI * 4.0f / 3.0f);
        }

        Box Sphere::calcBounds() const
        {
            return Box(position(), radius());
        }

        void Sphere::calcCRC(base::CRC64& crc) const
        {
            crc << m_positionAndRadius.x;
            crc << m_positionAndRadius.y;
            crc << m_positionAndRadius.z;
            crc << m_positionAndRadius.w;
        }

        ShapePtr Sphere::copy() const
        {
            auto ret  = CreateSharedPtr<Sphere>();
            ret->m_positionAndRadius = m_positionAndRadius;
            return ret;
        }

        bool Sphere::contains(const Vector3& point) const
        {
            auto d  = position().squareDistance(point);
            auto sq  = radius() * radius();
            return (d <= sq);
        }

        bool Sphere::intersect(const Vector3& origin, const Vector3& direction, float maxLength, float* outEnterDistFromOrigin, Vector3* outEntryPoint, Vector3* outEntryNormal) const
        {
            auto vec  = origin - position();

            auto a  = Dot(direction, direction);
            auto b  = 2.0f * Dot(vec, direction);
            auto c  = Dot(vec, vec) - (radius() * radius());

            float tMin=0.0f, tMax=0.0f;
            if (!SolveQuadraticEquation(a, b, c, tMin, tMax))
                return false;

            if (tMax < 0.0f)
                return false;

            if (tMin >= maxLength)
                return false;

            if (outEnterDistFromOrigin)
                *outEnterDistFromOrigin = tMin;

            if (outEntryPoint)
                *outEntryPoint = origin + (direction * tMin);

            if (outEntryNormal)
            {
                auto pos  = origin + (direction * tMin);
                *outEntryNormal = (pos - position()).normalized();
            }

            return true;
        }

        namespace helper
        {
            // half sphere builder
            class HalfSphereBuilder : public base::ISingleton
            {
                DECLARE_SINGLETON(HalfSphereBuilder);

            public:
                struct Verts
                {
                    base::Array<base::Vector3> m_top;
                    base::Array<base::Vector3> m_bottom;
                    base::Array<base::Vector3> m_circle;
                };

                Verts m_verts[3];

            private:
                HalfSphereBuilder()
                {
                    base::Vector3 points[5];
                    points[0] = base::Vector3( -1,-1,0 ).normalized();
                    points[1] = base::Vector3( 1,-1,0 ).normalized();
                    points[2] = base::Vector3( 1,1,0 ).normalized();
                    points[3] = base::Vector3( -1,1,0 ).normalized();
                    points[4] = base::Vector3( 0,0,1 );

                    for (uint32_t i=0; i<3; ++i)
                    {
                        auto level  = 2 + i;
                        auto& data = m_verts[i];

                        buildTri(points[0], points[1], points[4], level, data.m_top);
                        buildTri(points[1], points[2], points[4], level, data.m_top);
                        buildTri(points[2], points[3], points[4], level, data.m_top);
                        buildTri(points[3], points[0], points[4], level, data.m_top);

                        buildTri(points[6],points[5],points[9], level, data.m_bottom);
                        buildTri(points[7],points[6],points[9], level, data.m_bottom);
                        buildTri(points[8],points[7],points[9], level, data.m_bottom);
                        buildTri(points[5],points[8],points[9], level, data.m_bottom);

                        // Create the circumreference vertices
                        uint32_t numVertices = 4 << level;
                        data.m_circle.resize(numVertices + 1);
                        for (uint32_t i = 0; i <= numVertices; ++i)
                        {
                            float angle = TWOPI * ((float) i / (float) numVertices);
                            data.m_circle[i].x = cosf(angle);
                            data.m_circle[i].y = sinf(angle);
                            data.m_circle[i].z = 0.f;
                        }
                    }
                }

                void buildTri(const base::Vector3& a, const base::Vector3& b, const base::Vector3& c, uint32_t level, base::Array<base::Vector3 >& outList)
                {
                    // emit the triangle once we've reached the final level (it's over 9000!)
                    if ( level == 0 )
                    {
                        outList.pushBack(a);
                        outList.pushBack(b);
                        outList.pushBack(c);
                        return;
                    }

                    // calculate split points for each triangle and project them on the sphere (normalization)
                    base::Vector3 midAB = ((a + b) * 0.5f).normalized();
                    base::Vector3 midBC = ((b + c) * 0.5f).normalized();
                    base::Vector3 midCA = ((c + a) * 0.5f).normalized();

                    // recruse
                    buildTri(a,midAB,midCA, level-1, outList);
                    buildTri(midAB,b,midBC, level-1, outList);
                    buildTri(midBC,c,midCA, level-1, outList);
                    buildTri(midAB,midBC,midCA, level-1, outList);
                }

                virtual void deinit() override
                {
                    for (uint32_t i=0; i<ARRAY_COUNT(m_verts); ++i)
                    {
                        m_verts[i].m_circle.clear();
                        m_verts[i].m_top.clear();
                        m_verts[i].m_bottom.clear();
                    }
                }
            };

        } // helper

        void Sphere::render(IShapeRenderer& renderer, ShapeRenderingMode mode /*= ShapeRenderingMode::Solid*/, ShapeRenderingQualityLevel qualityLevel /*= ShapeRenderingQualityLevel::Medium*/) const
        {


        }

        void Capsule::render(IShapeRenderer& renderer, ShapeRenderingMode mode /*= ShapeRenderingMode::Solid*/, ShapeRenderingQualityLevel qualityLevel /*= ShapeRenderingQualityLevel::Medium*/) const
        {

        }

        //---

    } // shape
} // base