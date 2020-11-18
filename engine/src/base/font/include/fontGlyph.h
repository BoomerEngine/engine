/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#pragma once

#include "base/math/include/point.h"
#include "base/image/include/image.h"

namespace base
{
    namespace font
    {
        /// glyph identification data
        struct GlyphID
        {
            FontID fontId = 0; // id of the font itself, we don't have that many fonts so this is just a number
            FontGlyphID glyphId = 0;  // glyph ID, usually this is just the wchar_t
            FontStyleHash style = 0; // render hash of the parameters the glyph was rendered with

            INLINE GlyphID(FontID fontId, FontGlyphID glyphId, FontStyleHash style)
                : fontId(fontId)
                , glyphId(glyphId)
                , style(style)
            {}

            INLINE bool operator==(const GlyphID& other) const
            {
                return (fontId == other.fontId) && (glyphId == other.glyphId) && (style == other.style);
            }

            INLINE bool operator<(const GlyphID& other) const
            {
                if (fontId < other.fontId) return true;
                if (fontId > other.fontId) return false;
                if (glyphId < other.glyphId) return true;
                if (glyphId > other.glyphId) return false;
                return style < other.style;
            }

            INLINE static uint32_t CalcHash(const GlyphID& id) 
            {
                uint64_t ret = id.fontId;
                ret ^= id.glyphId;
                ret ^= id.style;
                return ret;
            }
        };

        /// renderable font glyph, managed by the glyph cache
        /// internally the glyph is just a simple image bitmap and sizing information
        /// NOTE: for performance reasons the glyphs are not memory managed via the SharedPtr, do not store pointers to them
        class BASE_FONT_API Glyph : public NoCopy
        {
            RTTI_DECLARE_POOL(POOL_FONTS)

        public:
            Glyph(const GlyphID& id, const image::ImagePtr& imagePtr, const Point& offset, const Point& size, const Vector2& advance, const Rect& logicalRect);

            // get the ID of this glyph
            INLINE const GlyphID& id() const { return m_id; }

            // get the generated glyph image
            INLINE const image::ImagePtr& bitmap() const { return m_bitmap; }

            // get the placement offset for the bitmap content with respect to the cursor position
            INLINE const Point& offset() const { return m_offset; }

            // get the size of the glyph bitmap
            INLINE const Point& size() const { return m_size; }

            // get the logical rectangle for the glyph, does not include glyph effects or padding on the bitmap
            INLINE const Rect& logicalRect() const { return m_logicalRect; }

            // get the advance to next rendering position after this glyph is rendered
            INLINE const Vector2& advance() const { return m_advance; }

            //--

            // compute memory consumed by this glyph
            uint32_t calcMemoryUsage() const;

        private:
            GlyphID m_id; // id of the glyph, unique in the system
            
            Point m_offset; // placement offset for the bitmap content with respect to the cursor position
            Point m_size; // size of the renderable area of the glyph (NOTE: this is the size of the bitmap)
            Vector2 m_advance; // advance to next rendering position after this glyph is rendered, NOTE: sub-pixel rendering and kerning is already accounted for here
            Rect m_logicalRect; // get the logical rectangle for the glyph

            image::ImagePtr m_bitmap; // rendered glyph bitmap
        };

    } // font
} // base
