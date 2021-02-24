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
#include "uiRenderer.h"

#include "base/input/include/inputStructures.h"
#include "base/image/include/image.h"
#include "base/canvas/include/canvasGeometryBuilder.h"

BEGIN_BOOMER_NAMESPACE(ui)

RTTI_BEGIN_TYPE_CLASS(EditBox);
    RTTI_METADATA(ElementClassNameMetadata).name("EditBox");
RTTI_END_TYPE();
    
EditBox::EditBox(EditBoxFeatureFlags features, base::StringView initialText)
    : ScrollArea(ScrollMode::None, ScrollMode::Hidden)
    , m_features(features)
    , m_cursorToggleInterval(0.5)
{
    layoutMode(LayoutMode::Vertical);
    allowFocusFromKeyboard(true);
    allowFocusFromClick(true);

    hitTest(HitTestState::Enabled);

    const bool multiline = m_features.test(ui::EditBoxFeatureBit::Multiline);
    m_textBuffer = base::CreateUniquePtr<TextBuffer>(multiline);

    verticalScrollMode(multiline ? ScrollMode::Auto : ScrollMode::None);

    m_cursorToggleTime.resetToNow();
    m_cursorToggleTime += m_cursorToggleInterval;

    if (features.test(ui::EditBoxFeatureBit::NoBorder))
        addStyleClass("noborder"_id);

    text(initialText);

    bindCommands();
}

EditBox::~EditBox()
{
    m_textBuffer.reset();
}

void EditBox::text(base::StringView txt)
{
    m_textBuffer->text(txt);

    if (isFocused())
    {
        m_focusedClickConter = 0;
        selectWholeText();
    }

    recheckValidation();
    invalidateLayout();
    invalidateGeometry();
}

void EditBox::recheckValidation()
{
    bool valid = m_validation ? m_validation(text()) : true;
    if (valid != m_validationResult)
    {
        m_validationResult = valid;
        call(EVENT_TEXT_VALIDATION_CHANGED, valid);

        if (valid)
            removeStylePseudoClass("invalid"_id);
        else
            addStylePseudoClass("invalid"_id);
    }
}

void EditBox::validation(const TInputValidationFunction& func)
{
    m_validation = func;
    recheckValidation();
}

void EditBox::postfixText(base::StringView txt)
{
    m_textBuffer->postfixText(txt);
    invalidateLayout();
    invalidateGeometry();
}

void EditBox::prefixText(base::StringView txt)
{
    m_textBuffer->prefixText(txt);
    invalidateLayout();
    invalidateGeometry();
}

void EditBox::hintText(base::StringView txt)
{
    // TODO
}

base::StringBuf EditBox::text() const
{
    return m_textBuffer->text();
}

void EditBox::selectWholeText()
{
    moveCursor(0, false);
    moveCursor(m_textBuffer->length(), true);
}

void EditBox::clearSelection()
{
    moveCursor(m_textBuffer->length(), false);
}

base::StringBuf EditBox::selectedText() const
{
    if (m_textBuffer->hasSelection())
        return m_textBuffer->text(m_textBuffer->selectionStartPos(), m_textBuffer->selectionEndPos());
    else
        return base::StringBuf::EMPTY();
}

void EditBox::computeLayout(ElementLayout& outLayout)
{
    m_textBuffer->style(this);
    TBaseClass::computeLayout(outLayout);
}

void EditBox::computeSize(Size& outSize) const
{
    TBaseClass::computeSize(outSize);
    outSize = m_textBuffer->size();
}

void EditBox::prepareForegroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const
{
    TBaseClass::prepareForegroundGeometry(stash, drawArea, pixelScale, builder);

    // apply the padding to the text being built...
    auto innerArea = cachedLayoutParams().calcInnerAreaFromDrawArea(drawArea);
    auto offset = innerArea.absolutePosition() - drawArea.absolutePosition();

    builder.pushTransform();
    builder.translate(offset.x, offset.y);
    m_textBuffer->renderText(innerArea, pixelScale, builder);
    builder.popTransform();
}

