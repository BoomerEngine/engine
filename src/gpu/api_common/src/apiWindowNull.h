/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\winapi #]
* [# platform: winapi #]
***/

#pragma once

#include "apiWindow.h"
#include "gpu/device/include/output.h"
#include "gpu/device/include/device.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

// NULL Window output - imitates window to some degree, keeps internal state, etc
class WindowNull : public INativeWindowInterface
{
public:
	WindowNull();
	virtual ~WindowNull();

	//--

	uint64_t handle() const { return m_handle; }
	ObjectID owner() const { return m_owner; }

	//--

	bool prepareWindowForRendering(uint16_t& outWidth, uint16_t& outHeight);
	void disconnectWindow();
	void update();

	//--

	virtual void windowMinimize() override;
	virtual void windowMaximize() override;
	virtual void windowRestore() override;
	virtual void windowActivate() override;
	virtual void windowShow(bool bringToFront = true) override;
	virtual void windowHide() override;
	virtual void windowEnable(bool enabled) override;
	virtual uint64_t windowGetNativeHandle() const override;
	virtual uint64_t windowGetNativeDisplay() const override;
	virtual float windowGetPixelScale() const override;
	virtual Point windowGetClientPlacement() const override;
	virtual Point windowGetClientSize() const override;
	virtual Point windowGetWindowPlacement() const override;
	virtual Point windowGetWindowSize() const override;
    virtual bool windowGetWindowDefaultPlacement(Rect& outWindowNormalRect) const override;
	virtual bool windowHasCloseRequest() const override;
	virtual bool windowIsActive() const override;
	virtual bool windowIsVisible() const override;
	virtual bool windowIsMaximized() const override;
	virtual bool windowIsMinimized() const override;
	virtual void windowSetTitle(const StringBuf& title) override;
	virtual void windowAdjustClientPlacement(const Rect& clientRect) override;
	virtual void windowAdjustWindowPlacement(const Rect& windowRect) override;
	virtual void windowSetAlpha(float alpha) override;
	virtual void windowCancelCloseRequest() override;
	virtual ContextPtr windowGetInputContext() override;
	virtual void windowBindOwner(ObjectID id) override;

	//--

	// create the window output
	static WindowNull* Create(const OutputInitInfo& initInfo);

private:
	ObjectID m_owner;
	uint64_t m_handle = 0;

	Point m_placement;

	uint16_t m_width = 1920;
	uint16_t m_height = 1080;
	bool m_minimized = false;
	bool m_maximized = false;

	bool m_active = false;
	bool m_visible = true;
	bool m_enabled = true;
            
	INativeWindowCallback* m_callback;

	std::atomic<uint32_t> m_windowCloseRequest = 0; // WM_CLOSE
	//--

	Mutex m_windowLock;
};

//--

// manager of WinApi driven windows
class GPU_API_COMMON_API WindowManagerNull : public WindowManager
{
public:
	WindowManagerNull();
	virtual ~WindowManagerNull();

	// --

	bool initialize(bool apiNeedsWindowForOutput) { return true; }

	virtual uint64_t offscreenWindow() override final;
	virtual void updateWindows() override final;
	virtual uint64_t createWindow(const OutputInitInfo& initInfo) override final;
	virtual void closeWindow(uint64_t handle) override final;
	virtual bool prepareWindowForRendering(uint64_t handle, uint16_t& outWidth, uint16_t& outHeight) override final;
	virtual void disconnectWindow(uint64_t handle) override final;
	virtual INativeWindowInterface* windowInterface(uint64_t handle) override final;

	//--

	virtual void enumMonitorAreas(Array<Rect>& outMonitorAreas) const override final;
	virtual void enumDisplays(Array<DisplayInfo>& outDisplayInfos) const override final;
	virtual void enumResolutions(uint32_t displayIndex, Array<ResolutionInfo>& outResolutions) const override final;

	//--

private:
	Array<WindowNull*> m_windows;
	Mutex m_windowsLock;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api)
