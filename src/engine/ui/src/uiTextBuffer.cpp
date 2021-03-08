/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#include "build.h"
#include "uiTextBuffer.h"
#include "uiStyleValue.h"

#include "engine/font/include/fontGlyph.h"
#include "engine/font/include/fontGlyphBuffer.h"
#include "core/containers/include/stringBuilder.h"
#include "engine/canvas/include/canvas.h"
#include "core/containers/include/utf8StringFunctions.h"
#include "engine/canvas/include/geometryBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

TextBuffer::TextBuffer(bool multiline)
    : m_lastPos(0)
    , m_cursorPos(0)
    , m_selectionStartPos(0)
    , m_selectionEndPos(0)
    , m_highlightStartPos(0)
    , m_highlightEndPos(0)
    , m_fontSize(10)
    , m_multiline(multiline)
    , m_wrapWidth(0)
    , m_finalGlyph(0)
{
    m_chars.reserve(128);
    generateLayout();
}

TextBuffer::~TextBuffer()
{
}

StringBuf TextBuffer::text() const
{
    return text(0, m_lastPos);
}

StringBuf TextBuffer::text(int startPos, int endPos) const
{
    // validate range
    auto rangeStart = std::clamp(std::min(startPos, endPos), 0, m_lastPos);
    auto rangeEnd = std::clamp(std::max(startPos, endPos), 0, m_lastPos);
    if (rangeEnd <= rangeStart)
        return StringBuf();

    // calculate size required for UTF8 buffer
    uint32_t bufferSize = 1;
    for (auto pos = rangeStart; pos < rangeEnd; ++pos)
    {
        char temp[6];

        auto code = m_chars[pos].m_code;
        bufferSize += utf8::ConvertChar(temp, code);
    }

    // create the output buffer
    Array<char> tempBuffer;
    tempBuffer.resize(bufferSize);

    // write UTF8 chars
    auto* writePtr = tempBuffer.typedData();
    for (auto pos = rangeStart; pos < rangeEnd; ++pos)
    {
        auto code = m_chars[pos].m_code;
        writePtr += utf8::ConvertChar(writePtr, code);
    }
    *writePtr = 0; // zero terminate

    // return as string
    return StringBuf(tempBuffer.typedData());
}
    
void TextBuffer::style(const IElement* styleOwner)
{
    auto prevFont = m_font;
    auto prevSize = m_fontSize;

    if (auto style = styleOwner->evalStyleValueIfPresentPtr<style::FontFamily>("font-family"_id))
    {
        m_font = style->normal;
        m_fontSize = std::max<uint32_t>(1, std::floorf(styleOwner->evalStyleValue<float>("font-size"_id, 14.0f) * styleOwner->cachedStyleParams().pixelScale));
    }

    if (const auto* stylePtr = styleOwner->evalStyleValueIfPresentPtr<Color>("color"_id))
        m_textColor = *stylePtr;

    if (const auto* stylePtr = styleOwner->evalStyleValueIfPresentPtr<Color>("selection"_id))
        m_selectionColor = *stylePtr;

    if (const auto* stylePtr = styleOwner->evalStyleValueIfPresentPtr<Color>("highlight"_id))
        m_hightlightColor = *stylePtr;

    if (m_font != prevFont || m_fontSize != prevSize)
    {
        invalidateGeometry();
        generateLayout();
    }
}

void TextBuffer::invalidateGeometry()
{
    m_cachedCursorGeometry.reset();
    m_cachedSelectionGeometry.reset();
    m_cachedHighlightGeometry.reset();

    for (auto& ch : m_chars)
        ch.glyph = nullptr;
    for (auto& ch : m_postfixChars)
        ch.glyph = nullptr;
    for (auto& ch : m_prefixChars)
        ch.glyph = nullptr;
}

void TextBuffer::wrapWidth(uint32_t wrapWidth)
{
    if (m_wrapWidth != wrapWidth)
    {
        m_wrapWidth = wrapWidth;
        generateLayout();
    }
}

void TextBuffer::text(StringView txt)
{
    // extract chars from the string
    m_chars.reset();
    generateChars(txt, m_chars);

    // reset selection
    m_lastPos = m_chars.size();
    m_cursorPos = m_chars.size();
    m_selectionEndPos = m_lastPos;
    m_selectionStartPos = m_lastPos;
    m_modified = false;

    // render the visuals of the text
    generateLayout();
}

void TextBuffer::postfixText(StringView txt)
{
    // generate the chars
    m_postfixChars.clear();
    generateChars(txt, m_postfixChars);

    // render the visuals of the text
    generateLayout();
}

