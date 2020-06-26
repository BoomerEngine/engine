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
    class WindowFrame;
    class WindowMessage;

    /// UI Window request for platform that renders the windows
    struct BASE_UI_API WindowRequests
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

    /// saved window placement
    struct WindowSavedPlacementSetup
    {
        Position position;
        Size size;
        bool maximized = false;
        bool minimized = false;
    };

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

    /// window state flags, as prompted by native window
    enum class WindowStateFlagBit : uint8_t
    {
        Resizable = FLAG(0),
        HasSystemTitleBar = FLAG(1),
        CanMaximize = FLAG(2),
        CanMinimize = FLAG(3),
        CanClose = FLAG(4),
        Maximized = FLAG(5),
        Minimized = FLAG(6),
        Active = FLAG(7),
    };

    typedef base::DirectFlags<WindowStateFlagBit> WindowStateFlags;


    /// window initial placement information
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

        Position offset; // offset to apply to placedd window
        Position size; // specific size, non zero size on given axis is assumed to be the forced size, the rest is computed from content size, NOTE: if both sizes are zero than window if fitted to it's content size
        WindowInitialPlacementMode mode = WindowInitialPlacementMode::ScreenCenter; // how to place window
        ElementWeakPtr referenceElement;
        WindowSavedPlacementSetup* savedPlacement = nullptr; // saved placement, try to position window exactly there
    };

    /// window
    class BASE_UI_API Window : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Window, IElement);

    public:
        Window();
        virtual ~Window();

        //--

        // current title
        INLINE const base::StringBuf& title() const { return m_title; }

        //--

        // request window to be closed
        void requestClose();

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
        void requestTitleChange(base::StringView<char> newTitle);

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

        /// handle flag update
        virtual void handleStateUpdate(WindowStateFlags flags);

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

        // refresh all UI cached data, done after reloads
        static void ForceRedrawOfEverything();

        //--

        /// show as a modal window
        void showModal(IElement* owner);

    private:
        WindowRequests* m_requests = nullptr; // pending requests

        WindowRequests* createPendingRequests(); // will create the request structure, if missing
        WindowRequests* consumePendingRequests(); // will release the requests 

        mutable base::Color m_clearColor = base::Color(0,0,0,0); // color to fill the whole window background to
        mutable bool m_clearColorHack = false;

        float m_lastPixelScale = 1.0f;
        bool m_hasMaximizeButton = true;
        bool m_hasMinimizeButton = true;
        bool m_hasCloseButton = true;
        bool m_resizable = true;
        base::StringBuf m_title;

        ElementWeakPtr m_modalOwner;

        void prepare(DataStash& stash, float nativePixelScale, bool initial=false, bool forceUpdate=false);
        void render(base::canvas::Canvas& canvas, HitCache& hitCache, const ElementArea& nativeArea);

        virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
        virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const override;
        virtual bool handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const override;
        virtual bool handleTemplateProperty(base::StringView<char> name, base::StringView<char> value) override final;
        virtual void prepareBackgroundGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;
        virtual void renderBackground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;

        bool handleWindowFrameArea(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const;
        static int QuerySizeCode(const ElementArea& area, const Position& absolutePosition);

        friend class Renderer;
        friend class PopupWindow;
    };

    //--

    /// popup window setup
    struct PopupWindowSetup
    {
        WindowInitialPlacementMode m_mode = WindowInitialPlacementMode::BottomLeft;
        Size m_initialSize = Size(0, 0); // use content size 
        Position m_initialOffset = Position(0, 0); // no offset
        bool m_interactive = true;
        bool m_closeWhenDeactivated = true;
        bool m_relativeToCursor = false;
        WindowWeakPtr m_popupOwner;

        INLINE PopupWindowSetup() {};
        INLINE PopupWindowSetup& relativeToCursor() { m_relativeToCursor = true; return *this; }
        INLINE PopupWindowSetup& size(Size size) { m_initialSize = size; return *this; }
        INLINE PopupWindowSetup& size(float x, float y) { m_initialSize = Size(x, y); return *this; }
        INLINE PopupWindowSetup& offset(Position pos) { m_initialOffset = pos; return *this; }
        INLINE PopupWindowSetup& offset(float x, float y) { m_initialOffset = Position(x, y); return *this; }
        INLINE PopupWindowSetup& mode(WindowInitialPlacementMode mode) { m_mode = mode; return *this; }
        INLINE PopupWindowSetup& bottomLeft() { m_mode = WindowInitialPlacementMode::BottomLeft; return *this; }
        INLINE PopupWindowSetup& bottomRight() { m_mode = WindowInitialPlacementMode::BottomRight; return *this; }
        INLINE PopupWindowSetup& topLeft() { m_mode = WindowInitialPlacementMode::TopLeft; return *this; }
        INLINE PopupWindowSetup& topRight() { m_mode = WindowInitialPlacementMode::TopRight; return *this; }
        INLINE PopupWindowSetup& topRightNeighbour() { m_mode = WindowInitialPlacementMode::TopRightNeighbour; return *this; }
        INLINE PopupWindowSetup& center() { m_mode = WindowInitialPlacementMode::WindowCenter; return *this; }
        INLINE PopupWindowSetup& screenCenter() { m_mode = WindowInitialPlacementMode::ScreenCenter; return *this; }
        INLINE PopupWindowSetup& areaCenter() { m_mode = WindowInitialPlacementMode::AreaCenter; return *this; }
        INLINE PopupWindowSetup& interactive(bool flag=true) { m_interactive = flag; return *this; }
        INLINE PopupWindowSetup& autoClose(bool flag=true) { m_closeWhenDeactivated = flag; return *this; }
        INLINE PopupWindowSetup& owner(Window* window) { m_popupOwner = window; }
    };

    /// popup is a simple window that will close 
    class BASE_UI_API PopupWindow : public Window
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PopupWindow, Window);

    public:
        PopupWindow();

        /// get owner of this popup
        INLINE const WindowWeakPtr& owner() const { return m_setup.m_popupOwner; }

        /// setup popup position/size to pop at, shows the popup
        void show(IElement* owner, const PopupWindowSetup& setup = PopupWindowSetup());

    protected:
        ElementWeakPtr m_parent; // we auto close if parent is gone
        PopupWindowSetup m_setup;

        virtual bool queryResizableState() const override;
        virtual void queryInitialPlacementSetup(WindowInitialPlacementSetup& outSetup) const override;
        virtual void handleExternalActivation(bool isActive) override;
        virtual bool runAction(base::StringID name) override;
        virtual ActionStatusFlags checkAction(base::StringID name) const override;
    };

    //--

    /// title bar for the window, allows to move the window and, if window supports resizing, also maximize/minimize it
    /// NOTE: add as a first element to the window's sizer
    class BASE_UI_API WindowTitleBar : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WindowTitleBar, IElement);

    public:
        WindowTitleBar(base::StringView<char> title = "");
        virtual ~WindowTitleBar();

        void updateWindowState(ui::Window* window, WindowStateFlags flags);

    private:
        virtual bool handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const override final;

        ButtonPtr m_minimizeButton;
        ButtonPtr m_maximizeButton;
        ButtonPtr m_closeButton;
        TextLabelPtr m_title;
    };

    //--

} // ui