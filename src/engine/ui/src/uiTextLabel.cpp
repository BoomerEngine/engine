/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#include "build.h"
#include "uiTextLabel.h"
#include "uiTextLayout.h"
#include "uiGeometryBuilder.h"
#include "uiStyleValue.h"
#include "uiRenderer.h"

#include "engine/font/include/fontGlyphBuffer.h"
#include "engine/font/include/fontGlyph.h"
#include "core/containers/include/stringParser.h"
#include "core/containers/include/utf8StringFunctions.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

RTTI_BEGIN_TYPE_CLASS(TextLabel);
    RTTI_METADATA(ElementClassNameMetadata).name("TextLabel");
    RTTI_PROPERTY(m_text);
RTTI_END_TYPE();

TextLabel::TextLabel()
{
    if (!IsDefaultObjectCreation())
        m_displayData = new prv::LayoutDisplayData();
}

TextLabel::TextLabel(StringView txt)
    : m_text(txt)
{
    if (!IsDefaultObjectCreation())
        m_displayData = new prv::LayoutDisplayData();
}

TextLabel::~TextLabel()
{
    delete m_data;
    delete m_displayData;
}

void TextLabel::text(StringView text)
{
    if (m_text != text)
    {
        m_text = StringBuf(text);

        delete m_data;
        m_data = nullptr;

        invalidateStyle();
        invalidateGeometry();
        invalidateLayout();
    }
}

void TextLabel::highlight(int startPos, int endPos)
{
    // TODO
}

void TextLabel::resetHighlight()
{
    // TODO
}

void TextLabel::computeLayout(ElementLayout& outLayout)
{
    // get text to process
    auto text = m_text;
    if (const auto* content = evalStyleValueIfPresentPtr<StringBuf>("content"_id))
        text = *content;

    // create the layout data
    if (!m_data || m_data->text != text)
    {
        delete m_data;
        m_data = new prv::LayoutData(text);
    }

    // render data
    {
        const auto bold = style::FontWeight::Bold == evalStyleValue("font-weight"_id, style::FontWeight::Normal);
        const auto italic = style::FontStyle::Italic == evalStyleValue("font-style"_id, style::FontStyle::Normal);
        const auto size = std::max<uint32_t>(1, std::floorf(evalStyleValue<float>("font-size"_id, 14.0f) * cachedStyleParams().pixelScale));
        const auto fonts = evalStyleValue<style::FontFamily>("font-family"_id);

        Color color = Color::WHITE;
        if (auto textColorPtr = evalStyleValueIfPresentPtr<Color>("color"_id))
            color = *textColorPtr;

        m_data->render(renderer()->stash(), fonts, cachedStyleParams().pixelScale, size, bold, italic, color, -1.0f, *m_displayData);
    }

    TBaseClass::computeLayout(outLayout);
}

void TextLabel::computeSize(Size& outSize) const
{
    TBaseClass::computeSize(outSize);
    m_displayData->measure(outSize);
}

void TextLabel::prepareShadowGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, CanvasGeometryBuilder& builder) const
{
    TBaseClass::prepareShadowGeometry(stash, drawArea, pixelScale, builder);
}

namespace helper
{
    struct GlyphBufferEntryEx
    {
        GlyphBufferEntryEx(const font::GlyphBufferEntry &entry, const font::Font* font, float fontSize)
        {
            // determine the line bounding box
            auto ascenderLine = font->relativeAscender() * fontSize;
            auto descenderLine = font->relativeDescender() * fontSize;
            auto lineHeight = font->relativeLineHeight() * fontSize;

            // set placement, always
            m_pos = entry.pos;
            if (entry.glyph != nullptr)
            {
                m_pos.x = (float) (int) (m_pos.x - entry.glyph->offset().x);
                m_pos.y = (float) (int) (m_pos.y - entry.glyph->offset().y);
            }

            m_pos.y -= ascenderLine;

            // set bounding box
            if (entry.glyph != nullptr)
            {
                auto rect = entry.glyph->logicalRect();
                m_boxMin.x = m_pos.x;
                m_boxMin.y = m_pos.y;
                m_boxMax.x = m_pos.x + entry.glyph->advance().x;
                m_boxMax.y = m_pos.y + lineHeight;
            }
            else
            {
                m_boxMin.x = m_pos.x + 0.0f;
                m_boxMin.y = m_pos.y + 0.0f;
                m_boxMax.x = m_pos.x + 5.0f;
                m_boxMax.y = m_pos.y + lineHeight;
            }
        }

        Vector2 m_pos;
        Vector2 m_boxMin;
        Vector2 m_boxMax;
    };

} // helper

void GenerateHighlight()
{
    /*// add highlight range
    if (m_highlightStartPos != m_highlightEndPos)
    {
        Color highlightColor = Color::WHITE;
        if (auto highlightColorPtr = evalStyleValueIfPresentPtr<style::RenderStyle>("highlight"_id))
            highlightColor = highlightColorPtr->innerColor;
        if (highlightColor.a > 0)
        {
            auto rangeStart = std::clamp(std::min(m_highlightStartPos, m_highlightEndPos), 0, (int)glyphs.size());
            auto rangeEnd = std::clamp(std::max(m_highlightStartPos, m_highlightEndPos), 0, (int)glyphs.size());
            if (rangeEnd > rangeStart)
            {
                // generate the selection area
                int cur = rangeStart;
                while (cur < rangeEnd)
                {
                    helper::GlyphBufferEntryEx rowGlyph(glyphs.glyphs()[cur], font.get(), (float)styleParams.size);

                    auto rowStart = cur;
                    auto rowMinY = rowGlyph.m_boxMin.y;
                    auto rowMaxY = rowGlyph.m_boxMax.y;
                    auto rowStartX = rowGlyph.m_boxMin.x;
                    auto rowX = rowStartX;

                    // scan till the end row
                    while (cur < rangeEnd)
                    {
                        helper::GlyphBufferEntryEx curGlyph(glyphs.glyphs()[cur], font.get(), (float)styleParams.size);

                        if (curGlyph.m_boxMin.y != rowMinY || curGlyph.m_boxMax.y != rowMaxY || curGlyph.m_boxMin.x != rowX)
                            break;

                        rowX = curGlyph.m_boxMax.x;
                        ASSERT(rowX >= rowStartX);
                        ++cur;
                    }

                    // add box
                    builder.builder().beginPath();
                    builder.builder().fillColor(highlightColor);
                    builder.builder().rect(rowStartX, rowMinY, rowX - rowStartX, rowMaxY - rowMinY);
                    builder.builder().fill();
                }
            }
        }
    }*/
}

void TextLabel::prepareForegroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, CanvasGeometryBuilder& builder) const
{
    TBaseClass::prepareForegroundGeometry(stash, drawArea, pixelScale, builder);

    auto align = prv::LayoutHorizontalAlign::Left;
    switch (cachedLayoutParams().m_horizontalAlignment)
    {
    case ElementHorizontalLayout::Center: 
        align = prv::LayoutHorizontalAlign::Center;
        break;
    case ElementHorizontalLayout::Right:
        align = prv::LayoutHorizontalAlign::Right;
        break;
    }

    m_displayData->render(builder, drawArea, align);
}

//--

END_BOOMER_NAMESPACE_EX(ui)