void TextBuffer::prefixText(StringView txt)
{
    // generate the chars
    m_prefixChars.clear();
    generateChars(txt, m_prefixChars);

    // render the visuals of the text
    generateLayout();
}

CharacterBounds TextBuffer::characterBounds(int index) const
{
    if (index >= 0 && index <= m_lastPos)
        return charAt(index).m_bounds;
    return CharacterBounds();
}

CursorNavigation TextBuffer::findCursorPosition(CursorNavigation currentPosition, TextNavigation mode) const
{
    switch (mode)
    {
        case TextNavigation::DocumentStart:
        {
            return 0;
        }

        case TextNavigation::DocumentEnd:
        {
            return m_lastPos;
        }

        case TextNavigation::LineStart:
        {
            if (!m_multiline)
                return 0;

            int pos = currentPosition.m_char;
            while (pos > 0)
            {
                if (m_chars[pos - 1].m_code == '\n')
                    break;
                pos -= 1;
            }
            return pos;
        }

        case TextNavigation::PrevLineChar:
        {
            if (!m_multiline)
                return currentPosition;

            auto posX = charAt(currentPosition.m_char).m_bounds.m_boxMin.x + currentPosition.m_pixelOffset;

            auto thisLineStart = findCursorPosition(currentPosition, TextNavigation::LineStart).m_char;
            if (thisLineStart == 0)
                return currentPosition;

            auto prevLineStart = findCursorPosition(thisLineStart-1, TextNavigation::LineStart).m_char;
            auto pos = thisLineStart - 1;
            while (pos > prevLineStart)
            {
                const auto& ch = m_chars[pos];

                if (ch.m_bounds.m_boxMin.x <= posX)
                    break;

                pos -= 1;
            }

            return CursorNavigation(pos, posX - charAt(pos).m_bounds.m_boxMin.x);
        }

        case TextNavigation::LineEnd:
        {
            if (!m_multiline)
                return m_lastPos;

            int pos = currentPosition.m_char;
            while (pos < m_lastPos)
            {
                if (m_chars[pos].m_code == '\n')
                    break;
                pos += 1;
            }
            return pos;
        }

        case TextNavigation::NextLineChar:
        {
            if (!m_multiline)
                return currentPosition;

            auto posX = charAt(currentPosition.m_char).m_bounds.m_boxMin.x + currentPosition.m_pixelOffset;

            auto thisLineEnd = findCursorPosition(currentPosition, TextNavigation::LineEnd).m_char;
            if (thisLineEnd == m_lastPos)
                return currentPosition;

            auto nextLineEnd = findCursorPosition(thisLineEnd + 1, TextNavigation::LineEnd).m_char;
            auto pos = nextLineEnd;
            while (pos > thisLineEnd)
            {
                const auto& ch = charAt(pos);

                if (ch.m_bounds.m_boxMin.x <= posX)
                    break;

                pos -= 1;
            }

            return CursorNavigation(pos, posX - charAt(pos).m_bounds.m_boxMin.x);
        }

        case TextNavigation::WordStart:
        {
            int pos = currentPosition.m_char; // ala ma  kota
            if (pos > 0)
            {
                // skip initial whitespaces
                while (pos > 0)
                {
                    if (m_chars[pos - 1].m_code > 32)
                        break;
                    pos -= 1;
                }

                // search for word start
                while (pos > 0)
                {
                    if (m_chars[pos - 1].m_code <= 32)
                        break;
                    pos -= 1;
                }
            }
            return pos;
        }

        case TextNavigation::WordEnd:
        {
            int pos = currentPosition.m_char;
            if (pos < m_lastPos)
            {
                // search for end of word
                while (pos < m_lastPos)
                {
                    if (m_chars[pos].m_code <= 32)
                        break;
                    pos += 1;
                }

                // skip whitespaces
                while (pos < m_lastPos)
                {
                    if (m_chars[pos].m_code > 32)
                        break;
                    pos += 1;
                }           
            }

            return pos;
        }

        case TextNavigation::PrevChar:
        {
            int pos = currentPosition.m_char;
            while (pos > 0)
            {
                const auto& ch = charAt(--pos);
                if (ch.glyph && ch.glyph->advance().x > 0)
                    break;

            }

            return pos;
        }

        case TextNavigation::NextChar:
        {
            int pos = currentPosition.m_char;
            while (pos < m_lastPos)
            {
                const auto& ch = charAt(++pos);
                if (ch.glyph && ch.glyph->advance().x > 0)
                    break;
            }

            return pos;
        }

        case TextNavigation::NextPage:
        {
            auto numLinesPerPage = 10; // TODO
            auto pos = m_cursorPos;
            for (uint32_t i = 0; i < numLinesPerPage; ++i)
                pos = findCursorPosition(pos, TextNavigation::NextLineChar);
            return pos;
        }

        case TextNavigation::PrevPage:
        {
            auto numLinesPerPage = 10; // TODO
            auto pos = m_cursorPos;
            for (uint32_t i = 0; i < numLinesPerPage; ++i)
                pos = findCursorPosition(pos, TextNavigation::PrevLineChar);
            return pos;
        }
    }

    ASSERT(!"Invalid navigation mode");
    return currentPosition;
}

