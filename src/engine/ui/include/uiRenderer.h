/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: host #]
***/

#pragma once

#include "uiWindow.h"
#include "core/memory/include/structurePool.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

/// ID of native window
typedef uint32_t NativeWindowID;

/// Callback for native window
class ENGINE_UI_API INativeWindowCallback : public NoCopy
{
public:        
    virtual ~INativeWindowCallback();

    virtual bool nativeWindowSelectCursor(NativeWindowID id, const Position& absolutePosition, input::CursorType& outCursorType) = 0;
    virtual bool nativeWindowHitTestNonClientArea(NativeWindowID id, const Position& absolutePosition, input::AreaType& outAreaType) = 0;
    virtual void nativeWindowForceRedraw(NativeWindowID id) = 0;
};

/// Setup for native window creation
struct ENGINE_UI_API NativeWindowSetup
{
    bool topmost = false; // keep to most
    bool activate = false;
    bool maximized = false;
    bool minimized = false;
    bool popup = false; // a proper popup window
    bool visible = true;
    bool showOnTaskBar = false;
    Point position;
    Point size;
    StringBuf title;
    NativeWindowID owner = (NativeWindowID)0;
    uint64_t externalParentWindowHandle = 0;
    INativeWindowCallback* callback = nullptr;
};

/// Native part of window renderer
class ENGINE_UI_API IRendererNative : public NoCopy
{
public:
    virtual ~IRendererNative();

    /// get last valid position of the mouse cursor
    virtual Position mousePosition() const = 0;

    /// test if the given position is within the region considered the render area (the desktop)
    virtual bool testPosition(const Position& pos) const = 0;

    /// test if the given area is within the region considered the render area (the desktop)
    virtual bool testArea(const ElementArea& area) const = 0;

    /// adjust window area to be placed within operating desktop
    virtual ElementArea adjustArea(const ElementArea& area, WindowInitialPlacementMode placement = WindowInitialPlacementMode::ScreenCenter) const = 0;

    /// play a system sound
    virtual void playSound(MessageType type) = 0;

    //--

    /// create native window
    virtual NativeWindowID windowCreate(const NativeWindowSetup& setup) = 0;

    /// destroy native window
    virtual void windowDestroy(NativeWindowID id) = 0;

    /// get top-level window at given position
    virtual NativeWindowID windowAtPos(const Position& absoluteWindowPosition) = 0;
        
    /// calculate monitor area for position
    virtual ElementArea windowMonitorAtPos(const Position& absoluteWindowPosition) = 0;

    /// pull input event from native window, returns null if queue empty
    virtual input::EventPtr windowPullInputEvent(NativeWindowID id) = 0;

    /// get native window handle
    virtual uint64_t windowNativeHandle(NativeWindowID id) = 0;

    /// show/hide window, newly created windows are initially hidden so SetWindowPos/Size/Title can be called on them first
    virtual void windowShow(NativeWindowID id, bool visible) = 0;

    /// enable/disable window, mostly used to disable the existing window when modal window pops up
    virtual void windowEnable(NativeWindowID id, bool visible) = 0;

    /// set window position
    virtual void windowSetPos(NativeWindowID id, const Position& pos) = 0;

    /// set window size
    virtual void windowSetSize(NativeWindowID id, const Size& size) = 0;

    /// set window placement (both position and size)
    virtual void windowSetPlacement(NativeWindowID id, const Position& pos, const Size& size) = 0;

    /// get window pixel scale
    virtual float windowPixelScale(NativeWindowID id) = 0;

    /// move window to front and set input focus
    virtual void windowSetFocus(NativeWindowID id) = 0;

    /// is this window active ?
    virtual bool windowGetFocus(NativeWindowID id) = 0;

    /// is this window minimized ?
    virtual bool windowGetMinimized(NativeWindowID id) = 0;

    /// is this window maximized ?
    virtual bool windowGetMaximized(NativeWindowID id) = 0;

    /// is this window resizable ? 
    virtual bool windowGetResizable(NativeWindowID id) = 0;

    /// is this window movable ? popup windows are not movable also minimized/maximized are not movable
    virtual bool windowGetMovable(NativeWindowID id) = 0;

