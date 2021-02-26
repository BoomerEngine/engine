/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "renderingObject.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

// general "window-like" interface
class GPU_DEVICE_API INativeWindowInterface : public NoCopy
{
public:
    virtual ~INativeWindowInterface();

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
    virtual Point windowGetClientPlacement() const = 0;

    /// get the window client area size
    virtual Point windowGetClientSize() const = 0;

    /// get the placement of the window area (with border/title)
    virtual Point windowGetWindowPlacement() const = 0;

    /// get the window area size (with border/title)
    virtual Point windowGetWindowSize() const = 0;

    /// get the normal (not maximized/not minimized) placement of the window (non maximized/minimized)
    virtual bool windowGetWindowDefaultPlacement(Rect& outWindowNormalRect) const = 0;

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
    virtual void windowSetTitle(const StringBuf& title) = 0;

    /// set window transparency alpha
    virtual void windowSetAlpha(float alpha) = 0;

    // set new client rect coordinates for the window
    virtual void windowAdjustClientPlacement(const Rect& clientRect) = 0;

    // set new window rect (will include border) for the window
    virtual void windowAdjustWindowPlacement(const Rect& windowRect) = 0;

    /// cancel the close request on the window
    virtual void windowCancelCloseRequest() = 0;

    /// get input context for the window
    virtual input::ContextPtr windowGetInputContext() = 0;

	/// bind owner to window
	virtual void windowBindOwner(ObjectID id) = 0;
};

    
// callback for the devicedriver output
class GPU_DEVICE_API INativeWindowCallback : public NoCopy
{
public:
    virtual ~INativeWindowCallback();

    /// window was activated or deactivated
    virtual void onOutputWindowStateChanged(ObjectID output, bool active) {};

    /// window size and/or position has changed
    virtual void onOutputWindowPlacementChanged(ObjectID output, const Rect& newSize, float pixelScale, bool duringSizeMove) {};

    /// hit test window area for cursor type
    virtual bool onOutputWindowSelectCursor(ObjectID output, const Point& absolutePosition, input::CursorType& outCursorType) { return false; };

    /// hit test window title area for window system integration (custom title bar and system buttons)
    virtual bool onOutputWindowHitTestNonClientArea(ObjectID output, const Point& absolutePosition, input::AreaType& outAreaType) { return false; };
};
    
//---

struct OutputInitInfo
{
    StringBuf m_context;
    OutputClass m_class = OutputClass::Offscreen;
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
	bool m_windowStartInFullScreen = false;
	bool m_windowAllowFullscreenToggle = false;
	float m_windowOpacity = 1.0f;
    uint64_t m_windowNativeParent = 0;
    StringBuf m_windowTitle;
    INativeWindowCallback* m_windowCallback = nullptr;
};

//---

/// output object
class GPU_DEVICE_API IOutputObject : public IDeviceObject
{
	RTTI_DECLARE_VIRTUAL_CLASS(IOutputObject, IDeviceObject);

public:
    IOutputObject(ObjectID id, IDeviceObjectHandler* impl, bool flipped, INativeWindowInterface* window);

    //--

    /// is this driver using OpenGL UV mode ? vertical flip of textures ?
    INLINE bool verticalFlipRequired() const { return m_flipped; }

    /// get the "window like" interface for given output, allows for tighter UI integration
    /// NOTE: callable and usable from main thread only and valid only for outputs with windows
    INLINE INativeWindowInterface* window() const { return m_window; }

    //--

	/// Query output status and rendering parameters (format/resolution)
    /// NOTE: if this function returns false than output is dead (closed/lost) and has to be recreated
    virtual bool prepare(RenderTargetViewPtr* outColorRT, RenderTargetViewPtr* outDepthRT, Point& outViewport) = 0;

	/// Disconnect external callback
	virtual void disconnect() = 0;

    //--

private:
    INativeWindowInterface* m_window = nullptr;
    bool m_flipped = false;
};

//---

END_BOOMER_NAMESPACE_EX(gpu)

