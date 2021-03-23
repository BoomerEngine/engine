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

BEGIN_BOOMER_NAMESPACE_EX(font)

/// placement information for renderable glyph
struct GlyphBufferEntry
{
    const Glyph* glyph = nullptr; // glyph to render, may be empty for white space/non renderable characters
    Vector2 pos; // placement for rendering
    Color color = Color::WHITE;
    int textPosition = 0; // original text position for this glyph
};

/// glyph buffer - output of text rendering via font
/// contains placement informations of individual glyphs that make up the font
class ENGINE_FONT_API GlyphBuffer : public NoCopy
{
public:
    GlyphBuffer();
    
    /// reset content
    INLINE void reset(const Vector2& startPos)
    { 
        m_glyphs.clear();
        m_bounds = Rect::EMPTY();
    }

    /// get the bounding box of the text
    INLINE Rect bounds() const
    {
        return m_bounds;
    }

    /// get number of glyphs
    INLINE uint32_t size() const
    {
        return m_glyphs.size();
    }

    /// get the glyph data
    INLINE const GlyphBufferEntry* glyphs() const
    {
        return m_glyphs.typedData();
    }

    /// get fonts
    INLINE const Array<RefPtr<Font>>& fonts() const
    {
        return m_fonts;
    }

    /// add glyph to buffer
    void addGlyph(uint32_t ch, const Glyph* glyph, float x, float y, int textPosition, Color color = Color::WHITE);

    /// add font
    void addFont(const RefPtr<Font>& font);

private:
    typedef Array<GlyphBufferEntry> TGlyphs;
    TGlyphs m_glyphs;

    typedef Array<RefPtr<Font>> TFonts;
    TFonts m_fonts;

    Rect m_bounds;
};

END_BOOMER_NAMESPACE_EX(font)
