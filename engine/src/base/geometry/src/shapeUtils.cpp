/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#include "build.h"
#include "shapeUtils.h"

namespace base
{
    namespace shape
    {

        //---

        bool SolveQuadraticEquation(float a, float b, float c, float& x1, float& x2)
        {
            auto discrim = b * b - 4.0f * a * c;
            if (discrim <= 0.0f)
                return false;

            auto rootDiscrim = sqrtf(discrim);
            auto q = (b < 0.0f) ? -0.5f * (b - rootDiscrim) : -0.5f * (b + rootDiscrim);

            x1 = q / a;
            x2 = c / q;

            if (x1 > x2) std::swap(x1, x2);
            return true;
        }

        //---

        void TranslatePlane(Plane& plane, const Vector3& offset)
        {
            plane.d = -Dot(offset + (plane.n * -plane.d), plane.n);
        }

        void TransformPlane(Plane& plane, const Matrix& transform)
        {
            auto pointOnPlane = transform.transformPoint(plane.n * -plane.d);
            auto pointInFront = transform.transformPoint(pointOnPlane + plane.n);

            auto newNormal = (pointOnPlane - pointInFront).normalized();
            plane = Plane(pointOnPlane, newNormal);
        }

        void TransformPlanes(Plane* planes, uint32_t numPlanes, const Matrix& matrix)
        {
            auto det = matrix.det3();
            if (!IsNearZero(det))
            {
                for (uint32_t i=0; i<numPlanes; ++i)
                {
                    TransformPlane(planes[i], matrix);

                    if (det < 0)
                        planes[i].flip();
                }
            }
        }

        void CalcBoxCorners(const Vector3& pos, const Vector3& edgeX, const Vector3& edgeY, const Vector3& edgeZ, Vector3* outVertices)
        {
            outVertices[0] = pos;
            outVertices[1] = pos + edgeX;
            outVertices[2] = pos + edgeY;
            outVertices[3] = pos + edgeX + edgeY;
            outVertices[4] = pos + edgeZ;
            outVertices[5] = pos + edgeX + edgeZ;
            outVertices[6] = pos + edgeY + edgeZ;
            outVertices[7] = pos + edgeX + edgeY + edgeZ;
        }

        void CalcAlignedBoxCorners(const Vector3& boxMin, const Vector3& boxMax, Vector3* outVertices)
        {
           outVertices[0] = Vector3(boxMin.x, boxMin.y, boxMin.z);
           outVertices[1] = Vector3(boxMax.x, boxMin.y, boxMin.z);
           outVertices[2] = Vector3(boxMin.x, boxMax.y, boxMin.z);
           outVertices[3] = Vector3(boxMax.x, boxMax.y, boxMin.z);
           outVertices[4] = Vector3(boxMin.x, boxMin.y, boxMax.z);
           outVertices[5] = Vector3(boxMax.x, boxMin.y, boxMax.z);
           outVertices[6] = Vector3(boxMin.x, boxMax.y, boxMax.z);
           outVertices[7] = Vector3(boxMax.x, boxMax.y, boxMax.z);
        }

        void CalcCircleVertices(const Vector3& origin, const Vector3& normal, float radius, uint32_t numPoints, Vector3* outVertices)
        {
            Vector3 u, v;
            CalcPerpendicularVectors(normal, u, v);

            u *= radius;
            v *= radius;

            CalcCircleVertices(origin, u, v, numPoints, outVertices);
        }

        void CalcCircleVertices(const Vector3& origin, const Vector3& u, const Vector3& v, uint32_t numPoints, Vector3* outVertices)
        {
            auto div = numPoints ? TWOPI / (float)numPoints : 0.0f;
            auto pos = 0.0f;
            auto ptr  = outVertices;

            for (uint32_t i=0; i<numPoints; ++i, pos += div, ++ptr)
            {
                auto s = sinf(pos);
                auto c = cosf(pos);

                ptr->x = origin.x + (c * u.x) + (s * v.x);
                ptr->y = origin.y + (c * u.y) + (s * v.y);
                ptr->z = origin.z + (c * u.z) + (s * v.z);
            }

            if (numPoints > 0)
                outVertices[numPoints] = outVertices[0];
        }

        bool PointInPlaneList(const Plane* planes, uint32_t numPlanes, const Vector3& point)
        {
            if (numPlanes < 4)
                return false;

            for (uint32_t i=0; i<numPlanes; ++i, ++planes)
                if (Dot(point, planes->n) + planes->d > 0.0f)
                    return false;

            return true;
        }

