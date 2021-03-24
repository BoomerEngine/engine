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

#include "engine/font/include/font.h"
#include "engine/font/include/fontGlyphBuffer.h"
#include "engine/font/include/fontGlyphCache.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

GeometryBuilder::GeometryBuilder(CanvasGeometry& outGeometry)
	: CanvasGeometryBuilder(outGeometry)
{
    m_currentVerticalAlignment = FontAlignmentVertical::Top;
    m_currentHorizontalAlignment = FontAlignmentHorizontal::Left;
    m_currentSize = 12;
}

void GeometryBuilder::font(const FontPtr& fontPtr)
{
    m_currentFont = fontPtr;
}

void GeometryBuilder::textVerticalAlignment(const FontAlignmentVertical alignment)
{
    m_currentVerticalAlignment = alignment;
}

void GeometryBuilder::textHorizontalAlignment(const FontAlignmentHorizontal alignment)
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

uint32_t GeometryBuilder::textLineWidth(const FontInputText& text) const
{
    if (m_currentSize < 2 || !m_currentFont)
        return 0;

    FontStyleParams styleParams;
    styleParams.size = m_currentSize;
    FontAssemblyParams assemblyParams;
    assemblyParams.verticalAlignment = m_currentVerticalAlignment;
    assemblyParams.horizontalAlignment = m_currentHorizontalAlignment;

    FontMetrics metrics;
    m_currentFont->measureText(styleParams, assemblyParams, text, metrics);

    return metrics.textWidth;
}

void GeometryBuilder::drawText(const FontInputText& text)
{
    if (m_currentSize < 2 || !m_currentFont)
        return;

    FontStyleParams styleParams;
    styleParams.size = m_currentSize;
    FontAssemblyParams assemblyParams;
    assemblyParams.verticalAlignment = m_currentVerticalAlignment;
    assemblyParams.horizontalAlignment = m_currentHorizontalAlignment;

    FontGlyphBuffer glyphs;
    m_currentFont->renderText(styleParams, assemblyParams, text, glyphs);

    print(glyphs);      
}

END_BOOMER_NAMESPACE_EX(ui)
