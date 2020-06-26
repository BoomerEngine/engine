/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#pragma once

#include "shape.h"

namespace base
{
    namespace shape
    {
        //---

        /// axis aligned box
        class BASE_GEOMETRY_API AABB : public IConvexShape
        {
            RTTI_DECLARE_VIRTUAL_CLASS(AABB, IConvexShape);

        public:
            //--

            INLINE AABB() : m_min(0,0,0), m_max(0,0,0) {};
            INLINE AABB(const AABB& other) = default;
            INLINE AABB(AABB&& other) = default;
            INLINE AABB& operator=(const AABB& other) = default;
            INLINE AABB& operator=(AABB&& other) = default;

            // create AABB from a box
            AABB(const Box& box);

            // create AABB from min/max
            AABB(const Vector3& boxMin, const Vector3& boxMax);

            //--

            // get the min
            INLINE const Vector3& min() const { return m_min; }

            // get the max
            INLINE const Vector3& max() const { return m_max; }

            // get the center of the box
            Vector3 calcCenter() const;

            // get the extents of the box
            Vector3 calcSize() const;

            // calculate corners of the
            void calcCorners(Vector3* outCorners) const;

            //--

            // build from mesh
            void buildFromMesh(const ISourceMeshInterface& mesh, float shinkBy = 0.0f, const Matrix* localToParent = nullptr);

            //--

            // Compute mass (assumes density of 1)
            virtual float calcVolume() const override final;

            // compute bounding box of the shape in the coordinates of the shape
            virtual Box calcBounds() const override final;

            // create a copy
            virtual ShapePtr copy() const override final;

            // calculate data CRC
            virtual void calcCRC(base::CRC64& crc) const override final;

            // check if this convex hull contains a given point
            virtual bool contains(const Vector3& point) const override final;

            // intersect this convex shape with ray, returns distance to point of entry
            virtual bool intersect(const Vector3& origin, const Vector3& direction, float maxLength = VERY_LARGE_FLOAT, float* outEnterDistFromOrigin = nullptr, Vector3* outEntryPoint = nullptr, Vector3* outEntryNormal = nullptr) const override final;

            // render the shape geometry via the renderer interface
            virtual void render(IShapeRenderer& renderer, ShapeRenderingMode mode = ShapeRenderingMode::Solid, ShapeRenderingQualityLevel qualityLevel = ShapeRenderingQualityLevel::Medium) const override final;

        public:
            base::Vector3 m_min;
            base::Vector3 m_max;
        };

        //---

    } // shape
} // base