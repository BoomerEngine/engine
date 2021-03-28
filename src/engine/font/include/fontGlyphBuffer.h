/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#pragma once

#include "core/math/include/point.h"
#include "engine/font/include/font.h"

BEGIN_BOOMER_NAMESPACE()

/// placement information for renderable glyph
struct FontGlyphBufferEntry
{
    const FontGlyph* glyph = nullptr; // glyph to render, may be empty for white space/non renderable characters
    Point pos; // placement for rendering
    Color color = Color::WHITE;
    int textPosition = 0; // original text position for this glyph

};

/// glyph buffer - output of text rendering via font
/// contains placement informations of individual glyphs that make up the font
class ENGINE_FONT_API FontGlyphBuffer : public NoCopy
{
public:
    FontGlyphBuffer();

    //--

    void reset();

    void push(uint32_t ch, const FontGlyph* glyph, int textPosition, Color color = Color::WHITE);

    void advance(int dx, int dy);

    void shift(int dx, int dy);

    void newLine(int currentDescender);

    bool bounds(Rect& outBounds) const;

    //--

    Point m_cursor;

    typedef Array<FontGlyphBufferEntry> TGlyphs;
    TGlyphs m_glyphs;

    int m_currentLineStart = 0;
    int m_currentLineAscender = 0;
    bool m_firstLine = true;
};

END_BOOMER_NAMESPACE()
