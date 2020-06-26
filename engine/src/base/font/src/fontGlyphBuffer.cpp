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

namespace base
{
    namespace font
    {

        GlyphBuffer::GlyphBuffer()
            : m_bounds(Rect::EMPTY())
        {}

        void GlyphBuffer::addFont(const RefPtr<Font>& font)
        {
            m_fonts.pushBackUnique(font);
        }

        void GlyphBuffer::addGlyph(uint32_t ch, const Glyph* glyph, float x, float y, int textPosition)
        {
            if (glyph != nullptr)
            {
                Vector2 renderPos;
                renderPos.x = (float)(int)(x) + glyph->offset().x;
                renderPos.y = (float)(int)(y) + glyph->offset().y;

                m_glyphs.pushBack({ glyph, renderPos, base::Color::WHITE, textPosition });

                Rect drawRect;
                drawRect.min.x = (int)(renderPos.x);
                drawRect.min.y = (int)(renderPos.y);
                drawRect.max.x = drawRect.min.x + glyph->size().x;
                drawRect.max.y = drawRect.min.y + glyph->size().y;
                m_bounds.merge(drawRect);
            }
        }

    } // font
} // base