CursorNavigation TextBuffer::findCursorPosition(const Position& pos) const
{
    // empty
    if (!m_lastPos)
        return CursorNavigation(0);

    // find the row
    // TODO: binary search
    const auto* row = m_rows.typedData();
    const auto* lastRow = row + m_rows.size() - 1;
    while (row < lastRow)
    {
        if (pos.y >= row->m_minY && pos.y <= row->m_maxY)
            break;
        ++row;
    }

    // find the char
    auto index = row->m_start;
    while (index < row->m_end)
    {
        const auto& ch = charAt(index);
        auto halfX = (ch.m_bounds.m_boxMax.x + ch.m_bounds.m_boxMin.x) * 0.5f;
        if (halfX >= pos.x)
            break;
        ++index;
    }

    return CursorNavigation(index);
}

void TextBuffer::resetSelection()
{
    m_selectionStartPos = 0;
    m_selectionEndPos = 0;
    m_cachedSelectionGeometry.reset();
}

bool TextBuffer::moveCursor(CursorNavigation pos, bool extendSelection)
{
    TRACE_INFO("Move cursor native: {}, last pos: {}", pos.m_char, m_lastPos);

    // clamp cursor position to allowed range
    CursorNavigation validPos = pos;
    if (validPos.m_char < 0)
    {
        validPos.m_char = 0;
        validPos.m_pixelOffset = 0;
    }
    else if (validPos.m_char > m_lastPos)
    {
        validPos.m_char = m_lastPos;
        validPos.m_pixelOffset = 0;
    }

    // same position
    if (validPos.m_char == m_cursorPos.m_char)
        return false;

    // set new cursor position
    m_cursorPos = validPos;

    // reset or extend the selection
    m_selectionEndPos = m_cursorPos.m_char;
    if (!extendSelection)
        m_selectionStartPos = m_cursorPos.m_char;

    // invalidate selection rendering
    m_cachedSelectionGeometry.reset();
    return true;
}

void TextBuffer::deleteSelection()
{
    auto newCursorPos = std::min(m_selectionStartPos, m_selectionEndPos);
    deleteCharacters(m_selectionStartPos, m_selectionEndPos);
    moveCursor(newCursorPos, false); // resets selection
}

void TextBuffer::selection(int start, int end)
{
    auto newStart = std::clamp(start, 0, m_lastPos);
    auto newEnd = std::clamp(start, newStart, m_lastPos);
    if (newStart != m_selectionStartPos || newEnd != m_selectionEndPos)
    {
        m_selectionStartPos = newStart;
        m_selectionEndPos = newEnd;
        m_cachedSelectionGeometry.reset();
    }
}

void TextBuffer::deleteCharacters(int start, int end)
{
    auto rangeStart = std::clamp(std::min(start, end), 0, m_lastPos);
    auto rangeEnd = std::clamp(std::max(start, end), 0, m_lastPos);
    if (rangeEnd > rangeStart)
    {
        m_chars.erase(rangeStart, rangeEnd - rangeStart);
        m_modified = true;
        generateLayout();
    }
}

void TextBuffer::insertCharacters(int pos, const StringBuf& txt)
{
    TChars newChars;
    generateChars(txt.c_str(), newChars);

    if (!newChars.empty())
    {
        m_chars.insert(pos, newChars.typedData(), newChars.size());
        m_modified = true;

        generateLayout();

        moveCursor(pos + newChars.size(), false);
    }
}

void TextBuffer::resetHighlight()
{
    m_highlightStartPos = 0;
    m_highlightEndPos = 0;
    m_cachedHighlightGeometry.reset();
}

void TextBuffer::highlight(int start, int end)
{
    if (start != m_highlightStartPos || end != m_highlightEndPos)
    {
        m_highlightStartPos = 0;
        m_highlightEndPos = 0;
        m_cachedHighlightGeometry.reset();
    }
}

