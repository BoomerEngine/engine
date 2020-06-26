/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#pragma once

#include "rendering/driver/include/renderingImageView.h"
#include "base/containers/include/pagedBuffer.h"

namespace rendering
{
    namespace scene
    {
        ///---

        class DebugDrawer;

        ///---

        /// simple rendering vertex, used in the simple rendering :)
        /// NOTE: we don't have and don't want tangent space here as most of this crap has no lighting any way
        struct DebugVertex
        {
            base::Vector3 p;
            base::Vector2 t;
            base::Color c;

            //---

            INLINE DebugVertex()
            {}

            INLINE DebugVertex(float x, float y, float z, float u = 0.0f, float v = 0.0f, base::Color color = base::Color::WHITE)
                : p(x, y, z), t(u, v), c(color)
            {}

            INLINE DebugVertex& pos(const base::Vector3& pos)
            {
                p = pos;
                return *this;
            }

            INLINE DebugVertex& pos(float x, float y, float z = 0.0f)
            {
                p.x = x;
                p.y = y;
                p.z = z;
                return *this;
            }

            INLINE DebugVertex& uv(const base::Vector2& uv)
            {
                t = uv;
                return *this;
            }

            INLINE DebugVertex& uv(float u, float v)
            {
                t.x = u;
                t.y = v;
                return *this;
            }

            INLINE DebugVertex& color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
            {
                c = base::Color(r, g, b, a);
                return *this;
            }

            INLINE DebugVertex& color(base::Color color)
            {
                c = color;
                return *this;
            }
        };

        ///----


        struct DebugVertexEx
        {
            uint32_t subSelectionId = 0;
            uint32_t paramsId = 0;
            int depthBias = 0; // in "step" units
            float size = 0.0f; // pixel size for sprites, line thickness for lines
        };

        ///----

        /// rendering layer for frame geometry
        enum class DebugGeometryLayer : uint8_t
        {
            SceneSolid, // scene fragment, rendering amongst normal geometry
            SceneTransparent, // pre-multiplied alpha transparent MESH (no lines, no sprites here)
            Overlay, // overlay fragment, rendered on top of 3D geometry (gizmos)
            Screen, // screen fragments, mostly text 
        };

        /// rendering mode for direct frame geometry
        enum class DebugGeometryType : uint8_t
        {
            Solid, // full solid
            Sprite, // sprite, assumed alpha tested, transparent if in transparent group
            Lines, // render as lines, always used for line geometry (we can't have transparent lines)
        };

        //--

        struct DebugGeometryElement
        {
            DebugGeometryType type = DebugGeometryType::Solid;
            uint32_t firstIndex = 0;
            uint32_t numIndices = 0;
            uint32_t firstVertex = 0;
            uint32_t numVeritices = 0;
        };

        struct DebugGeometryElementSrc
        {
            DebugGeometryType type = DebugGeometryType::Solid;
            const base::PagedBuffer<uint32_t>* sourceIndicesData = nullptr;
            const base::PagedBuffer<DebugVertex>* sourceVerticesData = nullptr;
            const base::PagedBuffer<DebugVertexEx>* sourceVerticesDataEx = nullptr;
        };

        //--

        /// collector of debug geometry, usually owned by frame
        class DebugGeometry : public base::NoCopy
        {
        public:
            DebugGeometry(DebugGeometryLayer layer = DebugGeometryLayer::SceneSolid);
            ~DebugGeometry();

            //--

            // buffers
            INLINE const base::PagedBuffer<uint32_t>& indices() const { return m_indicesData; }
            INLINE const base::PagedBuffer<DebugVertex>& vertices() const { return m_verticesData; }
            INLINE const base::PagedBuffer<DebugVertexEx>& verticesEx() const { return m_verticesDataEx; }
            INLINE const base::PagedBuffer<DebugGeometryElement>& elements() const { return m_elements; }
                      
            //--

            // push new element
            void pushElement(const DebugGeometryElementSrc& source, const DebugVertexEx& defaultDebugVertex = DebugVertexEx());

            //--

        protected:
            DebugGeometryLayer m_layer;

            //---

            base::PagedBuffer<uint32_t> m_indicesData;
            base::PagedBuffer<DebugVertex> m_verticesData;
            base::PagedBuffer<DebugVertexEx> m_verticesDataEx;
            base::PagedBuffer<DebugGeometryElement> m_elements;

            //---
        };

        //---

    } // rendering
} // scene