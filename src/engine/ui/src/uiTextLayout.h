/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#pragma once

#include "engine/font/include/fontGlyphBuffer.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

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
        StringID imageName;

        uint16_t firstCode = 0;
        uint16_t numCodes = 0;
        LayoutVerticalAlign valign = LayoutVerticalAlign::Baseline;

        Color color;
        Color tagColor;
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

    struct LayoutData : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_UI_CANVAS)

    public:
        StringBuf text;
        Array<LayoutRegion> regions;
        Array<LayoutLine> lines;
        Array<uint32_t> codes;

        LayoutData(const StringBuf& text);

        void render(DataStash& stash, const style::FontFamily& fonts, float pixelScale, float fontSize, bool bold, bool italic, Color color, float wrapWidth, LayoutDisplayData& outData) const;
    };


    //--

    struct LayoutDisplayGlyph
    {
        FontGlyphBufferEntry data;
    };

    struct LayoutDisplayImage
    {
        CanvasImageEntry image;
        LayoutVerticalAlign valaign = LayoutVerticalAlign::Baseline;
        Color color;
        float pixelScale = 1.0f;
        Vector2 pos;
    };

    struct LayoutDisplayTag
    {
        float x0 = 0.0f;
        float x1 = 0.0f;
        float y0 = 0.0f;
        float y1 = 0.0f;
        float r = 0.0f;
        Color color;
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

    struct LayoutDisplayData : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_UI_CANVAS)

    public:
        Array<LayoutDisplayGlyph> glyphs;
        Array<LayoutDisplayLine> lines;
        Array<LayoutDisplayImage> images;
        Array<LayoutDisplayTag> tags;

        void render(CanvasGeometryBuilder& b, const ElementArea& area, LayoutHorizontalAlign defaultAlignment = LayoutHorizontalAlign::Left) const;
        void measure(Size& outSize) const; // optimal size
    };

    //--

} // prv

END_BOOMER_NAMESPACE_EX(ui)