void TextBuffer::renderText(const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder)
{
    // generate the text
    builder.fillColor(m_textColor);
    builder.print(m_prefixChars.typedData(), m_prefixChars.size(), sizeof(CharInfo));
    builder.print(m_chars.typedData(), m_chars.size(), sizeof(CharInfo));
    builder.print(m_postfixChars.typedData(), m_postfixChars.size(), sizeof(CharInfo));
}

void TextBuffer::generateRangeBlock(int start, int end, Color color, canvas::GeometryBuilder& builder) const
{
    auto rangeStart = std::clamp(std::min(start, end), 0, m_lastPos);
    auto rangeEnd = std::clamp(std::max(start, end), 0, m_lastPos);
    if (rangeEnd > rangeStart)
    {
        builder.fillColor(color);

        // generate the selection area
        int cur = rangeStart;
        while (cur < rangeEnd)
        {
            auto rowStart = cur;
            auto rowMinY = m_chars[cur].m_bounds.m_boxMin.y;
            auto rowMaxY = m_chars[cur].m_bounds.m_boxMax.y;
            auto rowStartX = m_chars[cur].m_bounds.m_boxMin.x;
            auto rowX = rowStartX;

            // scan till the end row
            while (cur < rangeEnd)
            {
                const auto& glyph = (cur < m_lastPos) ? m_chars[cur] : m_finalGlyph;
                if (glyph.m_bounds.m_boxMin.y != rowMinY || glyph.m_bounds.m_boxMax.y != rowMaxY || glyph.m_bounds.m_boxMin.x != rowX)
                    break;

                rowX = glyph.m_bounds.m_boxMax.x;
                ++cur;
            }

            // add box
            builder.beginPath();
            builder.rect(rowStartX, rowMinY, rowX - rowStartX, rowMaxY - rowMinY);
            builder.fill();
        }
    }
}

void TextBuffer::renderSelection(const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    // generate geometry if needed
	if (m_cachedSelectionGeometry.empty())
	{
		canvas::GeometryBuilder builder(m_cachedSelectionGeometry);
		generateRangeBlock(m_selectionStartPos, m_selectionEndPos, m_selectionColor, builder);
	}

    // draw the selection
	canvas.place(drawArea.absolutePosition(), m_cachedSelectionGeometry, mergedOpacity);
}

void TextBuffer::renderHighlight(const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    // generate geometry if needed
	if (m_cachedHighlightGeometry.empty())
	{
		canvas::GeometryBuilder builder(m_cachedHighlightGeometry);
		generateRangeBlock(m_highlightStartPos, m_highlightEndPos, m_hightlightColor, builder);
	}

    // draw the highlight
	canvas.place(drawArea.absolutePosition(), m_cachedHighlightGeometry, mergedOpacity);
}

void TextBuffer::renderCursor(const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    const auto& glyph = (m_cursorPos.m_char < m_lastPos) ? m_chars[m_cursorPos.m_char] : m_finalGlyph;

    // generate the cursor geometry
    if (m_cachedCursorGeometry.empty())
    {
        canvas::GeometryBuilder builder(m_cachedCursorGeometry);

        builder.strokeColor(m_textColor);
        builder.beginPath();
        builder.moveTo(0.5f, 0.0f);
        builder.lineTo(0.5f, m_finalGlyph.m_bounds.m_boxMax.y - m_finalGlyph.m_bounds.m_boxMin.y);
        builder.stroke();
    }

    // draw the cursor
    auto cursorPos = drawArea.absolutePosition() + glyph.m_bounds.m_boxMin;
    canvas.place(cursorPos, m_cachedCursorGeometry, mergedOpacity);
}

bool TextBuffer::lookupGlyphs()
{
    if (!m_font)
        return false;

    font::FontStyleParams styleParams;
    styleParams.size = m_fontSize;

    for (auto& ch : m_chars)
        if (nullptr == ch.glyph)
            ch.glyph = m_font->renderGlyph(styleParams, ch.m_code);

    for (auto& ch : m_prefixChars)
        if (nullptr == ch.glyph)
            ch.glyph = m_font->renderGlyph(styleParams, ch.m_code);

    for (auto& ch : m_postfixChars)
        if (nullptr == ch.glyph)
            ch.glyph = m_font->renderGlyph(styleParams, ch.m_code);

    return true;
}

void TextBuffer::generateChars(StringView txt, TChars& outChars) const
{
    auto* cur = txt.data();
    auto* end = cur + txt.length();
    while (auto code = utf8::NextChar(cur, end))
    {
        if (code == '\r')
            continue;
        outChars.emplaceBack(code);
    }
}