void EditBox::renderForeground(DataStash& stash, const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
{
    auto innerArea = cachedLayoutParams().calcInnerAreaFromDrawArea(drawArea);
    m_textBuffer->renderHighlight(innerArea, canvas, mergedOpacity);

    if (isFocused())
        m_textBuffer->renderSelection(innerArea, canvas, mergedOpacity);

    TBaseClass::renderForeground(stash, drawArea, canvas, mergedOpacity); // text

    if (isFocused())
    {
        if (m_cursorVisible && isEnabled())
            m_textBuffer->renderCursor(innerArea, canvas, mergedOpacity);

        if (m_cursorToggleTime.reached() || !m_cursorToggleTime.valid())
        {
            m_cursorVisible = !m_cursorVisible;
            m_cursorToggleTime.resetToNow();
            m_cursorToggleTime += m_cursorToggleInterval;
        }
    }
}

void EditBox::moveCursor(const CursorNavigation& pos, bool extendSelection)
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

bool EditBox::canModify() const
{
    return isEnabled();
}

void EditBox::textModified()
{
    invalidateLayout();
    invalidateGeometry();

    call(EVENT_TEXT_MODIFIED, text());

    recheckValidation();
}

bool EditBox::handleKeyEvent(const base::input::KeyEvent& evt)
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

void EditBox::cmdCopySelection()
{
    if (m_textBuffer->hasSelection())
    {
        renderer()->storeTextToClipboard(selectedText());
    }
}

void EditBox::cmdCutSelection()
{
    if (canModify())
    {
        if (m_textBuffer->hasSelection())
        {
            renderer()->storeTextToClipboard(selectedText());

            m_textBuffer->deleteSelection();
            textModified();

            moveCursor(m_textBuffer->cursorPos(), false);
        }
    }
}

void EditBox::cmdPasteSelection()
{
    if (canModify())
    {
        base::StringBuf text;
        if (renderer()->loadStringFromClipboard(text))
        {
            // replace existing text
            if (m_textBuffer->hasSelection())
                m_textBuffer->deleteSelection();

            // paste new content
            auto insertPos = m_textBuffer->cursorPos().m_char;
            m_textBuffer->insertCharacters(insertPos, text);
            textModified();

            moveCursor(m_textBuffer->cursorPos(), false);
        }
    }
}

void EditBox::cmdDeleteSelection()
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

void EditBox::cmdSelectAll()
{
    selectWholeText();
}

void EditBox::bindCommands()
{
    actions().bindCommand("TextEdit.Copy"_id) = [](EditBox* box) { box->cmdCopySelection(); };
    actions().bindCommand("TextEdit.Cut"_id) = [](EditBox* box) { box->cmdCutSelection(); };
    actions().bindCommand("TextEdit.Delete"_id) = [](EditBox* box) { box->cmdDeleteSelection(); };
    actions().bindCommand("TextEdit.SelectAll"_id) = [](EditBox* box) { box->cmdSelectAll(); };
    actions().bindCommand("TextEdit.Undo"_id) = [](EditBox* box) { };
    actions().bindCommand("TextEdit.Redo"_id) = [](EditBox* box) { };

    auto hasSelectionFunc = [](EditBox* box) { return box->m_textBuffer->hasSelection(); };
    auto hasClipbordData = [](EditBox* box) { return true; };

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

bool EditBox::handleContextMenu(const ElementArea& area, const Position& absolutePosition, base::input::KeyMask controlKeys)
{
    auto ret = base::RefNew<MenuButtonContainer>();
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

bool EditBox::handleCharEvent(const base::input::CharEvent& evt)
{
    // ignore tabs
    if (evt.scanCode() == 9 && !m_features.test(EditBoxFeatureBit::AcceptsTab))
        return TBaseClass::handleCharEvent(evt);

    // ignore enter
    if (evt.scanCode() == 13 && !m_features.test(EditBoxFeatureBit::AcceptsEnter))
        return TBaseClass::handleCharEvent(evt);

    // ignore backspace and delete
    if ((evt.scanCode() >= 32 && evt.scanCode() < 127) || evt.scanCode() == 13 || evt.scanCode() == 9)
    {
        if (canModify())
        {
            // enter ?
            if (evt.scanCode() == 13 && !m_textBuffer->isMultiLine())
            {
                if (m_validationResult)
                    call(EVENT_TEXT_ACCEPTED, text());
                else
                    renderer()->playSound(MessageType::Warning);
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
            else if (evt.scanCode() == 9)
            {
                textToInsert = "\t";
            }
            else
            {
                wchar_t uniChars[] = {evt.scanCode(), 0};
                textToInsert = base::StringBuf(base::BaseStringView<wchar_t>(uniChars));
            }

            // check if text will validate after insertion

            auto insertPos = m_textBuffer->cursorPos().m_char;


            // insert new characters
            m_textBuffer->insertCharacters(insertPos, textToInsert);
                

            EditBox::


            textModified();
        }
    }

    return true;
}

class EditBoxSelectionDragInputAction : public IInputAction
{
public:
    EditBoxSelectionDragInputAction(EditBox* editBox, int startSelection)
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
    base::RefWeakPtr<EditBox> m_editBox;
    int m_startSelection;
};

void EditBox::handleFocusGained()
{
    TBaseClass::handleFocusGained();

    // reset click counter
    m_focusedClickConter = 0;

    // if we haven't changed anything in the text buffer so far select whole text
    if (!m_textBuffer->modified())
        selectWholeText();
}

InputActionPtr EditBox::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
{
    if (evt.leftDoubleClicked())
    {
        selectWholeText();
        return InputActionPtr();
    }
    else if (evt.leftClicked())
    {
        if (isFocused())
        {
            auto clickIndex = m_focusedClickConter++;
            if (clickIndex > 0)
            {
                auto relativePos = evt.absolutePosition().toVector() - area.absolutePosition();

                bool extendSelection = evt.keyMask().isShiftDown();
                auto clickPos = m_textBuffer->findCursorPosition(relativePos);
                m_textBuffer->moveCursor(clickPos, extendSelection);

                // TODO: enter drag action only if whole text is NOT yet selected
                return base::RefNew<EditBoxSelectionDragInputAction>(this, m_textBuffer->selectionStartPos());
            }
        }
    }

    return InputActionPtr();
}

bool EditBox::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const
{
    outCursorType = base::input::CursorType::TextBeam;
    return true;
}

//--

END_BOOMER_NAMESPACE(ui)

