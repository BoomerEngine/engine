/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scintilla\platform #]
*
***/

#include "build.h"
#include "uiScintillaTextEditor.h"
#include "uiScintillaPlatform.h"
#include "uiScintillaTextEditor.h"
#include "uiScintillaInnerWidget.h"

#include "base/canvas/include/canvas.h"
#include "base/containers/include/utf8StringFunctions.h"
#include "uiInputAction.h"
#include "uiScrollBar.h"
#include "uiEditBox.h"

namespace Scintilla
{

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ScintillaInnerWidget);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("ScintillaInnerWidget");
    RTTI_END_TYPE();

    static uint32_t ToSciColor(uint32_t r, uint32_t g, uint32_t b)
    {
        //r = (uint32_t)std::clamp(powf(r / 255.0f, 1.0f / 2.2f) * 255.0f, 0.0f, 255.0f);
        //g = (uint32_t)std::clamp(powf(g / 255.0f, 1.0f / 2.2f) * 255.0f, 0.0f, 255.0f);
        //b = (uint32_t)std::clamp(powf(b / 255.0f, 1.0f / 2.2f) * 255.0f, 0.0f, 255.0f);
        return r | (g << 8) | (b << 16);
    }

    ScintillaInnerWidget::ScintillaInnerWidget(ui::ScintillaTextEditor* owner, const base::RefPtr<ui::Scrollbar>& scrollbar)
        : m_owner(owner)
        , m_scrollbar(scrollbar)
        , m_updateScrollBarsTimer(this)
    {
        // setup window
        wMain = (WindowID)static_cast<IWindowInterface*>(this);

        // setup timebase
        m_timeBase = base::NativeTimePoint::Now();

        // request dynamic drawing every frame since we don't do any caching here
        hitTest(ui::HitTestState::Enabled);

        // update scrollbars
        m_updateScrollBarsTimer = [this]() { updateScrollbar();  };
        m_updateScrollBarsTimer.startRepeated(0.001f);
        
        // scroll event
        if (scrollbar)
        {
            scrollbar->bind(ui::EVENT_SCROLL) = [this](float value)
            {
                auto pos = value / vs.lineHeight;
                SetTopLine(pos);
            };
        }

        // disable view buffering (since we do it ourselves)
        WndProc(SCI_SETBUFFEREDDRAW, 0, 0);

        // configure carret
        WndProc(SCI_SETCARETPERIOD, 500, 0);
        WndProc(SCI_SETCARETSTYLE, 1, 0);
        WndProc(SCI_SETCARETLINEVISIBLE, 1, 0);

        // configure margins
        WndProc(SCI_SETMARGINS, 2, 0);
        WndProc(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
        WndProc(SCI_SETMARGINWIDTHN, 0, WndProc(SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t)"_9999"));
        WndProc(SCI_SETMARGINTYPEN, 1, SC_MARGIN_TEXT);
        WndProc(SCI_SETMARGINWIDTHN, 1, WndProc(SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t)"9"));

        // test
        WndProc(SCI_SETLEXER, SCLEX_LUA, 0);
        WndProc(SCI_SETWORDCHARS, 0, (sptr_t)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

        WndProc(SCI_STYLESETBACK, STYLE_DEFAULT, ToSciColor(43,43,43));
        WndProc(SCI_STYLESETFORE, STYLE_DEFAULT, ToSciColor(178,178,178));
        WndProc(SCI_STYLECLEARALL, 0, 0);

        WndProc(SCI_STYLESETFORE, STYLE_LINENUMBER, ToSciColor(92,94,95));
        WndProc(SCI_STYLESETBACK, STYLE_LINENUMBER, ToSciColor(49,51,53));
        WndProc(SCI_SETCARETLINEBACK, ToSciColor(50,50,50), 0);
        WndProc(SCI_SETCARETFORE, ToSciColor(200,200,200), 0);
        WndProc(SCI_SETCARETLINEBACK, ToSciColor(50,50,50), 0);
        WndProc(SCI_SETSELFORE, false, ToSciColor(50,50,50));
        WndProc(SCI_SETSELBACK, true, ToSciColor(33,66,131));

        WndProc(SCI_STYLESETFORE, SCE_LUA_COMMENT, ToSciColor(128,128,128));
        WndProc(SCI_STYLESETFORE, SCE_LUA_COMMENTLINE, ToSciColor(128,128,128));
        WndProc(SCI_STYLESETFORE, SCE_LUA_NUMBER, ToSciColor(80,141,185));
        WndProc(SCI_STYLESETFORE, SCE_LUA_WORD, ToSciColor(203,120,50));
        WndProc(SCI_STYLESETFORE, SCE_LUA_WORD2, ToSciColor(203,120,50));
        WndProc(SCI_STYLESETFORE, SCE_LUA_WORD3, ToSciColor(203,120,50));
        WndProc(SCI_STYLESETFORE, SCE_LUA_STRING, ToSciColor(80,126,88));
        WndProc(SCI_STYLESETFORE, SCE_LUA_CHARACTER, ToSciColor(80,88,126));
        WndProc(SCI_STYLESETFORE, SCE_LUA_LITERALSTRING, ToSciColor(80,126,88));
        WndProc(SCI_STYLESETBACK, SCE_LUA_STRINGEOL, ToSciColor(100,43,43));
        WndProc(SCI_STYLESETFORE, SCE_LUA_OPERATOR, ToSciColor(202,110,47));
        //WndProc(SCI_STYLESETFORE, SCE_LUA_PREPROCESSOR, 0);
        WndProc(SCI_STYLESETBOLD, SCE_LUA_WORD, true);
        WndProc(SCI_STYLESETBOLD, SCE_LUA_WORD2, true);
        WndProc(SCI_STYLESETBOLD, SCE_LUA_WORD3, true);
        WndProc(SCI_SETKEYWORDS, 0, (sptr_t)(void*)"and break do else elseif end for function if in local nil not or repeat return then until while false true goto");
        WndProc(SCI_SETKEYWORDS, 1, (sptr_t)(void*)"assert collectgarbage dofile error _G getmetatable ipairs loadfile next pairs pcall print rawequal rawget rawset setmetatable tonumber tostring type _VERSION xpcall string table math coroutine io os debug getfenv gcinfo load loadlib loadstring require select setfenv unpack _LOADED LUA_PATH _REQUIREDNAME package rawlen package bit32 utf8 _ENV");
        WndProc(SCI_SETKEYWORDS, 2, (sptr_t)(void*)"string.byte string.char string.dump string.find string.format string.gsub string.len string.lower string.rep string.sub string.upper table.concat table.insert table.remove table.sort math.abs math.acos math.asin math.atan math.atan2 math.ceil math.cos math.deg math.exp math.floor math.frexp math.ldexp math.log math.max math.min math.pi math.pow math.rad math.random math.randomseed math.sin math.sqrt math.tan string.gfind string.gmatch string.match string.reverse string.pack string.packsize string.unpack table.foreach table.foreachi table.getn table.setn table.maxn table.pack table.unpack table.move math.cosh math.fmod math.huge math.log10 math.modf math.mod math.sinh math.tanh math.maxinteger math.mininteger math.tointeger math.type math.ult bit32.arshift bit32.band bit32.bnot bit32.bor bit32.btest bit32.bxor bit32.extract bit32.replace bit32.lrotate bit32.lshift bit32.rrotate bit32.rshift utf8.char utf8.charpattern utf8.codes utf8.codepoint utf8.len utf8.offset");
    }

    ui::DragDropHandlerPtr ScintillaInnerWidget::handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
    {
        return nullptr;
    }

    bool ScintillaInnerWidget::handleCursorQuery(const ui::ElementArea &area, const ui::Position &absolutePosition, base::input::CursorType &outCursorType) const
    {
        outCursorType = base::input::CursorType::TextBeam;
        return true;
    }

    class ScintillaInnerWidgetMouseDragAction : public ui::MouseInputAction
    {
    public:
        ScintillaInnerWidgetMouseDragAction(ui::IElement* owner)
            : ui::MouseInputAction(owner, base::input::KeyCode::KEY_MOUSE0, true)
            , m_requestExit(false)
        {}

        INLINE void requestExit()
        {
            m_requestExit = true;
        }

        virtual ui::InputActionResult onUpdate(float dt) override
        {
            if (m_requestExit)
                return nullptr;
            return ui::MouseInputAction::onUpdate(dt);
        }

        virtual ui::InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ui::ElementWeakPtr& hoverStack) override
        {
            auto widget = element();
            if (widget)
                widget->handleMouseMovement(evt);
            return ui::InputActionResult();
        }

    private:
        bool m_requestExit;
    };

    ui::InputActionPtr ScintillaInnerWidget::handleMouseClick(const ui::ElementArea &area, const base::input::MouseClickEvent &evt)
    {
        auto pos = evt.absolutePosition() - area.absolutePosition();
        auto time = (int)m_timeBase.timeTillNow().toMiliSeconds();

        int modifiers = 0;
        if (evt.keyMask().isShiftDown()) modifiers |= SCI_SHIFT;
        if (evt.keyMask().isCtrlDown()) modifiers |= SCI_CTRL;
        if (evt.keyMask().isAltDown()) modifiers |= SCI_ALT;

        if (evt.leftClicked())
            ButtonDownWithModifiers(Point(pos.x, pos.y), time, modifiers);
        else if (evt.rightClicked())
            RightButtonDownWithModifiers(Point(pos.x, pos.y), time, modifiers);
        else if (evt.leftReleased())
            ButtonUpWithModifiers(Point(pos.x, pos.y), time, modifiers);

        if (evt.leftClicked())
        {
            auto action = base::RefNew<ScintillaInnerWidgetMouseDragAction>(this);
            m_inputAction = action;
            return action;
        }
        else
        {
            return nullptr;
        }
    }

    bool ScintillaInnerWidget::handleMouseMovement(const base::input::MouseMovementEvent &evt)
    {
        if (evt.keyMask().isLeftDown())
        {
            int modifiers = 0;
            if (evt.keyMask().isShiftDown()) modifiers |= SCI_SHIFT;
            if (evt.keyMask().isCtrlDown()) modifiers |= SCI_CTRL;
            if (evt.keyMask().isAltDown()) modifiers |= SCI_ALT;

            auto pos = evt.absolutePosition() - cachedDrawArea().absolutePosition();
            auto time = (int)m_timeBase.timeTillNow().toMiliSeconds();

            ButtonMoveWithModifiers(Point(pos.x, pos.y), time, modifiers);
            return true;
        }

        return TBaseClass::handleMouseMovement(evt);
    }

    bool ScintillaInnerWidget::handleMouseWheel(const base::input::MouseMovementEvent &evt, float delta)
    {
        if (evt.keyMask().isCtrlDown())
        {
            if (delta < 0)
                KeyCommand(SCI_ZOOMIN);
            else
                KeyCommand(SCI_ZOOMOUT);
        }
        else
        {
            if (delta < 0)
                WndProc(SCI_LINESCROLL, 0, 1);
            else
                WndProc(SCI_LINESCROLL, 0, -1);
        }

        return true;
    }

    bool ScintillaInnerWidget::handleKeyEvent(const base::input::KeyEvent &evt)
    {
        if (evt.pressedOrRepeated())
        {
            switch (evt.keyCode())
            {
                case base::input::KeyCode::KEY_LEFT:
                    if (evt.keyMask().isShiftDown())
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_WORDLEFTEXTEND);
                        else
                            KeyCommand(SCI_CHARLEFTEXTEND);
                    }
                    else
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_WORDLEFT);
                        else
                            KeyCommand(SCI_CHARLEFT);
                    }
                    return true;

                case base::input::KeyCode::KEY_RIGHT:
                    if (evt.keyMask().isShiftDown())
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_WORDRIGHTENDEXTEND);
                        else
                            KeyCommand(SCI_CHARRIGHTEXTEND);
                    }
                    else
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_WORDRIGHT);
                        else
                            KeyCommand(SCI_CHARRIGHT);
                    }
                    return true;

                case base::input::KeyCode::KEY_UP:
                    if (evt.keyMask().isShiftDown())
                        KeyCommand(SCI_LINEUPEXTEND);
                    else
                        KeyCommand(SCI_LINEUP);
                    return true;

                case base::input::KeyCode::KEY_DOWN:
                    if (evt.keyMask().isShiftDown())
                        KeyCommand(SCI_LINEDOWNEXTEND);
                    else
                        KeyCommand(SCI_LINEDOWN);
                    return true;

                case base::input::KeyCode::KEY_PRIOR:
                    if (evt.keyMask().isShiftDown())
                        KeyCommand(SCI_PAGEUPEXTEND);
                    else
                        KeyCommand(SCI_PAGEUP);
                    return true;

                case base::input::KeyCode::KEY_NEXT:
                    if (evt.keyMask().isShiftDown())
                        KeyCommand(SCI_PAGEDOWNEXTEND);
                    else
                        KeyCommand(SCI_PAGEDOWN);
                    return true;

                case base::input::KeyCode::KEY_HOME:
                    if (evt.keyMask().isShiftDown())
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_DOCUMENTSTARTEXTEND);
                        else
                            KeyCommand(SCI_VCHOMEEXTEND);
                    }
                    else
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_DOCUMENTSTART);
                        else
                            KeyCommand(SCI_VCHOME);
                    }
                    return true;

                case base::input::KeyCode::KEY_END:
                    if (evt.keyMask().isShiftDown())
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_DOCUMENTENDEXTEND);
                        else
                            KeyCommand(SCI_LINEENDEXTEND);
                    }
                    else
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_DOCUMENTEND);
                        else
                            KeyCommand(SCI_LINEEND);
                    }
                    return true;

                case base::input::KeyCode::KEY_ESCAPE:
                    KeyCommand(SCI_CANCEL);
                    return true;

                case base::input::KeyCode::KEY_DELETE:
                    if (evt.keyMask().isShiftDown())
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_DELLINERIGHT);
                        else
                            WndProc(SCI_CUT, 0, 0);
                    }
                    else
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_DELWORDRIGHT);
                        else
                            WndProc(SCI_CLEAR, 0, 0);
                    }
                    return true;

                case base::input::KeyCode::KEY_BACK:
                    if (evt.keyMask().isShiftDown())
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_DELLINELEFT);
                        else
                            KeyCommand(SCI_DELETEBACK);
                    }
                    else
                    {
                        if (evt.keyMask().isCtrlDown())
                            KeyCommand(SCI_DELWORDLEFT);
                        else
                            KeyCommand(SCI_DELETEBACK);
                    }
                    return true;

                case base::input::KeyCode::KEY_TAB:
                    KeyCommand(SCI_TAB);
                    return true;

                case base::input::KeyCode::KEY_RETURN:
                    KeyCommand(SCI_NEWLINE);
                    return true;
            }
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    void ScintillaInnerWidget::handleFocusLost()
    {
        TBaseClass::handleFocusLost();
        SetFocusState(false);
        DropCaret();
        FineTickerCancel(tickCaret);
    }

    void ScintillaInnerWidget::handleFocusGained()
    {
        TBaseClass::handleFocusGained();
        SetFocusState(true);
        ShowCaretAtCurrentPosition();
    }

    bool ScintillaInnerWidget::handleCharEvent(const base::input::CharEvent &evt)
    {
        auto key = (wchar_t)evt.scanCode();
        if (key >= ' ')
        {
            // convert to UTF8
            char text[10];
            auto len = base::utf8::FromUniChar(text, ARRAY_COUNT(text), &key, 1);
            if (len > 1)
                AddCharUTF(text, len - 1);

            // handled
            return true;
        }

        return false;
    }

    PRectangle ScintillaInnerWidget::GetPosition() const
    {
        return FromRect(cachedDrawArea());
    }

    void ScintillaInnerWidget::SetPosition(PRectangle rc)
    {
        // TODO
    }

    PRectangle ScintillaInnerWidget::GetClientPosition() const
    {
        auto size = cachedDrawArea().size();
        return PRectangle(0.0f, 0.0f, size.x, size.y);
    }

    void ScintillaInnerWidget::computeSize(ui::Size& outSize) const
    {
        TBaseClass::computeSize(outSize);

        outSize.x = 100;
        outSize.y = 100;
    }

    void ScintillaInnerWidget::renderForeground(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        TBaseClass::renderForeground(drawArea, canvas, mergedOpacity);

        // get the rendering size
        auto width = (int)drawArea.size().x;
        auto height = (int)drawArea.size().y;
        auto rect = base::Rect(0, 0, width, height);

        // render Scintilla into a static geometry
        auto surf = (Scintilla::SurfaceImpl*)Scintilla::Surface::Allocate(0);
        surf->Init(wMain.GetID());
        surf->SetUnicodeMode(true);
        surf->SetDBCSMode(SC_CP_UTF8);
        Paint(surf, Scintilla::FromRect(rect));

        // render geometry into canvas
		const int x = (int)std::round(drawArea.absolutePosition().x);
		const int y = (int)std::round(drawArea.absolutePosition().y);
        surf->render(x, y, canvas);

        // cleanup
        delete surf;
    }

    //--

    bool ScintillaInnerWidget::FineTickerRunning(TickReason reason)
    {
        return m_timers[reason] && m_timers[reason]->running();
    }

    void ScintillaInnerWidget::FineTickerStart(TickReason reason, int millis, int tolerance)
    {
        if (!m_timers[reason])
        {
            m_timers[reason].create(this);
            *m_timers[reason] = [this, reason]() { TickFor(reason); };
        }

        m_timers[reason]->stop();
        m_timers[reason]->startRepeated(millis / 1000.0f);
    }

    void ScintillaInnerWidget::FineTickerCancel(TickReason reason)
    {
        m_timers[reason].reset();
    }

    void ScintillaInnerWidget::updateScrollbar()
    {
        if (m_scrollbar)
        {
            auto viewHeight = (int) cachedDrawArea().size().y;

            auto docHeight = pcs->LinesDisplayed() * vs.lineHeight;
            if (!endAtLastLine)
                docHeight += (int(viewHeight / vs.lineHeight) - 3) * vs.lineHeight;

            // Allow extra space so that last scroll position places whole line at top
            int clipExtra = int(viewHeight) % vs.lineHeight;
            docHeight += clipExtra;

            m_scrollbar->visibility(docHeight > viewHeight);
            m_scrollbar->scrollAreaSize(docHeight);
            m_scrollbar->scrollThumbSize(viewHeight);
        }
    }

    void ScintillaInnerWidget::SetVerticalScrollPos()
    {
        if (m_scrollbar)
        {
            auto topPosition = topLine * vs.lineHeight;
            m_scrollbar->scrollPosition(topPosition, false);
        }
    }

    void ScintillaInnerWidget::SetHorizontalScrollPos()
    {}

    bool ScintillaInnerWidget::ModifyScrollBars(Sci::Line nMax, Sci::Line nPage)
    {
        SetVerticalScrollPos();
        return true;
    }

    void ScintillaInnerWidget::Copy()
    {
        if (!sel.Empty())
        {
            SelectionText selectedText;
            CopySelectionRange(&selectedText);
            CopyToClipboard(selectedText);
        }
    }

    void ScintillaInnerWidget::Paste()
    {
        //auto *windowRenderer = this->windowRenderer();
        //if (windowRenderer)
        {
            base::Buffer data;
            //if (windowRenderer->loadClipboardData("Text"_id, data))
            {
                auto text = base::StringBuf(data);

                pdoc->BeginUndoAction();
                ClearSelection(false);
                InsertPasteShape(text.c_str(), text.length(), pasteStream);
                pdoc->EndUndoAction();
            }
        }
    }

    void ScintillaInnerWidget::ClaimSelection()
    {}

    void ScintillaInnerWidget::NotifyChange()
    {
        auto parent = findParent<ui::ScintillaTextEditor>();
        if (parent)
            parent->call(ui::EVENT_TEXT_MODIFIED);
    }

    void ScintillaInnerWidget::NotifyParent(SCNotification scn)
    {}

    void ScintillaInnerWidget::CopyToClipboard(const SelectionText &selectedText)
    {
        //auto *windowRenderer = this->windowRenderer();
        //if (windowRenderer)
        {
            auto data = base::StringView(selectedText.Data()).toBuffer();
            auto format = "Text"_id;
            //windowRenderer->storeClipboardData(&format, &data, 1);
        }
    }

    void ScintillaInnerWidget::SetMouseCapture(bool on)
    {
        if (!on)
        {
            auto action = m_inputAction.lock();
            if (action)
                action->requestExit();
            m_inputAction.reset();
        }
    }

    bool ScintillaInnerWidget::HaveMouseCapture()
    {
        auto action = m_inputAction.lock();
        return !!action;
    }

    sptr_t ScintillaInnerWidget::DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam)
    {
        return 0;//ScintillaBase::DefWndProc(iMessage, wParam, lParam);
    }

    void ScintillaInnerWidget::CreateCallTipWindow(PRectangle rc)
    {}

    void ScintillaInnerWidget::AddToPopUp(const char *label, int cmd, bool enabled)
    {}

} // scintilla
