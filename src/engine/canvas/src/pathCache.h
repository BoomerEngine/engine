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

#pragma once

#include "style.h"
#include "geometryBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(canvas)

namespace prv
{

    enum class PointTypeFlag : uint8_t
    {
        Corner = 0x01,
        Left = 0x02,
        Bevel = 0x04,
        InnerBevel = 0x08,
    };

    typedef DirectFlags<PointTypeFlag> PointTypeFlags;

    struct PathPoint
    {
        Vector2 pos; // position of point
        Vector2 d; // normalized delta to next point
        float len; // length to next point
        Vector2 dm;
        PointTypeFlags flags; // flags, is this a corner

        INLINE PathPoint(const Vector2& _pos, PointTypeFlags _flags)
            : pos(_pos)
            , d(0,0)
            , len(0.0f)
            , dm(0,0)
            , flags(_flags)
        {}
    };

    struct PathData
    {
        uint32_t first; // first point in the point list
        uint32_t count; // number of points in the path so far
        Winding winding;
        bool closed;
        bool convex;

        uint32_t bevelCount; // number of extra bevel vertices needed

        INLINE PathData()
            : first(0)
            , count(0)
            , winding(Winding::CCW)
            , closed(false)
            , convex(false)
            , bevelCount(0)
        {}
    };

    struct PathCache
    {
        RTTI_DECLARE_POOL(POOL_CANVAS)
    public:

        float minPointDist = 0.025f;
        float tessTol = 0.1f;
        InplaceArray<PathPoint, 64> points;
        InplaceArray<PathData, 4> paths;

        //--

        PathCache(float minPointDist, float tesseltationTollerance);

        INLINE const PathPoint* lastPoint() const
        {
            return points.empty() ? nullptr : &points.back();
        }

        INLINE bool isConvex() const
        {
            return paths.size() == 1 && paths[0].convex;
        }

        void reset();
        void addPath();
        void addPoint(float x, float y, PointTypeFlags flags);
        void addPoint(const Vector2& pos, PointTypeFlags flags);
        void addBezier(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, uint32_t level, PointTypeFlags flags);
        void closePath();
        void winding(Winding winding);

        void computeDeltas();
        void computeJoints(float w, LineJoin lineJoin, float mitterLimit);

        uint32_t computeFillVertexCount(bool hasFringe) const;
        uint32_t computeStrokeVertexCount(LineJoin lineJoin, LineCap lineCap, float strokeWidth) const;
    };

} // prv

END_BOOMER_NAMESPACE_EX(canvas)
