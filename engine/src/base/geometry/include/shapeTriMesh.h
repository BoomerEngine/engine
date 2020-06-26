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

        /// collision triangle (20 bytes)
#pragma pack( push, 1 )
        struct TriMeshTriangle
        {
            uint16_t v0[3];
            uint16_t v1[3];
            uint16_t v2[3];
            uint16_t chunk;
        };

        /// collision node
        struct TriMeshNode
        {
            uint16_t boxMin[3];
            uint16_t boxMax[3];
            uint32_t childIndex;
            uint32_t triangleCount;
        };
#pragma pack(pop)


        //---

        /// triangle mesh shape, owns all the data
        class BASE_GEOMETRY_API TriMesh : public IShape
        {
            RTTI_DECLARE_VIRTUAL_CLASS(TriMesh, IShape);

        public:
            TriMesh();

            //--

            // get the internal bounds of the triangles
            INLINE const Box& bounds() const { return m_bounds; }

            // get number of triangles
            INLINE uint32_t numTriangles() const { return m_triangles.size() / sizeof(TriMeshTriangle); }

            // get number of nodes
            INLINE uint32_t numNodes() const { return m_nodes.size() / sizeof(TriMeshNode); }

            //--

            // build from list triangle soup
            void buildFromMesh(const ISourceMeshInterface& sourceMesh, const Matrix* localToParent = nullptr);

            //--

            // is this shape convex ?
            virtual bool isConvex() const override final;

            // Compute volume of the shape
            virtual float calcVolume() const override final;

            // compute bounding box of the shape in the coordinates of the shape
            virtual Box calcBounds() const override final;

            // calculate data size used by the shape
            virtual uint32_t calcDataSize() const override final;

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
            Buffer m_nodes;
            Buffer m_triangles;
            Box m_bounds;
            Vector3 m_quantizationScale;
        };

        //---

    } // shape
} // base