/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#pragma once

#include "base/containers/include/pagedBuffer.h"
#include "renderingSelectable.h"

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

			Selectable selectable;
        };

        struct DebugGeometryElementSrc : public DebugGeometryElement
        {
            const uint32_t* sourceIndicesData = nullptr; // can be empty
            const DebugVertex* sourceVerticesData = nullptr;
        };

        //--

        /// collector of debug geometry, usually owned by frame
        class DebugGeometry : public base::NoCopy
        {
        public:
            DebugGeometry();

            //--

            // buffers
            INLINE const base::PagedBufferTyped<uint32_t>& indices() const { return m_indices; }
            INLINE const base::PagedBufferTyped<DebugVertex>& vertices() const { return m_vertices; }
            INLINE const base::Array<DebugGeometryElement>& elements() const { return m_elements; }
                      
            //--

			// clear collector
			void clear();

            // push new element
			void push(const DebugGeometryElementSrc& source);

            //--

        protected:
            DebugGeometryLayer m_layer;

            //---

            base::PagedBufferTyped<uint32_t> m_indices;
            base::PagedBufferTyped<DebugVertex> m_vertices;
            base::InplaceArray<DebugGeometryElement, 1024> m_elements;

            //---
        };

        //---

    } // rendering
} // scene