void TextBuffer::generateLayout()
{
    // update length
    m_lastPos = m_chars.size();
    m_cursorPos.m_char = std::min(m_cursorPos.m_char, m_lastPos);
    m_selectionStartPos = std::min(m_selectionStartPos, m_lastPos);
    m_selectionEndPos = std::min(m_selectionEndPos, m_lastPos);

    // process the characters, retrieve the glyph informations
    if (!lookupGlyphs())
        return;

    // determine the line bounding box
    auto ascenderLine = m_font->relativeAscender() * m_fontSize;
    auto descenderLine = m_font->relativeDescender() * m_fontSize;
    auto lineHeight = m_font->relativeLineHeight() * m_fontSize;

    // reset rows
    m_rows.reset();

    // reset size
    m_totalSize = Size();

    // place the glyphs
    Vector2 placementPos(0.0f, ascenderLine);

    // process buffer as a whole
    TChars* charTables[3] = { &m_prefixChars, &m_chars, &m_postfixChars };
    for (uint32_t i=0; i<3; ++i)
    {
        auto &chars = *charTables[i];

        // process each part
        int cur = 0;
        int length = (int) chars.size();
        int lineStart = 0;
        int wordStart = 0;
        while (cur < length)
        {
            // get char
            auto index = cur;
            auto &ch = chars.typedData()[cur++];

            // lookup code
            auto code = ch.m_code;
            if (!m_multiline && code < 32)
                code = 32;

            // set placement, always
            ch.pos = placementPos;
            if (ch.glyph != nullptr)
            {
                ch.pos.x = (float) (int) (ch.pos.x + ch.glyph->offset().x);
                ch.pos.y = (float) (int) (ch.pos.y + ch.glyph->offset().y);
            }

            // set bounding box
            if (ch.glyph != nullptr)
            {
                auto rect = ch.glyph->logicalRect();
                ch.m_bounds.m_boxMin.x = placementPos.x;
                ch.m_bounds.m_boxMin.y = placementPos.y - ascenderLine;
                ch.m_bounds.m_boxMax.x = placementPos.x + ch.glyph->advance().x;
                ch.m_bounds.m_boxMax.y = placementPos.y - descenderLine;
            } else
            {
                ch.m_bounds.m_boxMin.x = placementPos.x + 0.0f;
                ch.m_bounds.m_boxMin.y = placementPos.y + 0.0f;
                ch.m_bounds.m_boxMax.x = placementPos.x + 5.0f;
                ch.m_bounds.m_boxMax.y = placementPos.y + lineHeight;
            }

            // normal char
            if (ch.m_code >= 32)
            {
                // advance
                if (ch.glyph != nullptr)
                    placementPos.x += ch.glyph->advance().x;
                else
                    placementPos.x += 5.0f;
            }

                // go to next line
            else if (ch.m_code == '\n')
            {
                if (m_multiline && (i == 1))
                {
                    m_rows.emplaceBack(placementPos.y - ascenderLine, placementPos.y - descenderLine, lineStart, index);

                    placementPos.x = 0.0f;
                    placementPos.y += lineHeight;

                    lineStart = index + 1;
                }
            }

                // tab
            else if (ch.m_code == '\t')
            {
                float tabSize = 50.0f; // TODO
                ch.m_bounds.m_boxMin.x = placementPos.x;
                placementPos.x = ceil((placementPos.x + 0.1f) / tabSize) * tabSize;
                ch.m_bounds.m_boxMax.x = placementPos.x;
            }

            // remember total size
            m_totalSize.x = std::max(m_totalSize.x, placementPos.x);
        }

        if (i == 1)
        {
            // finish line
            if (lineStart <= m_lastPos)
                m_rows.emplaceBack(placementPos.y - ascenderLine, placementPos.y - descenderLine, lineStart, m_lastPos);

            // remember the last drawing position
            m_finalGlyph.pos = placementPos;
            m_finalGlyph.m_bounds.m_boxMin = Vector2(placementPos.x, placementPos.y - ascenderLine);
            m_finalGlyph.m_bounds.m_boxMax = Vector2(placementPos.x, placementPos.y - descenderLine);
        }
    }

    // remember total
    m_totalSize.y = placementPos.y - descenderLine;

    // validate rows
    ASSERT(m_rows.size() >= 1);

    // reset cached geometries
    m_cachedCursorGeometry.reset();
    m_cachedSelectionGeometry.reset();
    m_cachedHighlightGeometry.reset();
}

END_BOOMER_NAMESPACE_EX(ui)


