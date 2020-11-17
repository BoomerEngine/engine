/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\window #]
***/

#pragma once

#include "uiElement.h"

namespace ui
{
    
    //---

    DECLARE_UI_EVENT(EVENT_WINDOW_CLOSED)
    DECLARE_UI_EVENT(EVENT_WINDOW_ACTIVATED)
    DECLARE_UI_EVENT(EVENT_WINDOW_DEACTIVATED)
    DECLARE_UI_EVENT(EVENT_WINDOW_MAXIMIZED)
    DECLARE_UI_EVENT(EVENT_WINDOW_MINIMIZED)

    //---

    /// UI Window request for platform that renders the windows
    struct BASE_UI_API WindowRequests : public base::mem::GlobalPoolObject<POOL_UI>
    {
        Size requestedSize;
        Position requestedPosition;
        base::StringBuf requestedTitle; // window title
        bool requestSizeChange = false;
        bool requestPositionChange = false;
        bool requestMinimize = false;
        bool requestMaximize = false;
        bool requestActivation = false;
        bool requestTitleChange = false;
        bool requestShow = false;
        bool requestHide = false;
        bool requestClose = false;
    };

    //---

    /// saved window placement, can be stored in config
    struct WindowSavedPlacementSetup
    {
        Position position;
        Size size;
        bool maximized = false;
        bool minimized = false;
    };


    //---

    /// window features
    enum class WindowFeatureFlagBit : uint32_t
    {
        Resizable = FLAG(0), // allow this window to be resized
        HasTitleBar = FLAG(1), // create the automatic title bar for the window
        ToolWindow = FLAG(2), // if titlebar is used it will be smaller, better for "in-app" windows
        ShowOnTaskBar = FLAG(3), // should this window be listed as a window in the OS? should be set only for main windows
        TopMost = FLAG(4), // window should be topmost
        ShowInactive = FLAG(5), // window should not be activated when shown
        Popup = FLAG(6), // window is a popup window

        CanMaximize = FLAG(10), // shows the "maximize" widget in the title bar
        CanMinimize = FLAG(11), // shows the "minimize" widget in the title bar
        CanClose = FLAG(12), // shows the close button in the title bar
        Maximized = FLAG(13), // start maximized
        Minimized = FLAG(14), // start minimized

        DEFAULT_FRAME = Resizable | HasTitleBar | ShowOnTaskBar | CanMaximize | CanMinimize | CanClose,
        DEFAULT_POPUP = Popup,
        DEFAULT_TOOLTIP = Popup | ShowInactive,
        DEFAULT_DIALOG = HasTitleBar | ToolWindow | CanClose,
        DEFAULT_POPUP_DIALOG = DEFAULT_DIALOG | DEFAULT_POPUP,
        MAIN_WINDOW = DEFAULT_FRAME | Maximized,
    };

    typedef base::DirectFlags<WindowFeatureFlagBit> WindowFeatureFlags;

    //---

    /// initial placement mode
    enum class WindowInitialPlacementMode : uint8_t
    {
        ScreenCenter = 0, // screen center around given position
        TopLeft, // place top left corner of window at the desired position (popup menu)
        TopRight, // place top right corner of window at the desired position
        BottomLeft, // place bottom left corner of window at the desired position
        BottomRight, // place bottom right corner of window at the desired position
        TopRightNeighbour, // place top right corner of window at the desired position
        Maximize, // use whole screen area (of the screen that has the given point)
        WindowCenter, // center in the parent window (if known)
        AreaCenter, // center in the reference area (of if no reference elemetn specified - the cursor) - good for bigger picker dialogs
    };

    /// Window initial placement information
    /// This structure is used by the uiRenderer to let the window describe itself to it, based on this returned data a native window is created
    struct WindowInitialPlacementSetup
    {
        bool flagTopMost = false; // show the window on top of every other window
        bool flagShowOnTaskBar = false; // show the window in the system tray
        bool flagForceActive = false; // force the window to be activated when created
        bool flagAllowResize = false; // after the initial window sizing should the user be allowed to resize the window?
        bool flagPopup = false; // a "proper" popup window 
        bool flagRelativeToCursor = false; // place window relative to current cursor position

