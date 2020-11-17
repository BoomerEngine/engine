/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#pragma once

#include "renderingFrameDebugGeometry.h"

namespace rendering
{
    namespace scene
    {
        //--

        struct DebugVertex;
        struct DebugVertexEx;

        //--

        // a line rendering context, all gathered lines will be written out as one batch (unless the flush() was used)
        class RENDERING_SCENE_API DebugDrawer : public base::NoCopy
        {
        public:
            DebugDrawer(DebugGeometry& dg, DebugGeometryType type, const base::Matrix& localToWorld = base::Matrix::IDENTITY());
            ~DebugDrawer();

            //--

            // set transform for current geometry generation
            void localToWorld(const base::Matrix& localToWorld);

            // change color
            INLINE void color(base::Color color) { m_color = color; }

            // change size (lines, sprites)
            INLINE void size(float val) { m_size = val; } 

            // change sub-selection ID
            INLINE void subSelectionId(uint32_t id) { m_subSelectionId = id; }

            // change the depth bias
            INLINE void depthBias(int bias) { m_depthBias = bias; }

            //--

            // append vertices, vertices are transformed by current matrix (with exception to 2D vertices that are not)
            uint32_t appendVertices(const DebugVertex* vertex, uint32_t numVertices, const float* sizeOverride = nullptr, const uint32_t* subSelectionIDOverride = nullptr);
            uint32_t appendVertices(const base::Vector2* points, uint32_t numVertices, const float* sizeOverride = nullptr, const uint32_t* subSelectionIDOverride = nullptr, const base::Color* colorOverride = nullptr);
            uint32_t appendVertices(const base::Vector3* points, uint32_t numVertices, const float* sizeOverride = nullptr, const uint32_t* subSelectionIDOverride = nullptr, const base::Color* colorOverride = nullptr);

            // append indices to the current element
            void appendIndices(const uint32_t* indices, uint32_t numIndices, uint32_t firstVertex);
            void appendIndices(const uint16_t* indices, uint32_t numIndices, uint32_t firstVertex);

            // append auto generated indices to current element
            void appendAutoPointIndices(uint32_t firstVertex, uint32_t numVertices);
            void appendAutoLineLoopIndices(uint32_t firstVertex, uint32_t numVertices);
            void appendAutoLineListIndices(uint32_t firstVertex, uint32_t numVertices);
            void appendAutoTriangleFanIndices(uint32_t firstVertex, uint32_t numVertices, bool swap = false);
            void appendAutoTriangleListIndices(uint32_t firstVertex, uint32_t numVertices, bool swap = false);

            //--

            // flush current state
            void flush();

            //--

        protected:
            DebugGeometry& m_geometry;

            DebugGeometryType m_geometryType;
            bool m_localToWorldSet = false;
            uint32_t m_subSelectionId = 0;
            uint32_t m_currentParamsId = 0;
            float m_size = 1.0f;
            int m_depthBias = 0;
            base::Color m_color = base::Color::WHITE;

            base::Matrix m_localToWorld;

            base::PagedBuffer<DebugVertex> m_verticesData;
            base::PagedBuffer<DebugVertexEx> m_verticesExData;
            base::PagedBuffer<uint32_t> m_indicesData;

            void appendControlVertices(uint32_t count, const float* sizeOverride = nullptr, const uint32_t* subSelectionIDOverride = nullptr);
        };

        //--

        // a line rendering context, all gathered lines will be written out as one batch (unless the flush() was used)
        class RENDERING_SCENE_API DebugLineDrawer : public DebugDrawer
        {
        public:
            DebugLineDrawer(DebugGeometry& dg, const base::Matrix& localToWorld = base::Matrix::IDENTITY());

            //--

            // draw a single line form point A to point B
            void line(const base::Vector3& a, const base::Vector3& b);

            // draw list of 2D lines, active color and sub selection ID is used
            void lines(const base::Vector2* points, uint32_t numPoints);

            // draw list of 3D lines, active color and sub selection ID is used
            void lines(const base::Vector3* points, uint32_t numPoints);

