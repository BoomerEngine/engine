/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "canvasGeometry.h"
#include "canvasGlyphCache.h"

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

namespace base
{
    namespace canvas
    {
        //---

        RTTI_BEGIN_TYPE_ENUM(CompositeOperation);
            RTTI_ENUM_OPTION(Copy);
            RTTI_ENUM_OPTION(Blend);
            RTTI_ENUM_OPTION(SourceOver);
            RTTI_ENUM_OPTION(SourceIn);
            RTTI_ENUM_OPTION(SourceOut);
            RTTI_ENUM_OPTION(SourceAtop);
            RTTI_ENUM_OPTION(DestinationOver);
            RTTI_ENUM_OPTION(DestinationIn);
            RTTI_ENUM_OPTION(DestinationOut);
            RTTI_ENUM_OPTION(DestinationAtop);
            RTTI_ENUM_OPTION(Addtive);
            RTTI_ENUM_OPTION(Xor);
        RTTI_END_TYPE();

        //---

        Geometry::Geometry()
            : m_vertexBoundsMin(FLT_MAX, FLT_MAX)
            , m_vertexBoundsMax(-FLT_MAX, -FLT_MAX)
            , m_glyphCacheVersion(0)
            , m_usedGlyphCachePages(0)
        {}

        Geometry::~Geometry()
        {}

        void Geometry::reset()
        {
            m_vertexBoundsMin = Vector2(FLT_MAX, FLT_MAX);
            m_vertexBoundsMax = Vector2(-FLT_MAX, -FLT_MAX);

            m_glyphCacheVersion = 0;
            m_usedGlyphCachePages = 0;

            m_vertices.reset();
            m_paths.reset();
            m_groups.reset();
            m_glyphs.reset();
            m_styles.reset();
        }

        void Geometry::prepareGlyphsForRendering()
        {
            if (!m_glyphs.empty())
            {
                auto& cache = GlyphCache::GetInstance();

                if (cache.beingUpdate(m_glyphCacheVersion))
                {
                    m_usedGlyphCachePages = 0;

                    for (auto& group : m_groups)
                    {
                        auto* g = group.glyphs;
                        auto* endG = group.glyphs + group.numGlyphs;
                        while (g < endG)
                        {
                            auto mappedGlyph = cache.mapGlyph(*g->glyph);

                            g->uvMin = mappedGlyph.uvStart;
                            g->uvMax = mappedGlyph.uvEnd;
                            g->page = mappedGlyph.pageIndex;

                            m_usedGlyphCachePages |= 1ULL << mappedGlyph.pageIndex;
                            
                            ++g;
                        }
                    }

                    DEBUG_CHECK_EX(0 != m_usedGlyphCachePages, "Geometry has glyphs but no glyph pages were referenced");

                    cache.endUpdate();
                }
            }
        }

		void Geometry::calcBounds()
		{
            for (auto& g : m_groups)
            {
                m_vertexBoundsMin = Min(m_vertexBoundsMin, g.vertexBoundsMin);
                m_vertexBoundsMax = Max(m_vertexBoundsMax, g.vertexBoundsMax);
            }
		}

    } // canvas
} // base