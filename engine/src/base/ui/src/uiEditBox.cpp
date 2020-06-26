/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#include "build.h"
#include "uiEditBox.h"
#include "uiTextBuffer.h"

#include "uiInputAction.h"
#include "uiWindow.h"
#include "uiMenuBar.h"

#include "base/input/include/inputStructures.h"
#include "base/image/include/image.h"
#include "base/containers/include/clipboard.h"

namespace ui
{

    RTTI_BEGIN_TYPE_CLASS(TextEditor);
        RTTI_METADATA(ElementClassNameMetadata).name("TextEditor");
    RTTI_END_TYPE();
    
    TextEditor::TextEditor()
        : ScrollArea(ScrollMode::None, ScrollMode::Hidden)
        , m_cursorToggleInterval(0.5)
        , OnTextModified(this, "OnTextModified"_id)
        , OnTextAccepted(this, "OnTextAccepted"_id)
    {
        layoutMode(LayoutMode::Vertical);
        allowFocusFromKeyboard(true);

        hitTest(HitTestState::Enabled);

        m_textBuffer = base::CreateUniquePtr<TextBuffer>();

        m_cursorToggleTime.resetToNow();
        m_cursorToggleTime += m_cursorToggleInterval;

        bindCommands();
    }

    TextEditor::~TextEditor()
    {
        m_textBuffer.reset();
    }

    void TextEditor::multiline(bool flag)
    {
        if (flag != m_multiline)
        {
            m_multiline = flag;
            m_textBuffer->multiLine(m_multiline);
            verticalScrollMode(m_multiline ? ScrollMode::Auto : ScrollMode::None);
        }
    }

    bool TextEditor::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        if (name == "text")
        {
            text(value);
            return true;
        }
        else if (name == "prefix")
        {
            prefixText(value);
            return true;
        }
        else if (name == "postfix")
        {
            postfixText(value);
            return true;
        }
        else if (name == "multiline" || name == "multiLine")
        {
            bool flag = false;
            if (base::MatchResult::OK != value.match(flag))
                return false;

            multiline(flag);
            return true;
        }

