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

        /// cylinder
        class BASE_GEOMETRY_API Cylinder : public IConvexShape
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Cylinder, IConvexShape);

        public:
            //--

            INLINE Cylinder() : m_positionAndRadius(0,0,0,0), m_normalAndHeight(0,0,0,0) {};
            INLINE Cylinder(const Cylinder& other) = default;
            INLINE Cylinder(Cylinder&& other) = default;
            INLINE Cylinder& operator=(const Cylinder& other) = default;
            INLINE Cylinder& operator=(Cylinder&& other) = default;

            // create cylinder between two positions with given radius
            Cylinder(const Vector3& pos1, const Vector3& pos2, float radius);

            // Create cylinder at given position, pointing along the normal vector with given radius and height
            Cylinder(const Vector3& pos, const Vector3& normal, float radius, float height );

            //--

            // get height of the cylinder
            INLINE float height() const { return m_normalAndHeight.w; }

            // get radius of the cylinder
            INLINE float radius() const { return m_positionAndRadius.w; }

            // get position at the bottom side of the cylinder
            INLINE const Vector3& position() const { return m_positionAndRadius.xyz(); }

            // get the direction between bottom and top part of the cylinder
            INLINE const Vector3& normal() const { return m_normalAndHeight.xyz(); }

            // calculate the U and V cylinder vectors (vectors in the base plane perpendicular to the normal)
            void calcBaseVectors(Vector3& outU, Vector3& outV) const;

            // get position at the bottom side of the cylinder
            Vector3 calcPosition2() const;

            // get center of mass for the cylinder
            Vector3 calcCenter() const;

            //--

            // build from mesh
            void buildFromMesh(const ISourceMeshInterface& mesh, uint8_t axis = 2, float shinkBy = 0.0f, const Matrix* localToParent = nullptr);

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
            base::Vector4 m_positionAndRadius; //< position (x,y,z), radius (w)
            base::Vector4 m_normalAndHeight;   //< orientation (x,y,z), height (w)
        };

        //---

    } // shape
} // base