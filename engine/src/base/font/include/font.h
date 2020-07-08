/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/system/include/mutex.h"

namespace base
{
    namespace font
    {
        //---

        /// font styling parameters, control how the font is pixelated into the bitmaps
        struct BASE_FONT_API FontStyleParams
        {
            uint32_t size = 10; // size in pixels
            float blur = 0.0f; // size of the blur
            bool bold = false; // make characters bold (additional width), NOTE: procedural effect, quality may vary
            bool italic = false; // make characters italic (slanted), NOTE: procedural effect, quality may vary

            // calculate unique hash describing this rendering setup
            uint32_t calcHash() const;

            FontStyleParams(uint32_t size = 10, bool bold = false, bool italic = false, float blur = 0.0f);
        };

        /// horizontal text alignment modes
        enum class FontAlignmentHorizontal : uint8_t
        {
            Left = 0,
            Center = 1,
            Right = 2,
        };

        /// vertical text alignment modes
        enum class FontAlignmentVertical : uint8_t
        {
            Top = 0,
            Middle = 1,
            Bottom = 2,
            Baseline = 3,
        };

        /// text generation parameters
        struct FontAssemblyParams
        {
            uint8_t interspace = 0; // additional space between characters in pixels
            FontAlignmentHorizontal horizontalAlignment = FontAlignmentHorizontal::Left; // alignment flags
            FontAlignmentVertical verticalAlignment = FontAlignmentVertical::Top; // alignment flags
        };

        //---

        /// font metrics
        struct FontMetrics
        {
            uint32_t lineHeight = 0; // vertical separation between text lines at given font size
            uint32_t textWidth = 0; // total width of the text at given font size
        };

        //---

        /// vector font, rendered via bitmaps
        class BASE_FONT_API Font : public res::IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Font, res::IResource);

        public:
            Font();
            Font(const Buffer& data);
            virtual ~Font();

            // get relative size of the upper part of the font (above baseline)
            INLINE float relativeAscender() const { return m_ascender; }

            // get relative size of lower part of the font (below baseline)
            INLINE float relativeDescender() const { return m_descender; }

            // get relative // relative line height scalar
            INLINE float relativeLineHeight() const { return m_lineHeight; }

            // get font glyph cache
            INLINE const GlyphCache& glyphCache() const { return *m_glyphCache; }

            // get raw font data (for use with different font systems)
            INLINE const Buffer& data() const { return m_packedData; }

            //--

            /// calculate line separation for given font size
            uint32_t lineSeparation(uint32_t fontSize) const;

            /// compute font metrics for given text
            void measureText(const FontStyleParams& styleParams, const FontAssemblyParams& textParams, const FontInputText& str, FontMetrics& outMetrics) const;

            /// generate renderable characters for this font
            /// printable characters are written in the render buffer 
            /// the images for particular glyphs needed to render the text are maintained in the provided glyph cache
            /// returns size of the text (the same as measureText)
            void renderText(const FontStyleParams& styleParams, const FontAssemblyParams& textParams, const FontInputText& str, GlyphBuffer& outBuffer) const;

            /// fetch information about single character
            const Glyph* renderGlyph(const FontStyleParams& styleParams, uint32_t glyphcode) const;

        private:
            Buffer m_packedData; // packed font data
            FT_Face m_face; // FreeType font data

            FontID m_id; // internal font ID

            float m_ascender; // upper part of the font (above baseline)
            float m_descender; // lower part of the font (below baseline)
            float m_lineHeight; // relative line height scalar

            UniquePtr<GlyphCache> m_glyphCache; // cache for glyphs created from this font

            void unregisterFont();
            void registerFont();
            virtual void onPostLoad();
        };

    } // font
} // base   