        return TBaseClass::handleTemplateProperty(name, value);
    }

    void TextEditor::text(base::StringView<char> txt)
    {
        m_textBuffer->text(txt);
        invalidateLayout();
        invalidateGeometry();
    }

    void TextEditor::postfixText(base::StringView<char> txt)
    {
        m_textBuffer->postfixText(txt);
        invalidateLayout();
        invalidateGeometry();
    }

    void TextEditor::prefixText(base::StringView<char> txt)
    {
        m_textBuffer->prefixText(txt);
        invalidateLayout();
        invalidateGeometry();
    }

    base::StringBuf TextEditor::text() const
    {
        return m_textBuffer->text();
    }

    void TextEditor::selectWholeText()
    {
        moveCursor(0, false);
        moveCursor(m_textBuffer->length(), true);
    }

    void TextEditor::clearSelection()
    {
        moveCursor(m_textBuffer->length(), false);
    }

    base::StringBuf TextEditor::selectedText() const
    {
        if (m_textBuffer->hasSelection())
            return m_textBuffer->text(m_textBuffer->selectionStartPos(), m_textBuffer->selectionEndPos());
        else
            return base::StringBuf::EMPTY();
    }

    void TextEditor::computeLayout(ElementLayout& outLayout)
    {
        m_textBuffer->style(this);
        TBaseClass::computeLayout(outLayout);
    }

    void TextEditor::computeSize(Size& outSize) const
    {
        TBaseClass::computeSize(outSize);
        outSize = m_textBuffer->size();
    }

    void TextEditor::prepareForegroundGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const
    {
        TBaseClass::prepareForegroundGeometry(drawArea, pixelScale, builder);
        m_textBuffer->renderText(drawArea, pixelScale, builder);
    }

    void TextEditor::renderForeground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        m_textBuffer->renderHighlight(drawArea, canvas, mergedOpacity);
        m_textBuffer->renderSelection(drawArea, canvas, mergedOpacity);

        TBaseClass::renderForeground(drawArea, canvas, mergedOpacity); // text

        if (isFocused())
        {
            if (m_cursorVisible)
                m_textBuffer->renderCursor(drawArea, canvas, mergedOpacity);

            if (m_cursorToggleTime.reached() || !m_cursorToggleTime.valid())
            {
                m_cursorVisible = !m_cursorVisible;
                m_cursorToggleTime.resetToNow();
                m_cursorToggleTime += m_cursorToggleInterval;
            }
        }
    }

    void TextEditor::moveCursor(const CursorNavigation& pos, bool extendSelection)
    {
        m_textBuffer->moveCursor(pos, extendSelection);

        {
            auto cursorCharBounds = m_textBuffer->characterBounds(m_textBuffer->cursorPos().m_char);
            auto cursorPos = base::Vector2(cursorCharBounds.m_boxMin.x, (cursorCharBounds.m_boxMin.y + cursorCharBounds.m_boxMax.y) * 0.5f);

            ElementArea charArea(cachedDrawArea().absolutePosition() + cursorPos, ui::Size(2, 2));
            //ensureVisible(charArea);
        }

        m_cursorVisible = true;
        m_cursorToggleTime.resetToNow();
        m_cursorToggleTime += m_cursorToggleInterval;
    }

    bool TextEditor::canModify() const
    {
        return isEnabled();
    }

    void TextEditor::textModified()
    {
        invalidateLayout();
        invalidateGeometry();

        OnTextModified();
    }

    bool TextEditor::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (evt.isDown())
        {
            auto pos = m_textBuffer->cursorPos().m_char;

            if (evt.keyCode() == base::input::KeyCode::KEY_DELETE)
            {
                if (canModify())
                {
                    if (m_textBuffer->hasSelection())
                    {
                        m_textBuffer->deleteSelection();
                    }
                    else
                    {
                        m_textBuffer->deleteCharacters(pos, pos + 1);
                    }

                    textModified();
                }
                return true;
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_BACK)
            {
                if (canModify())
                {
                    if (m_textBuffer->hasSelection())
                    {
                        m_textBuffer->deleteSelection();
                    }
                    else if (pos > 0)
                    {
                        m_textBuffer->deleteCharacters(pos - 1, pos);
                        m_textBuffer->moveCursor(pos - 1, false); // resets selection
                    }

                    textModified();
                }
                return true;
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_LEFT)
            {
                auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::WordStart : TextNavigation::PrevChar;
                auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                bool extendSelection = evt.keyMask().isShiftDown();
                moveCursor(newCursorPos, extendSelection);
                return true;
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_RIGHT)
            {
                auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::WordEnd : TextNavigation::NextChar;
                auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                bool extendSelection = evt.keyMask().isShiftDown();
                moveCursor(newCursorPos, extendSelection);
                return true;
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_HOME)
            {
                auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::DocumentStart : TextNavigation::LineStart;
                auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                bool extendSelection = evt.keyMask().isShiftDown();
                moveCursor(newCursorPos, extendSelection);
                return true;
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_END)
            {
                auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::DocumentEnd : TextNavigation::LineEnd;
                auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                bool extendSelection = evt.keyMask().isShiftDown();
                moveCursor(newCursorPos, extendSelection);
                return true;
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_UP)
            {
                if (m_textBuffer->isMultiLine())
                {
                    auto navigation = TextNavigation::PrevLineChar;
                    auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                    bool extendSelection = evt.keyMask().isShiftDown();
                    moveCursor(newCursorPos, extendSelection);
                    return true;
                }
                else
                {
                    return false; // explicitly DO NOT HANDLE
                }
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_DOWN)
            {
                if (m_textBuffer->isMultiLine())
                {
                    auto navigation = TextNavigation::NextLineChar;
                    auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                    bool extendSelection = evt.keyMask().isShiftDown();
                    moveCursor(newCursorPos, extendSelection);
                    return true;
                }
                else
                {
                    return false; // explicitly DO NOT HANDLE
                }
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_PRIOR)
            {
                if (m_textBuffer->isMultiLine())
                {
                    auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::PrevPage : TextNavigation::PrevPage;
                    auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                    bool extendSelection = evt.keyMask().isShiftDown();
                    moveCursor(newCursorPos, extendSelection);
                    return true;
                }
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_NEXT)
            {
                if (m_textBuffer->isMultiLine())
                {
                    auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::NextPage : TextNavigation::NextPage;
                    auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                    bool extendSelection = evt.keyMask().isShiftDown();
                    moveCursor(newCursorPos, extendSelection);
                    return true;
                }
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_ESCAPE)
            {
                return false; // pass through
            }
        }

        // process other events normally, eat all input
        TBaseClass::handleKeyEvent(evt);
        return true;
    }

    void TextEditor::cmdCopySelection()
    {
        if (m_textBuffer->hasSelection())
        {
            auto* windowRenderer = this->renderer();
            if (windowRenderer)
            {
                auto data = selectedText().view().toBuffer();
                auto format = "Text"_id;
                //windowRenderer->storeClipboardData(&format, &data, 1);
            }
        }
    }

    void TextEditor::cmdCutSelection()
    {
        if (canModify())
        {
            if (m_textBuffer->hasSelection())
            {
                auto *windowRenderer = this->renderer();
                if (windowRenderer)
                {
                    auto data = selectedText().view().toBuffer();
                    auto format = "Text"_id;
                    //windowRenderer->storeClipboardData(&format, &data, 1);
                }

                m_textBuffer->deleteSelection();
                textModified();

                moveCursor(m_textBuffer->cursorPos(), false);
            }
        }
    }

    void TextEditor::cmdPasteSelection()
    {
        if (canModify())
        {
            auto *windowRenderer = this->renderer();
            if (windowRenderer)
            {
                base::Buffer data;
                /*if (windowRenderer->loadClipboardData("Text"_id, data))
                {
                    auto text = base::StringBuf(data);

                    // replace existing text
                    if (m_textBuffer->hasSelection())
                        m_textBuffer->deleteSelection();

                    // paste new content
                    auto insertPos = m_textBuffer->cursorPos().m_char;
                    m_textBuffer->insertCharacters(insertPos, text);
                    textModified();

                    moveCursor(m_textBuffer->cursorPos(), false);
                }*/
            }
        }
    }

    void TextEditor::cmdDeleteSelection()
    {
        if (canModify())
        {
            if (m_textBuffer->hasSelection())
            {
                m_textBuffer->deleteSelection();
                textModified();
            }
        }
    }

    void TextEditor::cmdSelectAll()
    {
        selectWholeText();
    }

    void TextEditor::bindCommands()
    {
        actions().bindCommand("TextEdit.Copy"_id) = [](TextEditor* box) { box->cmdCopySelection(); };
        actions().bindCommand("TextEdit.Cut"_id) = [](TextEditor* box) { box->cmdCutSelection(); };
        actions().bindCommand("TextEdit.Delete"_id) = [](TextEditor* box) { box->cmdDeleteSelection(); };
        actions().bindCommand("TextEdit.SelectAll"_id) = [](TextEditor* box) { box->cmdSelectAll(); };
        actions().bindCommand("TextEdit.Undo"_id) = [](TextEditor* box) { };
        actions().bindCommand("TextEdit.Redo"_id) = [](TextEditor* box) { };

        auto hasSelectionFunc = [](TextEditor* box) { return box->m_textBuffer->hasSelection(); };
        auto hasClipbordData = [](TextEditor* box) { return true; };

        actions().bindFilter("TextEdit.Copy"_id) = hasSelectionFunc;
        actions().bindFilter("TextEdit.Cut"_id) = hasSelectionFunc;
        actions().bindFilter("TextEdit.Delete"_id) = hasSelectionFunc;
        actions().bindFilter("TextEdit.Paste"_id) = hasClipbordData;

        actions().bindShortcut("TextEdit.Copy"_id, "Ctrl+C");
        actions().bindShortcut("TextEdit.Cut"_id, "Ctrl+X");
        actions().bindShortcut("TextEdit.Paste"_id, "Ctrl+V"); 
        actions().bindShortcut("TextEdit.SelectAll"_id, "Ctrl+A");
        actions().bindShortcut("TextEdit.Undo"_id, "Ctrl+Z");
        actions().bindShortcut("TextEdit.Redo"_id, "Ctrl+Y");
    }

    bool TextEditor::handleContextMenu(const ElementArea& area, const Position& absolutePosition)
    {
        auto ret = base::CreateSharedPtr<MenuButtonContainer>();
        ret->createAction("TextEdit.Undo"_id, "Undo", "[img:undo]");
        ret->createAction("TextEdit.Redo"_id, "Redo", "[img:redo]");
        ret->createSeparator();
        ret->createAction("TextEdit.Copy"_id, "Copy", "[img:copy]");
        ret->createAction("TextEdit.Cut"_id, "Cut", "[img:cut]");
        ret->createAction("TextEdit.Paste"_id, "Cut", "[img:paste]");
        ret->createAction("TextEdit.Delete"_id, "Delete", "[img:cross]");
        ret->createSeparator();
        ret->createAction("TextEdit.SelectAll"_id, "Select all", "[img:selection_area]");
        ret->show(this);
        return true;
    }

    bool TextEditor::handleCharEvent(const base::input::CharEvent& evt)
    {
        // ignore backspace and delete
        if ((evt.scanCode() >= 32 && evt.scanCode() < 127) || evt.scanCode() == 13 || evt.scanCode() == 9)
        {
            if (canModify())
            {
                // accept
                if (!m_textBuffer->isMultiLine() && evt.scanCode() == 13)
                {
                    OnTextAccepted(m_textBuffer->text());
                    return true;
                }

                // replace existing text
                if (m_textBuffer->hasSelection())
                    m_textBuffer->deleteSelection();

                // text to insert
                // TODO: IME
                base::StringBuf textToInsert;
                if (evt.scanCode() == 13)
                {
                    textToInsert = "\n";
                }
                else if (evt.scanCode() == 9 && m_textBuffer->isMultiLine())
                {
                    textToInsert = "\t";
                }
                else
                {
                    wchar_t uniChars[] = {evt.scanCode(), 0};
                    textToInsert = base::StringBuf(base::StringView<wchar_t>(uniChars));
                }

                // insert new characters
                auto insertPos = m_textBuffer->cursorPos().m_char;
                m_textBuffer->insertCharacters(insertPos, textToInsert);
                //moveCursor(m_textBuffer->cursorPos(), false);

                textModified();
            }
        }

        return true;
    }

    class EditBoxSelectionDragInputAction : public IInputAction
    {
    public:
        EditBoxSelectionDragInputAction(TextEditor* editBox, int startSelection)
            : IInputAction(editBox)
            , m_editBox(editBox)
            , m_startSelection(startSelection)
        {}

        virtual void onCanceled() override
        {
            if (auto editBox = m_editBox.lock())
                editBox->m_textBuffer->moveCursor(m_startSelection, false);
        }

        virtual InputActionResult onKeyEvent(const base::input::KeyEvent& evt) override
        {
            // any key action resets the event
            if (evt.pressed())
            {
                if (evt.keyCode() == base::input::KeyCode::KEY_LEFT_CTRL || evt.keyCode() == base::input::KeyCode::KEY_RIGHT_CTRL)
                    return InputActionResult();

/*              if (evt.keyCode() == base::input::KeyCode::KEY_C && evt.isCtrlDown())
                {
                    m_editBox->handleKeyEvent(evt);
                    return InputActionResult();
                }
                else if (evt.keyCode() == base::input::KeyCode::KEY_X && evt.isCtrlDown())
                {
                    return InputActionResult();
                }
                else if (evt.keyCode() == base::input::KeyCode::KEY_V && evt.isCtrlDown())
                {
                    return InputActionResult();
                }*/

                if (evt.keyCode() != base::input::KeyCode::KEY_MOUSE0 && evt.keyCode() != base::input::KeyCode::KEY_LEFT_SHIFT)
                {
                    if (auto editBox = m_editBox.lock())
                        editBox->moveCursor(m_startSelection, false);
                    return InputActionResult(nullptr);
                }
            }

            return InputActionResult();
        }

        virtual InputActionResult onMouseEvent(const base::input::MouseClickEvent& evt, const ElementWeakPtr& hoverStack) override
        {
            if (!evt.leftReleased())
            {
                if (auto editBox = m_editBox.lock())
                    editBox->moveCursor(m_startSelection, false);
            }

            return InputActionResult(nullptr);
        }

        virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
        {            
            if (auto editBox = m_editBox.lock())
            {
                auto relativePos = evt.absolutePosition().toVector() - editBox->cachedDrawArea().absolutePosition();

                bool extendSelection = evt.keyMask().isShiftDown();
                auto clickPos = editBox->m_textBuffer->findCursorPosition(relativePos);
                editBox->moveCursor(clickPos, true);
            }

            return InputActionResult();
        }

    private:
        base::RefWeakPtr<TextEditor> m_editBox;
        int m_startSelection;
    };

    InputActionPtr TextEditor::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.leftDoubleClicked())
        {
            selectWholeText();
            return InputActionPtr();
        }
        else if (evt.leftClicked())
        {
            auto relativePos = evt.absolutePosition().toVector() - area.absolutePosition();

            bool extendSelection = evt.keyMask().isShiftDown();
            auto clickPos = m_textBuffer->findCursorPosition(relativePos);
            m_textBuffer->moveCursor(clickPos, extendSelection);

            return base::CreateSharedPtr<EditBoxSelectionDragInputAction>(this, m_textBuffer->selectionStartPos());
        }

        return InputActionPtr();
    }

    bool TextEditor::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const
    {
        outCursorType = base::input::CursorType::TextBeam;
        return true;
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(EditBox);
        RTTI_METADATA(ElementClassNameMetadata).name("EditBox");
    RTTI_END_TYPE();

    EditBox::EditBox()
        : OnTextModified(this, "OnTextModified"_id)
        , OnTextAccepted(this, "OnTextAccepted"_id)
    {
        hitTest(true);
        allowFocusFromClick(true);
        layoutHorizontal();

        if (!base::IsDefaultObjectCreation())
        {
            m_editor = createInternalChild<TextEditor>();
            m_editor->bind("OnTextModified"_id, this) = [](EditBox* box) { box->OnTextModified(); };
            m_editor->bind("OnTextAccepted"_id, this) = [](EditBox* box) { box->OnTextAccepted(); };
            m_editor->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_editor->customVerticalAligment(ui::ElementVerticalLayout::Expand);
        }
    }

    void EditBox::text(base::StringView<char> txt)
    {
        m_editor->text(txt);
    }

    base::StringBuf EditBox::text() const
    {
        return m_editor->text();
    }

    void EditBox::selectWholeText()
    {
        m_editor->selectWholeText();
    }

    void EditBox::clearSelection()
    {
        m_editor->clearSelection();
    }

    base::StringBuf EditBox::selectedText() const
    {
        return m_editor->selectedText();
    }

    IElement* EditBox::handleFocusForwarding()
    {
        return m_editor;
    }

    bool EditBox::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        if (name == "text")
        {
            m_editor->text(value);
            return true;
        }
        else if (name == "prefix")
        {
            m_editor->prefixText(value);
            return true;
        }
        else if (name == "postfix")
        {
            m_editor->postfixText(value);
            return true;
        }
        else if (name == "multiline" || name == "multiLine")
        {
            bool flag = false;
            if (base::MatchResult::OK != value.match(flag))
                return false;

            m_editor->multiline(flag);
            return true;
        }

        return TBaseClass::handleTemplateProperty(name, value);
    }

    //--

} // ui