        base::StringBuf title; // window title
        base::image::ImagePtr icon; // window icon

        WindowWeakPtr owner = nullptr; // owner of this window, owner window is still considered "active" if one of it's children is, useful for Popup Windows

        uint64_t externalParentWindowHandle = 0; // parent for the modal window

        Position offset; // offset to apply to placedd window
        Position size; // specific size, non zero size on given axis is assumed to be the forced size, the rest is computed from content size, NOTE: if both sizes are zero than window if fitted to it's content size
        WindowInitialPlacementMode mode = WindowInitialPlacementMode::ScreenCenter; // how to place window
        ElementWeakPtr referenceElement;
        WindowSavedPlacementSetup* savedPlacement = nullptr; // saved placement, try to position window exactly there
    };

    ///--

    // A window - basically still a UI element but able to "communicate" with a native OS
    // NOTE: in general the communication is very one way (ie. from window to OS) and in form of "kind requests" that do not have to be fullfilled right away 
    class BASE_UI_API Window : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Window, IElement);

    public:
        Window(WindowFeatureFlags flags, base::StringView title = "Boomer Engine");
        virtual ~Window();

        //--

        // current title
        INLINE const base::StringBuf& title() const { return m_title; }

        // get the "Exit Code" the window was closed with
        INLINE int exitCode() const { return m_exitCode; }

        //--

        // did we request closing of this window ?
        bool requestedClose() const;

        // request window to be closed
        void requestClose(int exitCode = 0);

        // request window to be activated
        void requestActivate();

        // request window to be maximized
        void requestMaximize(bool restoreIfAlreadyMaximized=false);

        // request window to be minimized
        void requestMinimize();

        // request window to be moved to different position
        // NOTE: native site may completly ignore this ;P
        void requestMove(const Position& screenPosition);

        // request window to be resized
        // NOTE: native site may completly ignore this ;P
        void requestSize(const Size& screenSize);

        // request change of a title
        void requestTitleChange(base::StringView newTitle);

        // request window visibility change
        void requestShow(bool activate = false);

        // request window visibility change
        void requestHide();

        //--

        /// handle an external close request (user clicked the X on the window)
        /// if you wish to close the window for real call the requestClose(), otherwise do nothing
        virtual void handleExternalCloseRequest();

        /// handle external activation request for this window
        virtual void handleExternalActivation(bool isActive);

        /// query initial setup for window
        virtual void queryInitialPlacementSetup(WindowInitialPlacementSetup& outSetup) const;

        /// query current "saved placement" for this window
        virtual void queryCurrentPlacementForSaving(WindowSavedPlacementSetup& outPlacement) const;

        /// check if we can resize the window
        virtual bool queryResizableState() const;

        /// check if we can move the window
        virtual bool queryMovableState() const;

        //--

        /// get the native window this element belongs to, only true if we are inside a window :)
        virtual Window* findParentWindow() const override;

        //--

        /// Show window as modal window on given owner, owner must have valid renderer
        /// This function returns the exit code the window was closed with
        int runModal(IElement* owner);

        //--

    private:
        WindowRequests* m_requests = nullptr; // pending requests

        WindowRequests* createPendingRequests(); // will create the request structure, if missing
        WindowRequests* consumePendingRequests(); // will release the requests 

        mutable base::Color m_clearColor = base::Color(0,0,0,0); // color to fill the whole window background to
        mutable bool m_clearColorHack = false;

        float m_lastPixelScale = 1.0f;
        uint64_t m_parentModalWindowHandle = 0;
        int m_exitCode = 0;

        base::StringBuf m_title;
        WindowFeatureFlags m_flags;

        ElementWeakPtr m_modalOwner;

        void prepare(DataStash& stash, float nativePixelScale, bool initial=false, bool forceUpdate=false);
        void render(base::canvas::Canvas& canvas, HitCache& hitCache, const ElementArea& nativeArea);

        virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
        virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const override;
        virtual bool handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const override;
        virtual void prepareBackgroundGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;
        virtual void renderBackground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;

        bool handleWindowFrameArea(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const;
        static int QuerySizeCode(const ElementArea& area, const Position& absolutePosition);

        friend class Renderer;
        friend class PopupWindow;
    };

    //--

} // ui