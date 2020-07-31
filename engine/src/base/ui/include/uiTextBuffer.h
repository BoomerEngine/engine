/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#pragma once

#include "uiElement.h"

#include "base/font/include/fontGlyphBuffer.h"
#include "base/canvas/include/canvasGeometry.h"

namespace ui
{

    /// text navigation
    enum class TextNavigation
    {
        DocumentStart,
        LineStart,
        WordStart,
        PrevChar,
        PrevLineChar,
        NextChar,
        NextLineChar,
        WordEnd,
        LineEnd,
        DocumentEnd,
        PrevPage,
        NextPage,
    };

    /// cursor navigation position
    struct CursorNavigation
    {
        int m_char;
        float m_pixelOffset;

        INLINE CursorNavigation()
            : m_char(0)
            , m_pixelOffset(0.0f)
        {}

        INLINE CursorNavigation(int pos)
            : m_char(pos)
            , m_pixelOffset(0.0f)
        {}

        INLINE CursorNavigation(int pos, float offset)
            : m_char(pos)
            , m_pixelOffset(offset)
        {}

        INLINE CursorNavigation(const CursorNavigation& other) = default;
        INLINE CursorNavigation(CursorNavigation&& other) = default;
        INLINE CursorNavigation& operator=(const CursorNavigation& other) = default;
        INLINE CursorNavigation& operator=(CursorNavigation&& other) = default;
    };

    /// bounds of text character
    struct CharacterBounds
    {
        base::Vector2 m_boxMin;
        base::Vector2 m_boxMax;

        INLINE CharacterBounds()
            : m_boxMin(0,0)
            , m_boxMax(0,0)
        {}

        INLINE CharacterBounds(const base::Vector2& bmin, const base::Vector2& bmax)
            : m_boxMin(bmin)
            , m_boxMax(bmax)
        {}

        INLINE CharacterBounds(float minx, float miny, float maxx, float maxy)
            : m_boxMin(minx, miny)
            , m_boxMax(maxy, maxy)
        {}

        INLINE CharacterBounds(const CharacterBounds& other) = default;
        INLINE CharacterBounds(CharacterBounds&& other) = default;
        INLINE CharacterBounds& operator=(const CharacterBounds& other) = default;
        INLINE CharacterBounds& operator=(CharacterBounds&& other) = default;
    };

    /// helper class to deal with text editing
    class TextBuffer : public base::NoCopy
    {
    public:
        TextBuffer(bool multiline);
        ~TextBuffer();

        /// get length of the text
        INLINE int length() const { return m_lastPos; }

        /// get cursor position
        INLINE CursorNavigation cursorPos() const { return m_cursorPos; }

        /// do we have selection ?
        INLINE bool hasSelection() const { return m_selectionEndPos != m_selectionStartPos; }

        /// get first selected position
        INLINE int selectionStartPos() const { return m_selectionStartPos; }

        /// get last selected position
        INLINE int selectionEndPos() const { return m_selectionEndPos; }

        /// get first highlighted position
        INLINE int highlightStartPos() const { return m_selectionStartPos; }

        /// get last highlighted position
        INLINE int highlightEndPos() const { return m_selectionStartPos; }

        /// is this a multi line text ?
        INLINE bool isMultiLine() const { return m_multiline; }

        /// get the wrapping with for the text presentation
        INLINE uint32_t wrapWidth() const { return m_wrapWidth; }

        /// get size of the all text in the buffer if no size constraint was applied
        INLINE const Size& size() const { return m_totalSize; }

        /// are we modified from initial state ?
        INLINE bool modified() const { return m_modified; }

        ///---

        /// reset to contain text content
        void text(base::StringView<char> txt);

        /// set the additional text to print AT THE END (usually units)
        /// NOTE: this text is not editable but will be displayed normally
        void postfixText(base::StringView<char> txt);

        /// set the additional text to print BEFORE (usually some form of console prompt)
        /// NOTE: this text is not editable but will be displayed normally
        void prefixText(base::StringView<char> txt);

        /// setup styles
        void style(const IElement* styleOwner);

        /// set the wrapping width, 0 to disable
        void wrapWidth(uint32_t wrapWidth);

        /// update layout information
        //void updateLayout();

        //---

        /// find cursor position for given navigation mode
        CursorNavigation findCursorPosition(CursorNavigation currentPosition, TextNavigation mode) const;

        /// find cursor position for given point
        CursorNavigation findCursorPosition(const Position& pos) const;

        /// get bounds of given character
        CharacterBounds characterBounds(int index) const;

        /// reset selection
        void resetSelection();

        /// delete current selection
        void deleteSelection();

        /// set selection
        void selection(int start, int end);

        /// delete characters at given range
        void deleteCharacters(int start, int end);

        /// insert new characters at given position
        void insertCharacters(int pos, const base::StringBuf& txt);

        /// move cursor to given position
        void moveCursor(CursorNavigation pos, bool extendSelection);

        ///--

        /// reset the highlight region
        void resetHighlight();

        /// set the highlight range for the text
        void highlight(int start, int end);

        ///--

        /// render text into canvas
        void renderText(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder);

        /// render cursor
        void renderCursor(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity);

        /// render selection area
        void renderSelection(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity);

        /// render highlight area
        void renderHighlight(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity);

        ///--

        /// retrieve printable text (UTF8)
        base::StringBuf text() const;

        /// retrieve printable text for part of the buffer (UTF8)
        base::StringBuf text(int startPos, int endPos) const;

        ///--
        
        /// invalidate cached geometry
        void invalidateGeometry();

    private:
        struct CharInfo : public base::font::GlyphBufferEntry
        {
            // char code of the text
            uint32_t m_code;

            // logical bounding box of the glyph
            CharacterBounds m_bounds;

            INLINE CharInfo(uint32_t code)
                : m_code(code)
            {}
        };

        struct Row
        {
            float m_minY;
            float m_maxY;
            int m_start;
            int m_end;

            INLINE Row(float minY, float maxY, int start, int end)
                : m_minY(minY)
                , m_maxY(maxY)
                , m_start(start)
                , m_end(end)
            {}
        };

        typedef base::Array<CharInfo> TChars;
        TChars m_chars;

        TChars m_prefixChars;
        TChars m_postfixChars;

        typedef base::Array<Row> TRows;
        TRows m_rows;

        base::FontPtr m_font;
        uint32_t m_fontSize = 14;
        base::Color m_textColor = base::Color::WHITE;
        base::Color m_selectionColor = base::Color::BLUE;
        base::Color m_hightlightColor = base::Color::YELLOW;

        int m_lastPos;
        int m_selectionStartPos;
        int m_selectionEndPos;
        int m_highlightStartPos;
        int m_highlightEndPos;

        CursorNavigation m_cursorPos;

        bool m_multiline;
        uint32_t m_wrapWidth;

        CharInfo m_finalGlyph;

        bool m_modified;

        base::canvas::GeometryPtr m_cachedCursorGeometry;
        base::canvas::GeometryPtr m_cachedSelectionGeometry;
        base::canvas::GeometryPtr m_cachedHighlightGeometry;

        Size m_totalSize;

        void generateChars(base::StringView<char> txt, TChars& outChars) const;
        void generateLayout();

        void generateRangeBlock(const ElementArea& drawArea, int start, int end, base::Color color, base::canvas::Geometry& outGeometry) const;

        bool lookupGlyphs();

        INLINE const CharInfo& charAt(int index) const
        {
            ASSERT(index >= 0 && index <= m_lastPos);
            return (index == m_lastPos) ? m_finalGlyph : m_chars[index];
        }
    };

} // ui