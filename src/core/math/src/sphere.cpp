/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\sphere #]
***/

#include "build.h"
#include "sphere.h"
#include "mathShapes.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(Sphere);
    RTTI_PROPERTY(positionAndRadius);
RTTI_END_TYPE();

//---

Sphere::Sphere(const Vector3& pos, float radius)
    : positionAndRadius(pos.x, pos.y, pos.z, radius)
{}

float Sphere::volume() const
{
    return positionAndRadius.w * positionAndRadius.w * positionAndRadius.w * (PI * 4.0f / 3.0f);
}

Box Sphere::bounds() const
{
    return Box(position(), radius());
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

    auto a  = (direction | direction);
    auto b  = 2.0f * (vec | direction);
    auto c  = (vec | vec) - (radius() * radius());

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

#if 0
namespace helper
{
    // half sphere builder
    class HalfSphereBuilder : public ISingleton
    {
        DECLARE_SINGLETON(HalfSphereBuilder);

    public:
        struct Verts
        {
            Array<Vector3> top;
            Array<Vector3> bottom;
            Array<Vector3> circle;
        };

        Verts verts[3];

    private:
        HalfSphereBuilder()
        {
            Vector3 points[5];
            points[0] = Vector3( -1,-1,0 ).normalized();
            points[1] = Vector3( 1,-1,0 ).normalized();
            points[2] = Vector3( 1,1,0 ).normalized();
            points[3] = Vector3( -1,1,0 ).normalized();
            points[4] = Vector3( 0,0,1 );

            for (uint32_t i=0; i<3; ++i)
            {
                auto level  = 2 + i;
                auto& data = verts[i];

                buildTri(points[0], points[1], points[4], level, data.top);
                buildTri(points[1], points[2], points[4], level, data.top);
                buildTri(points[2], points[3], points[4], level, data.top);
                buildTri(points[3], points[0], points[4], level, data.top);

                buildTri(points[6],points[5],points[9], level, data.bottom);
                buildTri(points[7],points[6],points[9], level, data.bottom);
                buildTri(points[8],points[7],points[9], level, data.bottom);
                buildTri(points[5],points[8],points[9], level, data.bottom);

                // Create the circumreference vertices
                uint32_t numVertices = 4 << level;
                data.circle.resize(numVertices + 1);
                for (uint32_t i = 0; i <= numVertices; ++i)
                {
                    float angle = TWOPI * ((float) i / (float) numVertices);
                    data.circle[i].x = cosf(angle);
                    data.circle[i].y = sinf(angle);
                    data.circle[i].z = 0.f;
                }
            }
        }

        void buildTri(const Vector3& a, const Vector3& b, const Vector3& c, uint32_t level, Array<Vector3 >& outList)
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
            Vector3 midAB = ((a + b) * 0.5f).normalized();
            Vector3 midBC = ((b + c) * 0.5f).normalized();
            Vector3 midCA = ((c + a) * 0.5f).normalized();

            // recruse
            buildTri(a,midAB,midCA, level-1, outList);
            buildTri(midAB,b,midBC, level-1, outList);
            buildTri(midBC,c,midCA, level-1, outList);
            buildTri(midAB,midBC,midCA, level-1, outList);
        }

        virtual void deinit() override
        {
            for (uint32_t i=0; i<ARRAY_COUNT(verts); ++i)
            {
                verts[i].circle.clear();
                verts[i].top.clear();
                verts[i].bottom.clear();
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
#endif

END_BOOMER_NAMESPACE()
