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
#include "canvasStyle.h"

namespace base
{
    namespace canvas
    {
        /// raster composite operation - determines how pixels are mixed
        /// implemented using classical blending scheme
        enum class CompositeOperation : uint8_t
        {
            // Src=One, Dest=Zero
            // No blending, values are copied directly
            Copy,

            // Src=SrcAlpha, Dest=1-SrcAlpha
            // Typical alpha blending, user SourceOver if possible - it's better
            Blend,

            // Src=One, Dest=1-SrcAlpha
            // Composition of premultiplied alpha images
            SourceOver,

            // Src=DestAlpha, Dest=Zero
            SourceIn,

            // Src=1-DestAlpha, Dest=Zero
            SourceOut,

            // Src=DestAlpha, Dest=1-SrcAlpha
            SourceAtop,

            // Src=1-DestAlpha, Dest=One
            DestinationOver,

            // Src=Zero, Dest=SrcAlpha
            DestinationIn,

            // Src=Zero, Dest=1-SrcAlpha
            DestinationOut,

            // Src=1-DestAlpha, Src=SrcAlpha
            DestinationAtop,

            // Addtive blending
            // Src=One, Dest=One
            Addtive,

            // XOR mode
            // Src=1-DestAlpha, Dest=1-SrcAlpha
            Xor,

            MAX,
        };

        //// rendering vertex for canvas data
        TYPE_ALIGN(4, struct) RenderVertex
        {
            Vector2 pos;
            Vector2 uv;
            Vector2 paintUV; // encoded UV inside the paint brush at the time of rendering
        };

        //// polygon (closed collection of vertices)
        //// has optional strong and filling
        struct RenderPath
        {
            uint32_t fillVertexCount = 0;
            RenderVertex* fillVertices = nullptr;
            uint32_t strokeVertexCount = 0;
            RenderVertex* strokeVertices = nullptr;
        };

        /// text glyph
        /// placement is in local space
        /// glyphs are allocated in the texture during actual rendering
        struct RenderGlyph
        {
            const font::Glyph* glyph = nullptr;

            Vector2 coords[4]; // vertex positions for the glyph (4 because we may be rotated)
            Vector2 uvMin; // uv for the glyph- assigned in prepareGlyphsForRendering
            Vector2 uvMax; // uv for the glyph- assigned in prepareGlyphsForRendering
            Color color; // modulation color
            uint8_t page; // page for the glyph - assigned in prepareGlyphsForRendering
        };
        
        /// a renderable group of polygons
        /// collection of polygons sharing the same style
        /// NOTE: this is the input for the rendering
        struct RenderGroup
        {
            enum class Type : uint8_t
            {
                Fill,
                Stoke,
                Triangle,
                Glyphs,
            };

            uint16_t styleIndex = 0;
            CompositeOperation op = CompositeOperation::SourceOver;
            Type type = Type::Fill;
            bool convex = false;

            RenderPath* paths = nullptr;
            uint32_t numPaths = 0; // paths (if any - if not set than we render the vertices "raw")

            RenderVertex* vertices = nullptr; // all vertices used by the group
            uint32_t numVertices = 0; // number of vertices used by the group

            RenderGlyph* glyphs = nullptr; // rendering glyphs
            uint32_t numGlyphs = 0; // number of glyphs

            //float fringeWidth = 0.0f; // AA
            //float strokeWidth = 1.0f; // Strokes only

            Vector2 vertexBoundsMin;
            Vector2 vertexBoundsMax;
        };

        /// cached renderable geometry helper class
        /// separated from main canvas to allow ekhm, caching
        /// main use is to specify this container when using the path builder
        /// a cached geometry can be submitted to the canvas multiple times at fraction of the cost
        /// needed mainly for the UI project
        class BASE_CANVAS_API Geometry : public base::IReferencable // it's not possible to copy this shit because we have pointers to self in the data structures
        {
        public:
            Geometry();
            Geometry(Geometry&& other) = default;
            ~Geometry();

            Geometry& operator=(Geometry&& other) = default;

            //--

            INLINE bool empty() const { return m_vertices.empty() && m_glyphs.empty(); }

            INLINE uint32_t numVertices() const { return m_vertices.size(); }

            INLINE uint32_t numPaths() const { return m_paths.size(); }

            INLINE uint32_t numGroups() const { return m_groups.size(); }
            INLINE const RenderGroup* groups() const { return m_groups.typedData(); }

            INLINE uint64_t glyphPagesMask() const { return m_usedGlyphCachePages; }

            INLINE uint32_t numStyles() const { return m_styles.size(); }
            INLINE const RenderStyle* styles() const { return m_styles.typedData(); }

            //--
             
            INLINE const Vector2& boundsMin() const { return m_vertexBoundsMin; }
            INLINE const Vector2& boundsMax() const { return m_vertexBoundsMax; }

            //--

            // remove all geometry from this container but do not free the memory
            void reset();

            // prepare glyphs for rendering - make sure glyphs are in cache and assign UVs if needed
            void prepareGlyphsForRendering();

        private:
            typedef Array<RenderVertex> TVertices;
            typedef Array<RenderPath> TPaths;
            typedef Array<RenderGroup> TGroups;
            typedef Array<RenderGlyph> TGlyphs;
            typedef Array<RenderStyle> TStyles;

            TVertices m_vertices;
            TPaths m_paths;
            TGroups m_groups;
            TGlyphs m_glyphs;
            TStyles m_styles;

            Vector2 m_vertexBoundsMin;
            Vector2 m_vertexBoundsMax;

            uint32_t m_glyphCacheVersion;
            uint64_t m_usedGlyphCachePages;
           
            void calcBounds();

            friend class GeometryBuilder;
        };

    } // canvas
} // base