            // draw list of 3D lines with custom colors
            void lines(const DebugVertex* points, uint32_t numPoints);

            // draw a line loop from 2D vertices, active color and sub selection ID is used
            void loop(const base::Vector2* points, uint32_t numPoints);

            // draw a line loop from 3D vertices, active color and sub selection ID is used
            void loop(const base::Vector3* points, uint32_t numPoints);

            // draw line loop from 3D vertices with custom colors
            void loop(const DebugVertex* points, uint32_t numPoints);

            // draw an arrow with a head from "start" point to the "end" point
            void arrow(const base::Vector3& start, const base::Vector3& end);

            // draw a 3-axis XYZ coordinate system, the axes are colored Red, Green, Blue
            void axes(const base::Matrix& additionalTransform, float length = 0.5f);

            // draw a 3-axis XYZ coordinate system, the axes are colored Red, Green, Blue
            void axes(const base::Vector3& origin, const base::Vector3& x, const base::Vector3& y, const base::Vector3& z, float length = 0.5f);

            // add box bracket (corners highlight)
            void brackets(const base::Vector3* corners, float length = 0.1f);

            // add box bracket (corners highlight)
            void brackets(const base::Box& box, float length = 0.1f);

            //--

            // box shape - from 8 corners, can be any points actually (good for frustum)
            void box(const base::Vector3* corners);

            // box shape from extens
            void box(const base::Vector3& boxMin, const base::Vector3& boxMax);

            // box shape
            void box(const base::Box& box);

            // a sphere
            void sphere(const base::Vector3& center, float radius);

            // capsule, oriented on Z axis
            void capsule(const base::Vector3& center, float radius, float halfHeight);

            // a cylinder, can be capped
            void cylinder(const base::Vector3& center1, const base::Vector3& center2, float radius1, float radius2);

            // cone from origin and direction
            void cone(const base::Vector3& top, const base::Vector3& dir, float radius, float angleDeg);

            // plane (grid of lines)
            void plane(const base::Vector3& pos, const base::Vector3& normal, float size = 10.0f, int gridSize = 10);

            // camera projection frustum
            void frustum(const base::Matrix& frustumMatrix, float farPlaneScale = 1.0f);

            // a simple 2D circle
            void circle(const base::Vector3& center, const base::Vector3& normal, float radius, float startAngle = 0.0f, float endAngle = 360.0);

            //--

            // draw a wire rectangle - 2D shape
            void rect(float x0, float y0, float w, float h);
            void recti(int x0, int  y0, int  w, int  h);

            // draw a wire rectangle defined by bounds - 2D shape
            void bounds(float x0, float y0, float x1, float y1);
            void boundsi(int  x0, int  y0, int  x1, int  y1);
        };

        //--

        // sprite rendering context
        class RENDERING_SCENE_API DebugSpriteDrawer : public DebugDrawer
        {
        public:
            DebugSpriteDrawer(DebugGeometry& dg, const base::Matrix& localToWorld = base::Matrix::IDENTITY());

            //--

            // change image
            void image(const base::image::ImagePtr& image);
            
            //--

            // add single sprite at location
            void sprite(const base::Vector3& pos);

            // add multiple sprites
            void sprites(const base::Vector3* positions, uint32_t count);

            //--

        protected:
            base::image::ImagePtr m_image;
        };

        //--

        // debug text params
        struct DebugTextParams
        {
            DebugFont _font = DebugFont::Normal;
            base::Color _color = base::Color::WHITE;
            base::Color _boxBackground = base::Color(0, 0, 0, 0);
            base::Color _boxFrame = base::Color(0, 0, 0, 0);
            float _boxFrameWidth = 1.0f;
            uint8_t _boxMargin = 4;
            char _alignX = -1;
            char _alignY = -1;
            int _offsetX = 0;
            int _offsetY = 0;

            DebugTextParams& font(DebugFont font) { _font = font; return *this; }
            DebugTextParams& small() { _font = DebugFont::Small; return *this; }
            DebugTextParams& big() { _font = DebugFont::Big; return *this; }
            DebugTextParams& italic() { _font = DebugFont::Italic; return *this; }
            DebugTextParams& bold() { _font = DebugFont::Bold; return *this; }

