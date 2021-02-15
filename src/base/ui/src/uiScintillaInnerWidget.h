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

#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvas.h"

#include "uiScintillaPlatform.h"

namespace Scintilla
{
    //---

    class ScintillaInnerWidgetMouseDragAction;

    class ScintillaInnerWidget : public ui::IElement, public ScintillaBase, public IWindowInterface
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScintillaInnerWidget, ui::IElement);

    public:
        ScintillaInnerWidget(ui::ScintillaTextEditor* owner, const base::RefPtr<ui::Scrollbar>& scrollbar);

    protected:
        virtual void SetVerticalScrollPos() override final;
        virtual void SetHorizontalScrollPos() override final;
        virtual bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) override final;
        virtual void Copy() override final;
        virtual void Paste() override final;
        virtual void ClaimSelection() override final;
        virtual void NotifyChange() override final;
        virtual void NotifyParent(SCNotification scn) override final;
        virtual void CopyToClipboard(const SelectionText &selectedText) override final;
        virtual void SetMouseCapture(bool on) override final;
        virtual bool HaveMouseCapture() override final;
        virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) override final;
        virtual void CreateCallTipWindow(PRectangle rc) override final;
        virtual void AddToPopUp(const char *label, int cmd=0, bool enabled=true) override final;

        virtual PRectangle GetPosition() const override final;
        virtual void SetPosition(PRectangle rc) override final;
        virtual PRectangle GetClientPosition() const override final;

        virtual bool FineTickerRunning(TickReason reason) override;
        virtual void FineTickerStart(TickReason reason, int millis, int tolerance) override;
        virtual void FineTickerCancel(TickReason reason) override;

        //--

        // ui implementation
        virtual void renderForeground(ui::DataStash& stash, const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual bool handleCursorQuery(const ui::ElementArea &area, const ui::Position &absolutePosition, base::input::CursorType &outCursorType) const override;
        virtual ui::InputActionPtr handleMouseClick(const ui::ElementArea &area, const base::input::MouseClickEvent &evt) override;
        virtual bool handleMouseMovement(const base::input::MouseMovementEvent &evt) override;
        virtual bool handleMouseWheel(const base::input::MouseMovementEvent &evt, float delta) override;
        virtual bool handleKeyEvent(const base::input::KeyEvent &evt) override;
        virtual bool handleCharEvent(const base::input::CharEvent &evt) override;
        virtual void handleFocusLost() override;
        virtual void handleFocusGained() override;
        virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;
        virtual void computeSize(ui::Size& outSize) const override final;

        //--

        ui::ScintillaTextEditor* m_owner;
        base::NativeTimePoint m_timeBase;

        static const uint32_t MAX_TIMERS = 10;
        base::UniquePtr<ui::Timer> m_timers[MAX_TIMERS];

        ui::Timer m_updateScrollBarsTimer;

        base::RefWeakPtr<ScintillaInnerWidgetMouseDragAction> m_inputAction;
        friend class ScintillaInnerWidgetMouseDragAction;

        base::RefPtr<ui::Scrollbar> m_scrollbar;
        void updateScrollbar();
    };

} // scintilla

