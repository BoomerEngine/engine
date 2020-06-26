/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

// The Canvas class is heavily based on nanovg project by Mikko Mononen
// Adaptations were made to fit the rest of the source code in here

//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "build.h"
#include "canvasGeometry.h"
#include "canvasGeometryPathCache.h"
#include "canvasGeometryBuilder.h"

namespace base
{
    namespace canvas
    {
        namespace prv
        {
    
            PathCache::PathCache(float minPointDist, float tesseltationTollerance)
                : minPointDist(minPointDist)
                , tessTol(tesseltationTollerance)
            {}

            void PathCache::reset()
            {
                paths.reset();
                points.reset();
            }

            void PathCache::addPath()
            {
                paths.emplaceBack();
                paths.back().first = points.size();
            }

            void PathCache::closePath()
            {
                if (!paths.empty())
                    paths.back().closed = true;
            }

            void PathCache::winding(Winding winding)
            {
                if (!paths.empty())
                    paths.back().winding = winding;
            }

            void PathCache::addBezier(const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& p4, uint32_t level, PointTypeFlags flags)
            {
                if (level > 10)
                {
                    addPoint(p4, flags);
                    return;
                }

                // calculate half points (geometric bezier interpolation method)
                auto p12 = (p1 + p2) * 0.5f;
                auto p23 = (p2 + p3) * 0.5f;
                auto p34 = (p3 + p4) * 0.5f;
                auto p123 = (p12 + p23) * 0.5f;
                auto p234 = (p23 + p34) * 0.5f;
                auto p1234 = (p123 + p234) * 0.5f;

                // is there a point tesselating more ?
                {
                    auto d40 = p4 - p1;
                    auto d2 = fabs((p2.x - p4.x) * d40.y - (p2.y - p4.y) * d40.x);
                    auto d3 = fabs((p3.x - p4.x) * d40.y - (p3.y - p4.y) * d40.x);
                    if ((d2 + d3)*(d2 + d3) < tessTol * d40.squareLength())
                    {
                        addPoint(p4, flags);
                        return;
                    }
                }

                // recurse
                addBezier(p1, p12, p123, p1234, level + 1, PointTypeFlags());
                addBezier(p1234, p234, p34, p4, level + 1, flags);
            }

            void PathCache::addPoint(const Vector2& pos, PointTypeFlags flags)
            {
                if (paths.empty())
                    return;

                if (!points.empty())
                {
                    auto& lastPoint = points.back();
                    if (pos.isSimilar(lastPoint.pos, minPointDist))
                    {
                        lastPoint.flags |= flags;
                        return;
                    }
                }

                points.pushBack(PathPoint(pos, flags));
                paths.back().count += 1;

            }

            void PathCache::addPoint(float x, float y, PointTypeFlags flags)
            {
                addPoint(Vector2(x, y), flags);
            }

            static INLINE float CalcTriangleArea(const Vector2& a, const Vector2& b, const Vector2& c)
            {
                auto ab = b - a;
                auto ac = c - a;
                return ac.x*ab.y - ab.x*ac.y;
            }

            static float CalcPathArea(const PathPoint* points, uint32_t numPoints)
            {
                float area = 0.0f;
                for (uint32_t i=2; i<numPoints; i++)
                    area += CalcTriangleArea(points[0].pos, points[i - 1].pos, points[i].pos);

                return area * 0.5f;
            }

            static void ReversePathPoints(PathPoint* points, uint32_t numPoints)
            {
                uint32_t i = 0;
                uint32_t j = numPoints - 1;

                while (i < j)
                    std::swap(points[i++], points[j--]);
            }

            void PathCache::computeDeltas()
            {
                // count lengths of the paths and the deltas between points
                for (auto& path : paths)
                {
                    // skip over empty paths
                    if (path.count <= 1)
                        continue;

                    auto curPoint  = points.typedData() + path.first;
                    auto lastPoint  = curPoint + path.count - 1;

                    // If the first and last points are the same, remove the last, mark as closed path.
                    if (curPoint[0].pos.isSimilar(lastPoint->pos, minPointDist))
                    {
                        path.closed = true;
                        path.count -= 1;
                        lastPoint -= 1;
                    }

                    // Enforce path winding as set with in the winding flag
                    if (path.count > 2)
                    {
                        auto area = CalcPathArea(curPoint, path.count);
                        if ((path.winding == Winding::CCW && area < 0.0f) || (path.winding == Winding::CW && area > 0.0f))
                            ReversePathPoints(curPoint, path.count);
                    }

                    // compute deltas
                    for (uint32_t i=0; i<path.count; ++i, lastPoint = curPoint++ )
                    {
                        lastPoint->d = curPoint->pos - lastPoint->pos;
                        lastPoint->len = lastPoint->d.length();

                        if (lastPoint->len > 0.0f)
                            lastPoint->d /= lastPoint->len;

                        //boundsMin = Min(boundsMin, lastPoint->pos);
                        //boundsMax = Min(boundsMax, lastPoint->pos);
                    }
                }
            }

