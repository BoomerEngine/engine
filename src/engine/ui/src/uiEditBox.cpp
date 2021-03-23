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

#include "core/input/include/inputStructures.h"
#include "core/image/include/image.h"
#include "engine/canvas/include/geometryBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

RTTI_BEGIN_TYPE_CLASS(EditBox);
    RTTI_METADATA(ElementClassNameMetadata).name("EditBox");
RTTI_END_TYPE();
    
EditBox::EditBox(EditBoxFeatureFlags features, StringView initialText)
    : ScrollArea(ScrollMode::None, ScrollMode::Hidden)
    , m_features(features)
    , m_cursorToggleInterval(0.5)
{
    layoutMode(LayoutMode::Vertical);
    allowFocusFromKeyboard(true);
    allowFocusFromClick(true);

    hitTest(HitTestState::Enabled);

    const bool multiline = m_features.test(EditBoxFeatureBit::Multiline);
    m_textBuffer = CreateUniquePtr<TextBuffer>(multiline);

    verticalScrollMode(multiline ? ScrollMode::Auto : ScrollMode::None);

    m_cursorToggleTime.resetToNow();
    m_cursorToggleTime += m_cursorToggleInterval;

    if (features.test(EditBoxFeatureBit::NoBorder))
        addStyleClass("noborder"_id);

    text(initialText);

    bindCommands();
}

EditBox::~EditBox()
{
    m_textBuffer.reset();
}

void EditBox::text(StringView txt)
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

void EditBox::postfixText(StringView txt)
{
    m_textBuffer->postfixText(txt);
    invalidateLayout();
    invalidateGeometry();
}

void EditBox::prefixText(StringView txt)
{
    m_textBuffer->prefixText(txt);
    invalidateLayout();
    invalidateGeometry();
}

void EditBox::hintText(StringView txt)
{
    // TODO
}

StringBuf EditBox::text() const
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