    /// is this window visible ?
    virtual bool windowGetVisible(NativeWindowID id) = 0;

    /// set window title bar
    virtual void windowSetTitle(NativeWindowID id, StringView txt) = 0;

    /// set window opacity
    virtual void windowSetOpacity(NativeWindowID id, float opacity) = 0;

    /// update window - push all size/position updates, process all messages
    virtual void windowUpdate(NativeWindowID id) = 0;

    /// render collected content into the window
    virtual void windowRenderContent(NativeWindowID id, const ElementArea& area, bool forcedPaint, const canvas::Canvas& canvas) = 0;

    /// set mouse capture mode
    virtual void windowSetCapture(NativeWindowID id, int mode) = 0;

    /// query current rendering area for the window, we are committed to draw window into that area, returns false if window should not be rendered for some reason
    virtual bool windowGetRenderableArea(NativeWindowID id, ElementArea& outWindowDrawArea) = 0;

    /// query window's default placement (when not maximized or minimized), NOTE: this may be different than the current placement
    virtual bool windowGetDefaultPlacement(NativeWindowID id, Rect& outDefaultPlacement) = 0;
            
    /// check if window has system close request (Alt-F4, pressing the "X" on a window with sytem border, etc)
    virtual bool windowHasCloseRequest(NativeWindowID id) = 0;

    /// minimize window to iconic form
    virtual void windowMinimize(NativeWindowID id) = 0;

    /// maximize window to take full screen
    virtual void windowMaximize(NativeWindowID id) = 0;

    ///--

    /// check if clipboard has data of specific format
    virtual bool checkClipboardHasData(StringView format) = 0;

    /// store data in clipboard
    virtual bool stroreClipboardData(StringView format, const void* data, uint32_t size) = 0;

    /// load data from clipboard
    virtual Buffer loadClipboardData(StringView format) = 0;
};

/// UI renderer, responsible for managing all created windows and rendering them into "viewports" that are created on demand
class ENGINE_UI_API Renderer : public INativeWindowCallback, public IClipboard
{
public:
    Renderer(DataStash* dataStash, IRendererNative* nativeRenderer);
    virtual ~Renderer();

    //---

    // data stash for icons/templates other assets usable in UI
    INLINE DataStash& stash() const { return *m_stash; }

	// get list of all windows
    INLINE const Array<WindowPtr>& windows() const { return m_windowList; }

    // get the current active input action
    INLINE const InputActionPtr& currentInputAction() const { return m_currentInputAction; }

    //---

    /// attach window to the system
    void attachWindow(Window* window, IElement* focusElement = nullptr);

    /// detach window from the system
    /// NOTE: window may be detached without being closed
    void dettachWindow(Window* window);

    //----

    /// attach an timer to tick list
    void attachTimer(IElement* tickable, Timer* timer);

    /// detach an elements from the tick list
    void detachTimer(IElement* tickable, Timer* timer);

    //---

    /// request focus on given element, makes the window contains that element active as well
    void requestFocus(IElement* ptr, bool activateWindow=true);

    /// cancel any displayed tooltip
    void cancelTooltip();

    /// cancel active input action
    void cancelCurrentInputAction();

    //---

    /// update UI state, render all windows
    void updateAndRender(float dt);

    //---

    /// check current window resizable state
    bool queryWindowResizableState(const Window* window) const;

    /// check current window movable state
    bool queryWindowMovableState(const Window* window) const;

    /// query current window placement
    bool queryWindowPlacement(const Window* window, WindowSavedPlacementSetup& outPlacement) const;

    /// get the native window handle of this window, HWND, etc
    uint64_t queryWindowNativeHandle(const Window* window) const;
     
    //--

    /// play nice sound
    void playSound(MessageType type);

    //--

    /// enter modal loop with given window, returns once the window is closed
    void runModalLoop(Window* window, IElement* focusElement = nullptr);

    //--

protected:
    DataStash* m_stash = nullptr;
    IRendererNative* m_native = nullptr;

    uint32_t m_stylesVersion = 0;

    //--

    struct WindowInfo
    {
        WindowPtr window;
        NativeWindowID nativeId = (NativeWindowID)0;
        ElementWeakPtr autoFocus;
        UniquePtr<HitCache> hitCache;
        bool closeAndRemove = false;
        bool resizable = false;
        bool popup = false;
        bool activeState = false;
        bool activeStateSeen = false;
    };