        bool IntersectPlaneList(const Plane* planes, uint32_t numPlanes, const Vector3& origin, const Vector3& direction, float maxLength /*= VERY_LARGE_FLOAT*/, float* outEnterDistFromOrigin /*= nullptr*/, Vector3* outEntryPoint /*= nullptr*/, Vector3* outEntryNormal /*= nullptr*/)
        {
            auto enterDist = -VERY_LARGE_FLOAT;
            auto leaveDist = VERY_LARGE_FLOAT;

            if (numPlanes < 4)
                return false;

            const Plane* lastPlane = nullptr;
            for (uint32_t i=0; i<numPlanes; ++i)
            {
                auto& plane = *planes++;

                auto proj = -Dot(plane.n, direction);
                auto dist = Dot(origin, plane.n) + plane.d;
                if (proj == 0.0f)
                {
                    if (dist < 0.0f)
                        return false;
                    else
                        continue;
                }
                else
                {
                    auto intersectionDistance = dist / proj;
                    if (proj > 0.0f)
                    {
                        if (intersectionDistance > enterDist)
                        {
                            enterDist = intersectionDistance;
                            lastPlane = &plane;

                            // to far
                            if (enterDist >= maxLength)
                                return false;

                            // not intersecting
                            if (enterDist > leaveDist)
                                return false;
                        }
                    }
                    else if (proj < 0.0f)
                    {
                        if (intersectionDistance < leaveDist)
                        {
                            leaveDist = intersectionDistance;

                            // not intersecting
                            if (enterDist > leaveDist)
                                return false;
                        }
                    }
                }
            }

            if (lastPlane == nullptr)
                return false;

            if (outEnterDistFromOrigin)
                *outEnterDistFromOrigin = enterDist;

            if (outEntryPoint)
                *outEntryPoint = origin + (direction * enterDist);

            if (outEntryNormal)
                *outEntryNormal = lastPlane->n;

            return true;
        }

        auto TRIANGLE_RAY_EPSILON = std::numeric_limits<float>::epsilon() * std::numeric_limits<float>::epsilon();

        // Fast Minimum Storage Ray/Triangle Intersection
        // Tomas Moller, Ben Trumbbore
        // https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
        bool IntersectTriangleRay(const Vector3& origin, const Vector3& dir, const Vector3& v0, const Vector3& v1, const Vector3& v2, float maxDist, float* outDist, float* outU, float* outV, bool cull, float additionalEpsilon)
        {
            // Find vectors for two edges sharing v0
            auto edge1 = v1 - v0;
            auto edge2 = v2 - v0;

            // Begin calculating determinant - also used to calculate U parameter
            auto pvec = Cross(dir, edge2);

            // If determinant is near zero, ray lies in plane of triangle
            auto det = Dot(edge1, pvec);

            // Cull/NoCull
            if (cull)
            {
                // Prevent division by zero and cull
                if (det < TRIANGLE_RAY_EPSILON)
                    return false;

                // Calculate distance from v0 to ray origin
                auto tvec = origin - v0;

                // Calculate U parameter and test bounds
                auto u = Dot(tvec, pvec);

                // Compute the UV limits
                auto enlargeCoeff = additionalEpsilon * det;
                auto uvlimit = -enlargeCoeff;
                auto uvlimit2 = det + enlargeCoeff;
                if (u<uvlimit || u>uvlimit2)
                    return false;

                // Prepare to test V parameter
                auto qvec = Cross(tvec, edge1);

                // Calculate V parameter and test bounds
                auto v = Dot(dir, qvec);
                if (v<uvlimit || (u+v)>uvlimit2)
                    return false;

                // Calculate t, scale parameters, ray intersects triangle
                auto t = Dot(edge2, qvec);
                if (t >= maxDist * det)
                    return false;

                // Output params
                if (outDist || outU || outV)
                {
                    auto inv_det = 1.0f / det;
                    auto dist = t * inv_det;
                    if (dist >= maxDist)
                        return false;

                    if (outDist)
                        *outDist = dist;

                    if (outU)
                        *outU = u * inv_det;

                    if (outV)
                        *outV = v * inv_det;
                }
            }
            else
            {
                // Prevent division by zero and cull
                if (fabsf(det) < TRIANGLE_RAY_EPSILON)
                    return false;

                auto inv_det = 1.0f / det;

                // Calculate distance from v0 to ray origin
                Vector3 tvec = origin - v0; // error ~ |orig-v0|

                // Calculate U parameter and test bounds
                auto u = Dot(tvec, pvec) * inv_det;
                if(u<-additionalEpsilon || u>1.0f+additionalEpsilon)
                    return false;

                // prepare to test V parameter
                auto qvec = Cross(tvec, edge1);

                // Calculate V parameter and test bounds
                auto v = Dot(dir, qvec) * inv_det;
                if(v<-additionalEpsilon || (u+v)>1.0f+additionalEpsilon)
                    return false;

                // Calculate t, ray intersects triangle
                auto t = Dot(edge2, qvec) * inv_det;
                if (t >= maxDist)
                    return false;

                if (outDist)
                    *outDist = t * inv_det;

                if (outU)
                    *outU = u * inv_det;

                if (outV)
                    *outV = v * inv_det;
            }

            // report intersection
            return true;
        }

