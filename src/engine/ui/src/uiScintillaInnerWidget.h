/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scintilla\platform #]
*
***/

#include "build.h"

#include "scintilla/ILoader.h"
#include "scintilla/ILexer.h"
#include "scintilla/SciLexer.h"
#include "scintilla/PropSetSimple.h"
#include "scintilla/Platform.h"
#include "scintilla/Scintilla.h"
#include "scintilla/SciLexer.h"
#include "scintilla/Position.h"
#include "scintilla/UniqueString.h"
#include "scintilla/SplitVector.h"
#include "scintilla/Partitioning.h"
#include "scintilla/RunStyles.h"
#include "scintilla/ContractionState.h"
#include "scintilla/CellBuffer.h"
#include "scintilla/CallTip.h"
#include "scintilla/KeyMap.h"
#include "scintilla/Indicator.h"
#include "scintilla/LineMarker.h"
#include "scintilla/Style.h"
#include "scintilla/ViewStyle.h"
#include "scintilla/CharClassify.h"
#include "scintilla/Decoration.h"
#include "scintilla/CaseFolder.h"
#include "scintilla/Document.h"
#include "scintilla/CaseConvert.h"
#include "scintilla/UniConversion.h"
#include "scintilla/DBCS.h"
#include "scintilla/Selection.h"
#include "scintilla/PositionCache.h"
#include "scintilla/EditModel.h"
#include "scintilla/MarginView.h"
#include "scintilla/EditView.h"
#include "scintilla/Editor.h"
#include "scintilla/AutoComplete.h"
#include "scintilla/ScintillaBase.h"

#include "engine/canvas/include/geometry.h"
#include "engine/canvas/include/geometryBuilder.h"
#include "engine/canvas/include/canvas.h"

#include "uiScintillaPlatform.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

class ScintillaInnerWidgetMouseDragAction;

class ScintillaInnerWidget : public IElement, public Scintilla::ScintillaBase, public ::Scintilla::IWindowInterface
{
    RTTI_DECLARE_VIRTUAL_CLASS(ScintillaInnerWidget, IElement);

public:
    ScintillaInnerWidget(ScintillaTextEditor* owner, const RefPtr<Scrollbar>& scrollbar);

protected:
    virtual void SetVerticalScrollPos() override final;
    virtual void SetHorizontalScrollPos() override final;
    virtual bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) override final;
    virtual void Copy() override final;
    virtual void Paste() override final;
    virtual void ClaimSelection() override final;
    virtual void NotifyChange() override final;
    virtual void NotifyParent(SCNotification scn) override final;
    virtual void CopyToClipboard(const ::Scintilla::SelectionText &selectedText) override final;
    virtual void SetMouseCapture(bool on) override final;
    virtual bool HaveMouseCapture() override final;
    virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) override final;
    virtual void CreateCallTipWindow(::Scintilla::PRectangle rc) override final;
    virtual void AddToPopUp(const char *label, int cmd=0, bool enabled=true) override final;

    virtual ::Scintilla::PRectangle GetPosition() const override final;
    virtual void SetPosition(::Scintilla::PRectangle rc) override final;
    virtual ::Scintilla::PRectangle GetClientPosition() const override final;

    virtual bool FineTickerRunning(TickReason reason) override;
    virtual void FineTickerStart(TickReason reason, int millis, int tolerance) override;
    virtual void FineTickerCancel(TickReason reason) override;

    //--

    // ui implementation
    virtual void renderForeground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity) override;
    virtual bool handleCursorQuery(const ElementArea &area, const Position &absolutePosition, input::CursorType &outCursorType) const override;
    virtual InputActionPtr handleMouseClick(const ElementArea &area, const input::MouseClickEvent &evt) override;
    virtual bool handleMouseMovement(const input::MouseMovementEvent &evt) override;
    virtual bool handleMouseWheel(const input::MouseMovementEvent &evt, float delta) override;
    virtual bool handleKeyEvent(const input::KeyEvent &evt) override;
    virtual bool handleCharEvent(const input::CharEvent &evt) override;
    virtual void handleFocusLost() override;
    virtual void handleFocusGained() override;
    virtual DragDropHandlerPtr handleDragDrop(const DragDropDataPtr& data, const Position& entryPosition) override;
    virtual void computeSize(Size& outSize) const override final;

    //--

    ScintillaTextEditor* m_owner;
    NativeTimePoint m_timeBase;

    static const uint32_t MAX_TIMERS = 10;
    UniquePtr<Timer> m_timers[MAX_TIMERS];

    Timer m_updateScrollBarsTimer;

    RefWeakPtr<ScintillaInnerWidgetMouseDragAction> m_inputAction;
    friend class ScintillaInnerWidgetMouseDragAction;

    RefPtr<Scrollbar> m_scrollbar;
    void updateScrollbar();
};

END_BOOMER_NAMESPACE_EX(ui)

