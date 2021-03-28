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
{}

void FontGlyphBuffer::reset()
{
    m_cursor.x = 0;
    m_cursor.y = 0;
    m_currentLineAscender = 0;
    m_currentLineStart = 0;
    m_firstLine = true;
    m_glyphs.reset();
}

void FontGlyphBuffer::newLine(int currentDescender)
{
    int maxDescender = currentDescender;
    for (auto i = m_currentLineStart; i < m_glyphs.size(); ++i)
    {
        const auto& g = m_glyphs[i];
        if (g.glyph->descender() < maxDescender)
            maxDescender = g.glyph->descender();
    }

    m_cursor.x = 0;
    m_cursor.y -= maxDescender;
    m_cursor.y += 4; // line gap, hack

    m_currentLineStart = m_glyphs.size();
    m_currentLineAscender = 0;
    m_firstLine = false;
}

void FontGlyphBuffer::advance(int dx, int dy)
{
    m_cursor.x += dx;
    m_cursor.y += dy;
}

bool FontGlyphBuffer::bounds(Rect& outBounds) const
{
    if (m_glyphs.empty())
        return false;

    const auto& g = m_glyphs.front();
    outBounds.min.x = g.pos.x - g.glyph->offset().x;
    outBounds.max.x = g.pos.x - g.glyph->offset().x + g.glyph->advance().x;
    outBounds.min.y = g.pos.y - g.glyph->offset().y - g.glyph->ascender();
    outBounds.max.y = g.pos.y - g.glyph->offset().y - g.glyph->descender();

    for (const auto& g : m_glyphs)
    {
        outBounds.min.x = std::min<int>(g.pos.x - g.glyph->offset().x, outBounds.min.x);
        outBounds.max.x = std::max<int>(g.pos.x - g.glyph->offset().x + g.glyph->advance().x, outBounds.max.x);
        outBounds.min.y = std::min<int>(g.pos.y - g.glyph->offset().y - g.glyph->ascender(), outBounds.min.y);
        outBounds.max.y = std::max<int>(g.pos.y - g.glyph->offset().y - g.glyph->descender(), outBounds.max.y);
    }

    return true;
}

void FontGlyphBuffer::shift(int dx, int dy)
{
    for (auto& g : m_glyphs)
    {
        g.pos.x += dx;
        g.pos.y += dy;
    }
}

void FontGlyphBuffer::push(uint32_t ch, const FontGlyph* glyph, int textPosition, Color color)
{
    auto shiftY = glyph->ascender() - m_currentLineAscender;
    if (shiftY > 0 && !m_firstLine)
    {
        for (auto i = m_currentLineStart; i < m_glyphs.size(); ++i)
        {
            auto& g = m_glyphs[i];
            g.pos.y += shiftY;
        }

        m_currentLineAscender = glyph->ascender();
        m_cursor.y += shiftY;
        shiftY = 0;
    }

    auto& entry = m_glyphs.emplaceBack();
    entry.pos.x = m_cursor.x + glyph->offset().x;
    entry.pos.y = m_cursor.y + glyph->offset().y;
    entry.glyph = glyph;
    entry.textPosition = textPosition;
    entry.color = color;

    m_cursor.x += (int)glyph->advance().x;
    m_cursor.y += (int)glyph->advance().y;
}

END_BOOMER_NAMESPACE()
