/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#include "engine/font/include/font.h"
#include "engine/font/include/fontInputText.h"
#include "engine/canvas/include/geometryBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

/// extension of the canvas geometry builder, contains support for fonts and icons from atlas
class ENGINE_UI_API GeometryBuilder : public CanvasGeometryBuilder
{
public:
    GeometryBuilder(CanvasGeometry& outGeometry);

    //---

    // Select font by object
    void font(const FontPtr& fontPtr);

    // Set vertical text alignment mode
    void textVerticalAlignment(const FontAlignmentVertical alignment);

    // Set horizontal text alignment mode
    void textHorizontalAlignment(const FontAlignmentHorizontal alignment);

    // Select font size
    void fontSize(uint32_t size);

    // Get size of line of text under currently selected fonts
    uint32_t textLineHeight() const;

    // Get width of line of text under currently selected fonts
    uint32_t textLineWidth(const FontInputText& text) const;

    // Print text, uses only the fill color and current
    void drawText(const FontInputText& text);

    //---

private:
    FontPtr m_currentFont; // current font
    FontAlignmentVertical m_currentVerticalAlignment;
    FontAlignmentHorizontal m_currentHorizontalAlignment;
    uint32_t m_currentSize;
};

END_BOOMER_NAMESPACE_EX(ui)