            static Vector2 CalcExtrusionVector(const PathPoint& p0, const PathPoint& p1, float& dmr)
            {
                auto dl0 = p0.d.prep();
                auto dl1 = p1.d.prep();

                auto dm = (dl0 + dl1) * 0.5f;

                dmr = dm.squareLength();
                if (dmr > 0.000001f)
                {
                    float scale = std::min(600.0f, 1.0f / dmr);
                    return dm * scale;
                }
                else
                {
                    return Vector2::ZERO();
                }
            }

            void PathCache::computeJoints(float w, LineJoin lineJoin, float miterLimit)
             {
                float iw = (w > 0.0f) ? (1.0f / w) : 0.0f;
                
                // Calculate which joins needs extra vertices to append, and gather vertex count
                for (auto& path : paths)
                {
                    // skip over empty paths
                    if (path.count <= 1)
                        continue;

                    auto curPoint  = points.typedData() + path.first;
                    auto lastPoint  = curPoint + path.count - 1;

                    uint32_t nleft = 0;
                    path.bevelCount = 0;
                    for (uint32_t j=0; j<path.count; j++, lastPoint = curPoint++)
                    {
                        // calculate the extrusion direction
                        float dmr2;
                        curPoint->dm = CalcExtrusionVector(*lastPoint, *curPoint, dmr2);

                        // clear flags, but keep the corner flag
                        curPoint->flags &= PointTypeFlag::Corner;

                        // keep track of left turns
                        float cross = curPoint->d.x * lastPoint->d.y - lastPoint->d.x * curPoint->d.y;
                        if (cross > 0.0f)
                        {
                            nleft++;
                            curPoint->flags |= PointTypeFlag::Left;
                        }

                        // calculate if we should use bevel or miter for inner join
                        float limit = std::max(1.01f, std::min(lastPoint->len, curPoint->len) * iw);
                        if ((dmr2 * limit*limit) < 1.0f)
                            curPoint->flags |= PointTypeFlag::InnerBevel;

                        // check to see if the corner needs to be beveled
                        if (curPoint->flags.test(PointTypeFlag::Corner))
                            if ((dmr2 * miterLimit*miterLimit) < 1.0f || lineJoin == LineJoin::Bevel || lineJoin == LineJoin::Round)
                                curPoint->flags |= PointTypeFlag::Bevel;

                        if ((curPoint->flags.test(PointTypeFlag::Bevel)) || (curPoint->flags.test(PointTypeFlag::InnerBevel)))
                            path.bevelCount += 1;
                    }

                    // the path is convex if all edges go into "left"
                    path.convex = (nleft == path.count);
                }
            }

            static INLINE uint32_t CalcCurveDivs(float r, float arc, float tol)
            {
                float da = acosf(r / (r + tol)) * 2.0f;
                return (uint32_t)std::max<int>(2, (int)ceil(arc / da));
            }

            uint32_t PathCache::computeStrokeVertexCount(LineJoin lineJoin, LineCap lineCap, float strokeWidth) const
            {
                auto ncap = CalcCurveDivs(strokeWidth, PI, tessTol); // calculate divisions per half circle.

                uint32_t ret = 0;
                for (auto& path : paths)
                {
                    if (lineJoin == LineJoin::Round)
                        ret += (path.count + path.bevelCount*(ncap + 2) + 1) * 2; // plus one for loop
                    else
                        ret += (path.count + path.bevelCount * 5 + 1) * 2; // plus one for loop

                    if (!path.closed)
                    {
                        if (lineCap == LineCap::Round)
                            ret += (ncap * 2 + 2) * 2;
                        else
                            ret += (3 + 3) * 2;
                    }
                }

                return ret;
            }

            uint32_t PathCache::computeFillVertexCount(bool hasFringe) const
            {
                uint32_t ret = 0;
                for (auto& path : paths)
                {
                    ret += path.count + path.bevelCount + 1;
                    if (hasFringe)
                        ret += (path.count + path.bevelCount * 5 + 1) * 2; // plus one for loop
                }
                return ret;
            }

        } // prv
    } // canvas
} // base
