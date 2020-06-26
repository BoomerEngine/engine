/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#pragma once

#include "base/font/include/fontGlyphBuffer.h"

namespace ui
{
    namespace style
    {
        struct FontFamily;
    } // style

    namespace prv
    {

        //--

        enum class LayoutVerticalAlign : uint8_t
        {
            Baseline = 0, // align at the base line - typical for text
            Top = 1,
            Middle = 2, // middle of the line range
            Bottom = 3, 
        };

        enum class LayoutHorizontalAlign : uint8_t
        {
            Default = 0, // align the same way the whole text block is aligned
            Left = 1,
            Center = 2,
            Right = 3,
        };

        struct LayoutRegion
        {
            base::StringID imageName;

            uint16_t firstCode = 0;
            uint16_t numCodes = 0;
            LayoutVerticalAlign valign = LayoutVerticalAlign::Baseline;

            base::Color color;
            base::Color tagColor;
            float size = 1.0f;
            bool flagBold : 1;
            bool flagItalic : 1;
            bool flagColor : 1;
            bool flagTag : 1;
            bool flagImage : 1;

            INLINE LayoutRegion()
            {
                flagBold = 0;
                flagImage = 0;
                flagItalic = 0;
                flagColor = 0;
                flagTag = 0;
            }
        };

        struct LayoutLine
        {
            uint16_t firstRegion = 0;
            uint16_t numRegions = 0;
            LayoutHorizontalAlign halaign = LayoutHorizontalAlign::Default;
        };

        struct LayoutData
        {
            base::StringBuf text;
            base::Array<LayoutRegion> regions;
            base::Array<LayoutLine> lines;
            base::Array<uint32_t> codes;

            LayoutData(const base::StringBuf& text);

            void render(DataStash& stash, const style::FontFamily& fonts, float pixelScale, float fontSize, bool bold, bool italic, base::Color color, float wrapWidth, LayoutDisplayData& outData) const;
        };


        //--

        struct LayoutDisplayGlyph
        {
            base::font::GlyphBufferEntry data;
        };

        struct LayoutDisplayImage
        {
            const base::image::Image* image = nullptr;
            LayoutVerticalAlign valaign = LayoutVerticalAlign::Baseline;
            base::Color color;
            base::Vector2 pos;
        };

        struct LayoutDisplayTag
        {
            float x0 = 0.0f;
            float x1 = 0.0f;
            float y0 = 0.0f;
            float y1 = 0.0f;
            float r = 0.0f;
            base::Color color;
        };

        struct LayoutDisplayLine
        {
            float y = 0.0f;
            uint16_t firstGlyph = 0;
            uint16_t numGlyphs = 0;
            uint16_t firstImage = 0;
            uint16_t numImages = 0;
            uint16_t firstTag = 0;
            uint16_t numTags = 0;
            float width = 0.0f;
            float height = 0.0f;
            LayoutHorizontalAlign alignment = LayoutHorizontalAlign::Default;
        };

        struct LayoutDisplayData
        {
            base::Array<LayoutDisplayGlyph> glyphs;
            base::Array<LayoutDisplayLine> lines;
            base::Array<LayoutDisplayImage> images;
            base::Array<LayoutDisplayTag> tags;

            void render(base::canvas::GeometryBuilder& b, const ElementArea& area, LayoutHorizontalAlign defaultAlignment = LayoutHorizontalAlign::Left) const;
            void measure(Size& outSize) const; // optimal size
        };

        //--

    } // prv
} // ui