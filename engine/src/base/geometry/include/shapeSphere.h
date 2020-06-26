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

        /// sphere
        class BASE_GEOMETRY_API Sphere : public IConvexShape
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Sphere, IConvexShape);

        public:
            //--

            INLINE Sphere() : m_positionAndRadius(0,0,0,0) {};
            INLINE Sphere(const Sphere& other) = default;
            INLINE Sphere(Sphere&& other) = default;
            INLINE Sphere& operator=(const Sphere& other) = default;
            INLINE Sphere& operator=(Sphere&& other) = default;

            // create Sphere at given position and with given radius
            Sphere(const Vector3& pos, float radius);

            //--

            // get radius of the sphere
            INLINE float radius() const { return m_positionAndRadius.w; }

            // get position of the sphere's center
            INLINE const Vector3& position() const { return *(const Vector3*) &m_positionAndRadius; }

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
        };

        //---

    } // shape
} // base