StringBuf EditBox::selectedText() const
{
    if (m_textBuffer->hasSelection())
        return m_textBuffer->text(m_textBuffer->selectionStartPos(), m_textBuffer->selectionEndPos());
    else
        return StringBuf::EMPTY();
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

void EditBox::prepareForegroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const
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

void EditBox::renderForeground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
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

bool EditBox::moveCursor(const CursorNavigation& pos, bool extendSelection)
{
    if (!m_textBuffer->moveCursor(pos, extendSelection))
        return false;

    {
        auto cursorCharBounds = m_textBuffer->characterBounds(m_textBuffer->cursorPos().m_char);
        auto cursorPos = Vector2(cursorCharBounds.m_boxMin.x, (cursorCharBounds.m_boxMin.y + cursorCharBounds.m_boxMax.y) * 0.5f);

        ElementArea charArea(cachedDrawArea().absolutePosition() + cursorPos, Size(2, 2));
        //ensureVisible(charArea);
    }

    m_cursorVisible = true;
    m_cursorToggleTime.resetToNow();
    m_cursorToggleTime += m_cursorToggleInterval;

    return true;
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

bool EditBox::handleKeyEvent(const InputKeyEvent& evt)
{
    if (evt.isDown())
    {
        auto pos = m_textBuffer->cursorPos().m_char;

        if (evt.keyCode() == InputKey::KEY_DELETE)
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
        else if (evt.keyCode() == InputKey::KEY_BACK)
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
        else if (evt.keyCode() == InputKey::KEY_LEFT)
        {
            auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::WordStart : TextNavigation::PrevChar;
            auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

            bool extendSelection = evt.keyMask().isShiftDown();
            return moveCursor(newCursorPos, extendSelection);
        }
        else if (evt.keyCode() == InputKey::KEY_RIGHT)
        {
            auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::WordEnd : TextNavigation::NextChar;
            auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

            bool extendSelection = evt.keyMask().isShiftDown();
            return moveCursor(newCursorPos, extendSelection);
        }
        else if (evt.keyCode() == InputKey::KEY_HOME)
        {
            auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::DocumentStart : TextNavigation::LineStart;
            auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

            bool extendSelection = evt.keyMask().isShiftDown();
            return moveCursor(newCursorPos, extendSelection);
        }
        else if (evt.keyCode() == InputKey::KEY_END)
        {
            auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::DocumentEnd : TextNavigation::LineEnd;
            auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

            bool extendSelection = evt.keyMask().isShiftDown();
            return moveCursor(newCursorPos, extendSelection);
        }
        else if (evt.keyCode() == InputKey::KEY_UP)
        {
            if (m_textBuffer->isMultiLine())
            {
                auto navigation = TextNavigation::PrevLineChar;
                auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                bool extendSelection = evt.keyMask().isShiftDown();
                return moveCursor(newCursorPos, extendSelection);
            }
            else
            {
                return false; // explicitly DO NOT HANDLE
            }
        }
        else if (evt.keyCode() == InputKey::KEY_DOWN)
        {
            if (m_textBuffer->isMultiLine())
            {
                auto navigation = TextNavigation::NextLineChar;
                auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                bool extendSelection = evt.keyMask().isShiftDown();
                return moveCursor(newCursorPos, extendSelection);
            }
            else
            {
                return false; // explicitly DO NOT HANDLE
            }
        }
        else if (evt.keyCode() == InputKey::KEY_PRIOR)
        {
            if (m_textBuffer->isMultiLine())
            {
                auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::PrevPage : TextNavigation::PrevPage;
                auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                bool extendSelection = evt.keyMask().isShiftDown();
                return moveCursor(newCursorPos, extendSelection);
            }
        }
        else if (evt.keyCode() == InputKey::KEY_NEXT)
        {
            if (m_textBuffer->isMultiLine())
            {
                auto navigation = evt.keyMask().isCtrlDown() ? TextNavigation::NextPage : TextNavigation::NextPage;
                auto newCursorPos = m_textBuffer->findCursorPosition(m_textBuffer->cursorPos(), navigation);

                bool extendSelection = evt.keyMask().isShiftDown();
                return moveCursor(newCursorPos, extendSelection);
            }
        }
        else if (evt.keyCode() == InputKey::KEY_ESCAPE)
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
        clipboard().storeText(selectedText());
    }
}

void EditBox::cmdCutSelection()
{
    if (canModify())
    {
        if (m_textBuffer->hasSelection())
        {
            clipboard().storeText(selectedText());

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
        StringBuf text;
        if (clipboard().loadText(text))
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
    bindShortcut("Ctrl+C") = [this]() { cmdCopySelection(); };
    bindShortcut("Ctrl+X") = [this]() { cmdCutSelection(); };
    bindShortcut("Ctrl+V") = [this]() { cmdPasteSelection(); };
    bindShortcut("Ctrl+A") = [this]() { cmdSelectAll(); };
    bindShortcut("Ctrl+Z") = [this]() { };
    bindShortcut("Ctrl+Y") = [this]() { };
}

bool EditBox::handleContextMenu(const ElementArea& area, const Position& absolutePosition, InputKeyMask controlKeys)
{
    auto ret = RefNew<MenuButtonContainer>();
    ret->createCallback("Undo", "[img:undo]", "Ctrl+Z") = [this]() {};
    ret->createCallback("Redo", "[img:redo]", "Ctrl+Y") = [this]() {};
    ret->createSeparator();
    ret->createCallback("Copy", "[img:copy]", "Ctrl+Z") = [this]() { cmdCopySelection(); };
    ret->createCallback("Cut", "[img:cut]", "Ctrl+X") = [this]() { cmdCopySelection(); };
    ret->createCallback("Paste", "[img:paste]", "Ctrl+V") = [this]() { cmdPasteSelection(); };
    ret->createCallback("Delete", "[img:cross]", "Delete") = [this]() { cmdDeleteSelection(); };
    ret->createSeparator();
    ret->createCallback("Select all", "", "Ctrl+A") = [this]() { cmdSelectAll(); };
    ret->show(this);

    return true;
}

bool EditBox::handleCharEvent(const InputCharEvent& evt)
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
            StringBuf textToInsert;
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
                textToInsert = StringBuf(BaseStringView<wchar_t>(uniChars));
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

    virtual InputActionResult onKeyEvent(const InputKeyEvent& evt) override
    {
        // any key action resets the event
        if (evt.pressed())
        {
            if (evt.keyCode() == InputKey::KEY_LEFT_CTRL || evt.keyCode() == InputKey::KEY_RIGHT_CTRL)
                return InputActionResult();

/*              if (evt.keyCode() == InputKey::KEY_C && evt.isCtrlDown())
            {
                m_editBox->handleKeyEvent(evt);
                return InputActionResult();
            } 473823
            else if (evt.keyCode() == InputKey::KEY_X && evt.isCtrlDown())
            {
                return InputActionResult();
            }
            else if (evt.keyCode() == InputKey::KEY_V && evt.isCtrlDown())
            {
                return InputActionResult();
            }*/

            if (evt.keyCode() != InputKey::KEY_MOUSE0 && evt.keyCode() != InputKey::KEY_LEFT_SHIFT)
            {
                if (auto editBox = m_editBox.lock())
                    editBox->moveCursor(m_startSelection, false);
                return InputActionResult(nullptr);
            }
        }

        return InputActionResult();
    }

    virtual InputActionResult onMouseEvent(const InputMouseClickEvent& evt, const ElementWeakPtr& hoverStack) override
    {
        if (!evt.leftReleased())
        {
            if (auto editBox = m_editBox.lock())
                editBox->moveCursor(m_startSelection, false);
        }

        return InputActionResult(nullptr);
    }

    virtual InputActionResult onMouseMovement(const InputMouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
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
    RefWeakPtr<EditBox> m_editBox;
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

InputActionPtr EditBox::handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt)
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
                return RefNew<EditBoxSelectionDragInputAction>(this, m_textBuffer->selectionStartPos());
            }
        }
    }

    return InputActionPtr();
}

bool EditBox::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, CursorType& outCursorType) const
{
    outCursorType = CursorType::TextBeam;
    return true;
}

//--

END_BOOMER_NAMESPACE_EX(ui)

