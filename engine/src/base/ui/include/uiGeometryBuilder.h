/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#include "base/font/include/font.h"
#include "base/font/include/fontInputText.h"
#include "base/canvas/include/canvasGeometryBuilder.h"

namespace ui
{
    /// extension of the canvas geometry builder, contains support for fonts and icons from atlas
    class BASE_UI_API GeometryBuilder : public base::canvas::GeometryBuilder
    {
    public:
        GeometryBuilder(const base::canvas::IStorage* storage, base::canvas::Geometry& outGeometry);

        //---

        // Select font by object
        void font(const base::FontPtr& fontPtr);

        // Set vertical text alignment mode
        void textVerticalAlignment(const base::font::FontAlignmentVertical alignment);

        // Set horizontal text alignment mode
        void textHorizontalAlignment(const base::font::FontAlignmentHorizontal alignment);

        // Select font size
        void fontSize(uint32_t size);

        // Get size of line of text under currently selected fonts
        uint32_t textLineHeight() const;

        // Get width of line of text under currently selected fonts
        uint32_t textLineWidth(const base::font::FontInputText& text) const;

        // Print text, uses only the fill color and current
        void drawText(const base::font::FontInputText& text);

        //---

    private:
        base::FontPtr m_currentFont; // current font
        base::font::FontAlignmentVertical m_currentVerticalAlignment;
        base::font::FontAlignmentHorizontal m_currentHorizontalAlignment;
        uint32_t m_currentSize;
    };

} // ui