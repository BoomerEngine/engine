/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "renderingImageView.h"

namespace rendering
{
    //--

    // general output class
    enum class DriverOutputClass : uint8_t
    {
        // render offscreen, no window is allocated, content can be streamed to file/network
        Offscreen,

        // render to native window, window can be moved and resized be user
        NativeWindow, 

        // render to fullscreen device, input and rendering may be handled differently
        Fullscreen,

        // render to connected HMD device (VR/AD headset)
        HMD,
    };

    //--

    // general "window-like" interface
    class RENDERING_DRIVER_API IDriverNativeWindowInterface : public base::NoCopy
    {
    public:
        virtual ~IDriverNativeWindowInterface();

        /// minimize window
        virtual void windowMinimize() = 0;

        /// maximize window
        virtual void windowMaximize() = 0;

        /// restore maximized window
        virtual void windowRestore() = 0;

        /// activate window (bring it to front)
        virtual void windowActivate() = 0;

        /// show window
        virtual void windowShow(bool bringToFront = true) = 0;

        /// hide window
        virtual void windowHide() = 0;

        /// enable/disable window
        virtual void windowEnable(bool enabled) = 0;

        /// get pixel scale of the window
        virtual float windowGetPixelScale() const = 0;

        /// get window native handle
        virtual uint64_t windowGetNativeHandle() const = 0;

        /// get native display handle (x11 Display, etc)
        virtual uint64_t windowGetNativeDisplay() const = 0;

        /// get the placement of the window client area
        virtual base::Point windowGetClientPlacement() const = 0;

        /// get the window client area size
        virtual base::Point windowGetClientSize() const = 0;

        /// get the placement of the window area (with border/title)
        virtual base::Point windowGetWindowPlacement() const = 0;

        /// get the window area size (with border/title)
        virtual base::Point windowGetWindowSize() const = 0;

        /// has the physical window close been requested ?
        /// NOTE: window cannot be closed without us agreeing to it
        virtual bool windowHasCloseRequest() const = 0;

        /// is the physical window active (top) ?
        virtual bool windowIsActive() const = 0;

        /// is the physical window visible ? (false if minimized)
        virtual bool windowIsVisible() const = 0;

        /// is the window maximized ?
        virtual bool windowIsMaximized() const = 0;

        /// is the window minimized ?
        virtual bool windowIsMinimized() const = 0;

        /// set window title
        virtual void windowSetTitle(const base::StringBuf& title) = 0;

        /// set window transparency alpha
        virtual void windowSetAlpha(float alpha) = 0;

        // set new client rect coordinates for the window
        virtual void windowAdjustClientPlacement(const base::Rect& clientRect) = 0;

        // set new window rect (will include border) for the window
        virtual void windowAdjustWindowPlacement(const base::Rect& windowRect) = 0;

        /// cancel the close request on the window
        virtual void windowCancelCloseRequest() = 0;

        /// get input context for the window
        virtual base::input::ContextPtr windowGetInputContext() = 0;

        /// abandon window - remove all communication from outside world
        virtual void windowAbandon() = 0;
    };

    
    // callback for the driver output
    class RENDERING_DRIVER_API IDriverNativeWindowCallback : public base::NoCopy
    {
    public:
        virtual ~IDriverNativeWindowCallback();

        /// window was activated or deactivated
        virtual void onOutputWindowStateChanged(ObjectID output, bool active) {};

        /// window size and/or position has changed
        virtual void onOutputWindowPlacementChanged(ObjectID output, const base::Rect& newSize, float pixelScale, bool duringSizeMove) {};

        /// hit test window area for cursor type
        virtual bool onOutputWindowSelectCursor(ObjectID output, const base::Point& absolutePosition, base::input::CursorType& outCursorType) { return false; };

        /// hit test window title area for window system integration (custom title bar and system buttons)
        virtual bool onOutputWindowHitTestNonClientArea(ObjectID output, const base::Point& absolutePosition, base::input::AreaType& outAreaType) { return false; };
    };

    //--

    // information about output format
    struct DriverOutputFrameInfo 
    {
        uint16_t width = 0; // whole render target size
        uint16_t height = 0;  // whole render target size
        uint8_t samples = 1; // only if MSAA is enabled on the output itself
        uint16_t maxWidth = 0; // maximum render target width allowed for any output, can be used to preallocate render targets for worst case scenario
        uint16_t maxHeight = 0; // maximum render target height allowed for any output, can be used to preallocate render targets for worst case scenario
        ImageFormat colorFormat = ImageFormat::UNKNOWN; // always valid
        ImageFormat depthFormat = ImageFormat::UNKNOWN; // may be invalid
        base::Rect viewport; // actual usable viewport
        bool flippedY = false; // the Y is flipped (OpenGL window)
    };

    //---

    struct DriverOutputInitInfo
    {
        base::StringBuf m_context;
        DriverOutputClass m_class = DriverOutputClass::Offscreen;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_refreshRate = 0;

        int m_windowPlacementX = 0;
        int m_windowPlacementY = 0;
        bool m_windowMaximized = false;
        bool m_windowMinimized = false;
        bool m_windowSystemBorder = true;
        bool m_windowPopup = false;
        bool m_windowAdjustArea = false;
        bool m_windowShowOnTaskBar = true;
        bool m_windowCanResize = true;
        bool m_windowTopmost = false;
        bool m_windowShow = true;
        bool m_windowActivate = true;
        bool m_windowCreateInputContext = true;
        bool m_windowInputContextGameMode = true; // prefer full capture over cooperative capture
        float m_windowOpacity = 1.0f;
        uint64_t m_windowNativeParent = 0;
        base::StringBuf m_windowTitle;
        IDriverNativeWindowCallback* m_windowCallback = nullptr;
    };

    //---

} // rendering