    Array<WindowPtr> m_windowList; // all windows, in attachment order
    Array<WindowInfo> m_windows; // windows, active window last

    //--

    ElementWeakPtr m_currentFocusElement;
    Array<ElementWeakPtr> m_currentFocusElementStack;
    uint32_t m_currentFocusCounter = 0;

    ElementWeakPtr m_requestFocusElement;
    bool m_requestFocusElementSet = false;

    ElementWeakPtr m_currentHoverElement;
    Position m_lastHoverUpdatePosition;
    Position m_lastMouseMovementPosition;
    NativeTimePoint m_lastHoverUpdateTime;

    InputActionPtr m_currentInputAction;

    WindowInfo* windowAtPos(Position absolutePosition);
    WindowInfo* windowForElement(IElement* element);
    NativeWindowID nativeWindowForElement(IElement* element);

    void updateHoverPosition();
    bool isWindowInteractionAllowed(WindowInfo* window) const;

    void focus(IElement* element);
    void updateFocusRequest();
    void updateWindowRepresentation(WindowInfo& window);
    void updateWindowState(WindowInfo& window);

    void processMouseMovement(const input::MouseMovementEvent& evt);
    void processMouseClick(const input::MouseClickEvent& evt);
    void processMouseCaptureLostEvent(const input::MouseCaptureLostEvent& evt);
    void processKeyEvent(const input::KeyEvent& evt);
    void processCharEvent(const input::CharEvent& evt);

    bool processInputActionResult(const InputActionResult& result);

    Size calcWindowContentSize(Window* window, float pixelScale);
    Position calcWindowPlacement(Size& outSize, const WindowInitialPlacementSetup& placement);

    void prepareForModalLoop();
    void returnFromModalLoop();

    ElementArea bestScreenForPosition(Position pos);

    //--

    Point m_currentDragDropClickPos;
    ElementWeakPtr m_currentDragDropSource;
    DragDropDataPtr m_currentDragDropData;
    bool m_currentDragDropStarted = false;
    DragDropHandlerPtr m_currentDragDropHandler;
    PopupPtr m_currentDragDropPreviewPopup;
    ElementPtr m_currentDragDropPreview;
    bool m_currentDragDropCanFinish = false;

    void resetDragDrop();
    bool updateDragDrop(const input::MouseMovementEvent& evt);

    //--

    ElementWeakPtr m_currentTooltipOwner;
    ElementArea m_currentTooltipOwnerArea;
    PopupPtr m_currentTooltip;
    bool m_tooltipFrozenUntilNextMouseMove = false;

    void resetTooltip();
    void updateTooltip();

    //--

    ElementWeakPtr m_potentialContextMenuElementPtr;
    Point m_potentialContextMenuElementClickPos;

    //--

    struct TimerEntry
    {
        ElementWeakPtr owner;
        Timer* timer = nullptr;
        NativeTimePoint callTime;
        TimerEntry* prev = nullptr;
        TimerEntry* next = nullptr;
    };

    TimerEntry* m_timerListHead = nullptr;
    TimerEntry* m_timerListTail = nullptr;
    TimerEntry* m_timerIterationNext = nullptr;
    HashMap<Timer*, TimerEntry*> m_attachedTimers;
    StructurePool<TimerEntry> m_timerEntryPool;

    void processTimers();

    //--

    virtual void storeText(StringView text) override final;
    virtual bool loadText(StringBuf& outText) const override final;

    virtual void storeData(StringView format, const void* data, uint32_t size) override final;
    virtual bool loadData(StringView format, Buffer& outData) const override final;

    //--

    // INativeWindowCallback
    virtual bool nativeWindowSelectCursor(NativeWindowID id, const Position& absolutePosition, input::CursorType& outCursorType) override;
    virtual bool nativeWindowHitTestNonClientArea(NativeWindowID id, const Position& absolutePosition, input::AreaType& outAreaType) override;
    virtual void nativeWindowForceRedraw(NativeWindowID id) override;
};

//---

END_BOOMER_NAMESPACE_EX(ui)
