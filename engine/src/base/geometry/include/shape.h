/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#pragma once

namespace base
{
    namespace shape
    {

        //---

        /// quality level for shape rendering, affects mostly the parametric shapes
        enum class ShapeRenderingQualityLevel : uint8_t
        {
            Low,
            Medium,
            High,
        };

        //---

        /// quality level for shape rendering, affects mostly the parametric shapes
        enum class ShapeRenderingMode : uint8_t
        {
            Wire,
            Solid,
        };

        //---

        /// shape rendering helper
        class BASE_GEOMETRY_API IShapeRenderer : public base::NoCopy
        {
        public:
            virtual ~IShapeRenderer();

            /// add list of vertices to the drawing buffer, returns index
            virtual uint32_t addVertices(const Vector3* vertices, uint32_t numVertices) = 0;

            /// add list of indices for triangles in case of fill rendering or lines in case of wire rendering
            virtual void addIndices(uint32_t base, const uint32_t* indices, uint32_t numIndices) = 0;
        };

        //---

        /// common shape interface
        class BASE_GEOMETRY_API IShape : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(IShape, IObject);

        public:
            IShape();
            virtual ~IShape();

            // is this shape convex ?
            virtual bool isConvex() const = 0;

            // compute volume of the shape
            // NOTE: this most certainly fail for non-convex shapes
            virtual float calcVolume() const = 0;

            // compute bounding box of the shape in the coordinates of the shape
            virtual Box calcBounds() const = 0;

            // calculate data size used by the shape
            virtual uint32_t calcDataSize() const;

            // create a copy
            virtual ShapePtr copy() const = 0;

            // calculate data CRC
            virtual void calcCRC(base::CRC64& crc) const = 0;

            //--

            // check if this convex hull contains a given point
            virtual bool contains(const Vector3& point) const = 0;

            // intersect this convex shape with ray, returns distance to point of entry
            virtual bool intersect(const Vector3& origin, const Vector3& direction, float maxLength = VERY_LARGE_FLOAT, float* outEnterDistFromOrigin = nullptr, Vector3* outEntryPoint = nullptr, Vector3* outEntryNormal = nullptr) const = 0;

            //--

            // render the shape geometry via the renderer interface
            virtual void render(IShapeRenderer& renderer, ShapeRenderingMode mode = ShapeRenderingMode::Solid, ShapeRenderingQualityLevel qualityLevel = ShapeRenderingQualityLevel::Medium) const = 0;
        };

        //---

        // common class for convex shapes
        class BASE_GEOMETRY_API IConvexShape : public IShape
        {
            RTTI_DECLARE_VIRTUAL_CLASS(IConvexShape, IShape);

        public:
            IConvexShape();
            virtual ~IConvexShape();

            // is this shape convex ?
            virtual bool isConvex() const override final;
        };


    } // shape
} // base