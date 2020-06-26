/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#pragma once

#include "uiElement.h"
#include "uiScrollArea.h"

namespace ui
{
    class TextBuffer;
    struct CursorNavigation;
    class ScrolableContainer;
    class EditBoxSelectionDragInputAction;

    /// a simple editable text box, core functionality - no border, style, etc
    class BASE_UI_API TextEditor : public ScrollArea
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TextEditor, ScrollArea);

    public:
        TextEditor();
        virtual ~TextEditor();

        ///----

        /// called when text changes (after each change)
        ElementEventProxy OnTextModified;

        /// called when user pressed enter key
        ElementEventProxy OnTextAccepted;

        ///----

        // is this multiline editor ?
        INLINE bool multiline() const { return m_multiline; }

        // change multiline mode
        void multiline(bool flag);

        ///----

        // set text, forces regeneration of the glyph buffer and resets selection (UTF8)
        void text(base::StringView<char> txt);

        // get text for the whole buffer (UTF8)
        base::StringBuf text() const;

        ///---

        // select whole text
        void selectWholeText();

        // reset selection
        void clearSelection();

        // get selected text (UTF8)
        base::StringBuf selectedText() const;

        ///----

        // set the uneditable postfix text (usually used for units)
        void postfixText(base::StringView<char> txt);

        // set the uneditable prefix text (usually used for prompt or limited)
        void prefixText(base::StringView<char> txt);

        ///---

    private:
        base::UniquePtr<TextBuffer> m_textBuffer;
        bool m_cursorVisible = true;
        base::NativeTimePoint m_cursorToggleTime;
        base::NativeTimeInterval m_cursorToggleInterval;
        bool m_multiline = false;

        // IElement
        virtual void computeLayout(ElementLayout& outLayout) override;
        virtual void computeSize(Size& outSize) const override;
        virtual void prepareForegroundGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;
        virtual void renderForeground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;

        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
        virtual bool handleCharEvent(const base::input::CharEvent& evt) override;
        virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const override;
        virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
        virtual bool handleContextMenu(const ElementArea& area, const Position& absolutePosition) override;

        virtual bool handleTemplateProperty(base::StringView<char> name, base::StringView<char> value) override;

        void moveCursor(const CursorNavigation& pos, bool extendSelection);

        void cmdCopySelection();
        void cmdCutSelection();
        void cmdPasteSelection();
        void cmdDeleteSelection();
        void cmdSelectAll();
        void bindCommands();

        void textModified();

        bool canModify() const;

        friend class EditBoxSelectionDragInputAction;
    };

    //--

    // a "pretty" edit box for text
    class BASE_UI_API EditBox : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EditBox, IElement);

    public:
        EditBox();

        ///----

        /// called when text changes (after each change)
        ElementEventProxy OnTextModified;

        /// called when user pressed enter key
        ElementEventProxy OnTextAccepted;

        ///----

        /// get inner text editor
        INLINE const base::RefPtr<TextEditor>& editor() const { return m_editor; }

        ///----

        // set text, forces regeneration of the glyph buffer (UTF8)
        void text(base::StringView<char> txt);

        // get text for the whole buffer (UTF8)
        base::StringBuf text() const;

        // select whole text
        void selectWholeText();

        // reset selection
        void clearSelection();

        // get selected text (UTF8)
        base::StringBuf selectedText() const;

        ///---

    private:
        base::RefPtr<TextEditor> m_editor;

        virtual bool handleTemplateProperty(base::StringView<char> name, base::StringView<char> value) override;
        virtual IElement* handleFocusForwarding() override;
    };

    //--

} // ui