/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#include "build.h"
#include "fontGlyph.h"

#include "core/image/include/image.h"
#include "core/image/include/imageView.h"

BEGIN_BOOMER_NAMESPACE_EX(font)

Glyph::Glyph(const GlyphID& id, const ImagePtr& imagePtr, const Point& offset, const Point& size, const Vector2& advance, const Rect& logicalRect)
    : m_id(id)
    , m_offset(offset)
    , m_size(size)
    , m_advance(advance)
    , m_bitmap(imagePtr)
    , m_logicalRect(logicalRect)
{}

uint32_t Glyph::calcMemoryUsage() const
{
    uint32_t size = sizeof(Glyph);
    size += m_bitmap->view().dataSize();
    return size;
}

END_BOOMER_NAMESPACE_EX(font)
