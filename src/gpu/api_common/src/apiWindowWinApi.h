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

#include <Windows.h>

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

// WinAPI specific Window output (aka. swapchain)
class WindowWinApi : public INativeWindowInterface
{
public:
	WindowWinApi();
	virtual ~WindowWinApi();

	//--

	HWND handle() const { return m_hWnd; }
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
	virtual input::ContextPtr windowGetInputContext() override;
	virtual void windowBindOwner(ObjectID id) override;

	//--

	// create the window output
	static WindowWinApi* Create(const OutputInitInfo& initInfo);

private:
	static void RegisterWindowClass();

	ObjectID m_owner;

	HWND m_hWnd = NULL;

	float m_currentPixelScale = 1.0f;
	bool m_currentActiveFlag = false;
	bool m_currentActiveFlagSeen = false;
	bool m_hasSystemBorder = false;
	bool m_duringSizeMove = false;

	bool m_initialShowDone = false;
	bool m_initialShowMaximize = false;
	bool m_initialShowMinimize = false;
	bool m_initialShowActivate = false;
            
	StringBuf m_currentTitle;
	StringBuf m_fullTitleString;
	void updateTitle_NoLock();

	INativeWindowCallback* m_callback;

	std::atomic<uint32_t> m_windowCloseRequest = 0; // WM_CLOSE

	input::ContextPtr m_inputContext;

	std::atomic<uint32_t> m_numFramesStarted = 0;
	uint32_t m_numLastFramesRendered = 0;
	NativeTimePoint m_nextFPSCapture;

	Rect m_lastSizeMoveRect;

	//--

	Mutex m_windowLock;

	//--

	LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

//--

// manager of WinApi driven windows
class GPU_API_COMMON_API WindowManagerWinApi : public WindowManager
{
public:
	WindowManagerWinApi();
	virtual ~WindowManagerWinApi();

	//---

	bool initialize(bool apiNeedsWindowForOutput);

	// --

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
	Array<Rect> m_monitorRects;
	HWND m_hFakeWnd = NULL;

	struct CachedDisplay
	{
		DisplayInfo m_displayInfo;
		Array<ResolutionInfo> m_resolutions;
	};

	Array<CachedDisplay> m_cachedDisplayInfos;

	Array<WindowWinApi*> m_windows;
	Mutex m_windowsLock;

	SpinLock m_windowsToCloseOnMainThreadLock;
	Array<uint64_t> m_windowsToCloseOnMainThread;

	bool m_duringUpdate = false;

	void cacheDisplayModes();
	void enumerateMonitors();

	static LRESULT CALLBACK DummyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api)
