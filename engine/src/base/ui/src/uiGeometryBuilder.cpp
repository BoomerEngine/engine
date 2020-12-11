/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
*/

#include "build.h"
#include "uiGeometryBuilder.h"
#include "uiStyleLibrary.h"

#include "base/font/include/font.h"
#include "base/font/include/fontGlyphBuffer.h"
#include "base/font/include/fontGlyphCache.h"

namespace ui
{

    GeometryBuilder::GeometryBuilder(const base::canvas::IStorage* storage, base::canvas::Geometry& outGeometry)
		: base::canvas::GeometryBuilder(storage, outGeometry)
    {
        m_currentVerticalAlignment = base::font::FontAlignmentVertical::Top;
        m_currentHorizontalAlignment = base::font::FontAlignmentHorizontal::Left;
        m_currentSize = 12;
    }

    void GeometryBuilder::font(const base::FontPtr& fontPtr)
    {
        m_currentFont = fontPtr;
    }

    void GeometryBuilder::textVerticalAlignment(const base::font::FontAlignmentVertical alignment)
    {
        m_currentVerticalAlignment = alignment;
    }

    void GeometryBuilder::textHorizontalAlignment(const base::font::FontAlignmentHorizontal alignment)
    {
        m_currentHorizontalAlignment = alignment;
    }

    void GeometryBuilder::fontSize(uint32_t size)
    {
        m_currentSize = size;
    }

    uint32_t GeometryBuilder::textLineHeight() const
    {
        if (!m_currentFont)
            return 0;

        return (uint32_t)m_currentFont->lineSeparation(m_currentSize);
    }

    uint32_t GeometryBuilder::textLineWidth(const base::font::FontInputText& text) const
    {
        if (m_currentSize < 2 || !m_currentFont)
            return 0;

        base::font::FontStyleParams styleParams;
        styleParams.size = m_currentSize;
        base::font::FontAssemblyParams assemblyParams;
        assemblyParams.verticalAlignment = m_currentVerticalAlignment;
        assemblyParams.horizontalAlignment = m_currentHorizontalAlignment;

        base::font::FontMetrics metrics;
        m_currentFont->measureText(styleParams, assemblyParams, text, metrics);

        return metrics.textWidth;
    }

    void GeometryBuilder::drawText(const base::font::FontInputText& text)
    {
        if (m_currentSize < 2 || !m_currentFont)
            return;

        base::font::FontStyleParams styleParams;
        styleParams.size = m_currentSize;
        base::font::FontAssemblyParams assemblyParams;
        assemblyParams.verticalAlignment = m_currentVerticalAlignment;
        assemblyParams.horizontalAlignment = m_currentHorizontalAlignment;

        base::font::GlyphBuffer glyphs;
        m_currentFont->renderText(styleParams, assemblyParams, text, glyphs);

        print(glyphs);      
    }

} // ui