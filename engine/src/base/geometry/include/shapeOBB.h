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

        /// oriented bounding box
        class BASE_GEOMETRY_API OBB : public IConvexShape
        {
            RTTI_DECLARE_VIRTUAL_CLASS(OBB, IConvexShape);

        public:
            //--

            INLINE OBB() : m_position(0,0,0,0), m_edge1(0,0,0,0), m_edge2(0,0,0,0) {};
            INLINE OBB(const OBB& other) = default;
            INLINE OBB(OBB&& other) = default;
            INLINE OBB& operator=(const OBB& other) = default;
            INLINE OBB& operator=(OBB&& other) = default;

            // create OBB at given position
            OBB(const Vector3& pos, const Vector3& edge1, const Vector3& edge2, const Vector3& edge3);

            // create OBB from box at given orientation
            OBB(const Box& box, const Matrix& transform);

            //--

            // get position of the OOB corner
            INLINE const Vector3& position() const { return m_position.xyz(); }

            // get the first edge vector
            INLINE const Vector3& edgeA() const { return m_edge1.xyz(); }

            // get the second edge vector
            INLINE const Vector3& edgeB() const { return m_edge2.xyz(); }

            // calculate center of mass
            Vector3 calcCenter() const;

            // calculate the 3rd edge
            Vector3 calcEdgeC() const;

            // calculate corners of the
            void calcCorners(Vector3* outCorners) const;

            //--

            // Compute volume of the shape
            virtual float calcVolume() const override final;

            // compute bounding box of the shape in the coordinates of the shape
            virtual Box calcBounds() const override final;

            // calculate data CRC
            virtual void calcCRC(base::CRC64& crc) const override final;

            // create a copy
            virtual ShapePtr copy() const override final;

            // check if this convex hull contains a given point
            virtual bool contains(const Vector3& point) const override final;

            // intersect this convex shape with ray, returns distance to point of entry
            virtual bool intersect(const Vector3& origin, const Vector3& direction, float maxLength = VERY_LARGE_FLOAT, float* outEnterDistFromOrigin = nullptr, Vector3* outEntryPoint = nullptr, Vector3* outEntryNormal = nullptr) const override final;

            // render the shape geometry via the renderer interface
            virtual void render(IShapeRenderer& renderer, ShapeRenderingMode mode = ShapeRenderingMode::Solid, ShapeRenderingQualityLevel qualityLevel = ShapeRenderingQualityLevel::Medium) const override final;

        public:
            base::Vector4 m_position; //! position (x,y,z), edge3 length (w)
            base::Vector4 m_edge1; //! foward, edge1 - normalized, length (w)
            base::Vector4 m_edge2; //! right, edge2 - normalized, length (w)
        };

        //---

    } // shape
} // base