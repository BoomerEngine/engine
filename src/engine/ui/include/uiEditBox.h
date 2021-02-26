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

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

DECLARE_UI_EVENT(EVENT_TEXT_MODIFIED, StringBuf)
DECLARE_UI_EVENT(EVENT_TEXT_ACCEPTED, StringBuf)
DECLARE_UI_EVENT(EVENT_TEXT_VALIDATION_CHANGED, bool)

//--

class TextBuffer;
struct CursorNavigation;
class EditBoxSelectionDragInputAction;

//--

enum class EditBoxFeatureBit : uint16_t
{
    Multiline = FLAG(0), // uses enter to create new line
    AcceptsEnter = FLAG(1), // eat the "enter", usually it's not eaten and propagates upwards (ie. to dialog etc)
    AcceptsTab = FLAG(2), // should we eat the tab
    NoBorder = FLAG(3), // drop any styling border, create the MEAT of the text editor, not a fancy control
};

typedef DirectFlags<EditBoxFeatureBit> EditBoxFeatureFlags;

//--

/// a simple editable text box, core functionality - no border, style, etc
class ENGINE_UI_API EditBox : public ScrollArea
{
    RTTI_DECLARE_VIRTUAL_CLASS(EditBox, ScrollArea);

public:
    EditBox(EditBoxFeatureFlags flags = EditBoxFeatureFlags(), StringView initialText = "");
    virtual ~EditBox();

    ///----

    /// get features
    INLINE EditBoxFeatureFlags features() const { return m_features; }

    // get validation function
    INLINE const TInputValidationFunction& validation() const { return m_validation; }

    // do we have valid content ?
    INLINE bool validationResult() const { return m_validationResult; }

    //--

    // get text for the whole buffer (UTF8)
    StringBuf text() const;

    // set text, forces regeneration of the glyph buffer and resets selection (UTF8)
    void text(StringView txt);

    // set validation function
    void validation(const TInputValidationFunction& func);

    ///---

    // select whole text
    void selectWholeText();

    // reset selection
    void clearSelection();

    // get selected text (UTF8)
    StringBuf selectedText() const;

    ///----

    // set the non-editable postfix text (usually used for units)
    void postfixText(StringView txt);

    // set the non-editable prefix text (usually used for simple prompt)
    void prefixText(StringView txt);

    // set the non-editable background "hint" text that is only shown if there's no content
    void hintText(StringView txt);

    ///---

private:
    EditBoxFeatureFlags m_features;

    UniquePtr<TextBuffer> m_textBuffer;

    bool m_cursorVisible = true;
    NativeTimePoint m_cursorToggleTime;
    NativeTimeInterval m_cursorToggleInterval;

    TInputValidationFunction m_validation;
    bool m_validationResult = true;

    int m_focusedClickConter = 0;

    // IElement
    virtual void computeLayout(ElementLayout& outLayout) override;
    virtual void computeSize(Size& outSize) const override;
    virtual void prepareForegroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const override;
    virtual void renderForeground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity) override;

    virtual bool handleKeyEvent(const input::KeyEvent& evt) override;
    virtual bool handleCharEvent(const input::CharEvent& evt) override;
    virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, input::CursorType& outCursorType) const override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const input::MouseClickEvent& evt) override;
    virtual bool handleContextMenu(const ElementArea& area, const Position& absolutePosition, input::KeyMask controlKeys) override;
    virtual void handleFocusGained() override;

    void moveCursor(const CursorNavigation& pos, bool extendSelection);

    void cmdCopySelection();
    void cmdCutSelection();
    void cmdPasteSelection();
    void cmdDeleteSelection();
    void cmdSelectAll();
    void bindCommands();

    void textModified();
    void recheckValidation();

    bool canModify() const;

    friend class EditBoxSelectionDragInputAction;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