        bool IntersectBoxRay(const Vector3& origin, const Vector3& dir, const Vector3& invDir, float maxLength, const Vector3& boxMin, const Vector3& boxMax, float* outDist)
        {
            // lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
            // r.org is origin of ray
            float t1 = (boxMin.x - origin.x)*invDir.x;
            float t2 = (boxMax.x - origin.x)*invDir.x;
            float t3 = (boxMin.y - origin.y)*invDir.y;
            float t4 = (boxMax.y - origin.y)*invDir.y;
            float t5 = (boxMin.z - origin.z)*invDir.z;
            float t6 = (boxMax.z - origin.z)*invDir.z;

            float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
            float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));
            if (tmax < 0)
                return false;

            if (tmin > tmax)
                return false;

            if (tmin > maxLength)
                return false;

            if (outDist)
                *outDist = tmin;

            return true;
        }

        //--

        void CalcCircle(const Vector2&a, const Vector2& b, Vector2& outCenter, float& outRadius)
        {
            outCenter = (a + b) * 0.5f;
            outRadius = a.distance(outCenter);
        }

        bool CalcCircumcircle(const Vector2&a, const Vector2& b, const Vector2& c, Vector2& outCenter, float& outRadius)
        {
            // Mathematical algorithm from Wikipedia: Circumscribed circle
            double ox = (std::min(std::min(a.x, b.x), c.x) + std::max(std::min(a.x, b.x), c.x)) / 2;
            double oy = (std::min(std::min(a.y, b.y), c.y) + std::max(std::min(a.y, b.y), c.y)) / 2;
            double ax = a.x - ox,  ay = a.y - oy;
            double bx = b.x - ox,  by = b.y - oy;
            double cx = c.x - ox,  cy = c.y - oy;
            double d = (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by)) * 2;
            if (d == 0)
                return false;
            double x = ((ax*ax + ay*ay) * (by - cy) + (bx*bx + by*by) * (cy - ay) + (cx*cx + cy*cy) * (ay - by)) / d;
            double y = ((ax*ax + ay*ay) * (cx - bx) + (bx*bx + by*by) * (ax - cx) + (cx*cx + cy*cy) * (bx - ax)) / d;
            outCenter = Vector2(ox + x, oy + y);
            outRadius = std::max(std::max(outCenter.distance(a), outCenter.distance(b)), outCenter.distance(c));
            return true;
        }

        //--

        bool CalcCOMProperties(const Array<ShapePtr>& shapes, Vector3& outCOMPosition, Quat& outCOMRotation, float& outVolume, Vector3& outInertia)
        {
            // compute bounds of all the shapes
            Box bounds;
            for (auto& shape : shapes)
                bounds.merge(shape->calcBounds());


			return false;
        }


/*
        // helper class to compute smallest circle
        class BASE_GEOMETRY_API SmallestCircleBuilder
        {
        public:
            SmallestCircleBuilder();

            Vector2 m_center;
            float m_radius;
            float m_radiusSquared;

            void reset();
            void addPoint(const Vector2& point);

            bool contains(const Vector2& point) const;
        };

        SmallestCircleBuilder::SmallestCircleBuilder()
            : m_center(0.0f, 0.0f)
            , m_radius(-1.0f)
            , m_radiusSquared(-1.0f)
        {}

        void SmallestCircleBuilder::reset()
        {
            m_center.zero();
            m_radius = -1.0f;
            m_radiusSquared = -1.0f;
        }

        void SmallestCircleBuilder::addPoint(const Vector2& point)
        {
            if (m_radius <= 0.0f || !contains(point))
            {

            }
        }

        bool SmallestCircleBuilder::contains(const Vector2& point) const
        {
            return m_center.squareDistance(point) <= m_radiusSquared;
        }*/

        //--

    } // shape
} // base