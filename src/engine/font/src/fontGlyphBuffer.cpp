/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#include "build.h"
#include "fontGlyph.h"
#include "fontGlyphBuffer.h"

BEGIN_BOOMER_NAMESPACE()

FontGlyphBuffer::FontGlyphBuffer()
    : m_bounds(Rect::EMPTY())
{}

void FontGlyphBuffer::addFont(const RefPtr<Font>& font)
{
    m_fonts.pushBackUnique(font);
}

void FontGlyphBuffer::addGlyph(uint32_t ch, const FontGlyph* glyph, float x, float y, int textPosition, Color color)
{
    if (glyph != nullptr)
    {
        Vector2 renderPos;
        renderPos.x = (float)(int)(x) + glyph->offset().x;
        renderPos.y = (float)(int)(y) + glyph->offset().y;

        m_glyphs.pushBack({ glyph, renderPos, color, textPosition });

        Rect drawRect;
        drawRect.min.x = (int)(renderPos.x);
        drawRect.min.y = (int)(renderPos.y);
        drawRect.max.x = drawRect.min.x + glyph->size().x;
        drawRect.max.y = drawRect.min.y + glyph->size().y;
        m_bounds.merge(drawRect);
    }
}

END_BOOMER_NAMESPACE()