            DebugTextParams& color(base::Color color) { _color = color; return *this; }
            DebugTextParams& background(base::Color color) { _boxBackground = color; return *this; }
            DebugTextParams& backgroundMargin(int margin) { _boxMargin = margin; }
            DebugTextParams& frame(base::Color color, float width = 1.0f) { _boxFrame = color; _boxFrameWidth = width; return *this; }

            DebugTextParams& alignX(int align) { _alignX = align; return *this; }
            DebugTextParams& alignY(int align) { _alignY = align; return *this; }
            DebugTextParams& left() { _alignX = -1; return *this; }
            DebugTextParams& center() { _alignX = 0; return *this; }
            DebugTextParams& right() { _alignX = 1; return *this; }
            DebugTextParams& top() { _alignY = -1; return *this; }
            DebugTextParams& middle() { _alignY = 0; return *this; }
            DebugTextParams& bottom() { _alignY = 1; return *this; }

            DebugTextParams& offset(int x, int y) { _offsetX = x; _offsetY = y; return *this; }
        };

        //--

        // a solid rendering context, all gathered lines will be written out as one batch (unless the flush() was used)
        class RENDERING_SCENE_API DebugSolidDrawer : public DebugDrawer
        {
        public:
            DebugSolidDrawer(DebugGeometry& dg, const base::Matrix& localToWorld = base::Matrix::IDENTITY());

            //--

            // draw a polygon from 2D vertices
            void polygon(const base::Vector2* points, uint32_t numPoints, bool swap=false);

            // draw a polygon from 3D vertices
            void polygon(const base::Vector3* points, uint32_t numPoints, bool swap = false);

            // draw a polygon from 3D vertices with custom colors
            void polygon(const DebugVertex* points, uint32_t numPoints, bool swap = false);

            // draw indexed triangles made from 2D vertices
            void indexedTris(const base::Vector2* points, uint32_t numPoints, const uint32_t* indices, uint32_t numIndices);

            // draw indexed triangles made from 3D vertices
            void indexedTris(const base::Vector3* points, uint32_t numPoints, const uint32_t* indices, uint32_t numIndices);

            // draw indexed triangles made from 3D vertices with custom colors
            void indexedTris(const DebugVertex* points, uint32_t numPoints, const uint32_t* indices, uint32_t numIndices);

            //--

            // draw solid box from 8 corners
            void box(const base::Vector3* corners);

            // draw a box from a min/max extents
            void box(const base::Vector3& boxMin, const base::Vector3& boxMax);

            // draw box
            void box(const base::Box& box);

            // draw sphere
            void sphere(const base::Vector3& center, float radius);

            // draw capsule
            void capsule(const base::Vector3& center, float radius, float halfHeight);

            // draw cylinder
            void cylinder(const base::Vector3& center1, const base::Vector3& center2, float radius1, float radius2);

            // draw a cone
            void cone(const base::Vector3& top, const base::Vector3& dir, float radius, float angleDeg);

            // draw a plane
            void plane(const base::Vector3& pos, const base::Vector3& normal, float size = 10.0f, int gridSize = 10);

            // draw a frustum
            void frustum(const base::Matrix& frustumMatrix, float farPlaneScale = 1.0f);

            // draw a circle
            void circle(const base::Vector3& center, const base::Vector3& normal, float radius, float startAngle = 0.0f, float endAngle = 360.0);

            //--

            // add rectangle (mostly for screen)
            void rect(float x0, float y0, float w, float h);
            void recti(int x0, int  y0, int  w, int  h);

            // add rectangle from bounds (mostly for screen)
            void bounds(float x0, float y0, float x1, float y1);
            void boundsi(int  x0, int  y0, int  x1, int  y1);

            //--

            // measure size
            base::Point textSize(base::StringView txt, DebugFont font = DebugFont::Normal);

            // draw simple debug text
            base::Point text(int x, int y, base::StringView txt, const DebugTextParams& params = DebugTextParams());

            //--
        };

        //--

    } // rendering
} // scene