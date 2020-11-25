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
#include "rendering/device/include/renderingOutput.h"
#include "rendering/device/include/renderingDeviceApi.h"

#include <Windows.h>

namespace rendering
{
	namespace api
	{

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

			void bindOwner(ObjectID id);
			bool prepareWindowForRendering(uint16_t& outWidth, uint16_t& outHeight);
			void releaseWindowFromRendering();
			void disconnectWindow();
			void update();

			//--

			virtual void windowAbandon() override;
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
			virtual base::Point windowGetClientPlacement() const override;
			virtual base::Point windowGetClientSize() const override;
			virtual base::Point windowGetWindowPlacement() const override;
			virtual base::Point windowGetWindowSize() const override;
			virtual bool windowHasCloseRequest() const override;
			virtual bool windowIsActive() const override;
			virtual bool windowIsVisible() const override;
			virtual bool windowIsMaximized() const override;
			virtual bool windowIsMinimized() const override;
			virtual void windowSetTitle(const base::StringBuf& title) override;
			virtual void windowAdjustClientPlacement(const base::Rect& clientRect) override;
			virtual void windowAdjustWindowPlacement(const base::Rect& windowRect) override;
			virtual void windowSetAlpha(float alpha) override;
			virtual void windowCancelCloseRequest() override;
			virtual base::input::ContextPtr windowGetInputContext() override;

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
            
			base::StringBuf m_currentTitle;
			base::StringBuf m_fullTitleString;
			void updateTitle_NoLock();

			INativeWindowCallback* m_callback;

			std::atomic<uint32_t> m_windowCloseRequest = 0; // WM_CLOSE

			base::input::ContextPtr m_inputContext;

			std::atomic<uint32_t> m_numFramesStarted = 0;
			std::atomic<uint32_t> m_numFramesRendered = 0;
			uint32_t m_numLastFramesRendered = 0;
			base::NativeTimePoint m_nextFPSCapture;

			//--

			base::Mutex m_windowLock;

			//--

			LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
			static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		};

		//--

		// manager of WinApi driven windows
		class RENDERING_API_COMMON_API WindowManagerWinApi : public WindowManager
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
			virtual void bindWindowOwner(uint64_t handle, ObjectID owner) override final;
			virtual void closeWindow(uint64_t handle) override final;
			virtual bool prepareWindowForRendering(uint64_t handle, uint16_t& outWidth, uint16_t& outHeight) override final;
			virtual void finishWindowRendering(uint64_t handle) override final;
			virtual void disconnectWindow(uint64_t handle) override final;
			virtual INativeWindowInterface* windowInterface(uint64_t handle) override final;

			//--

			virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const override final;
			virtual void enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const override final;
			virtual void enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const override final;
			virtual void enumVSyncModes(uint32_t displayIndex, base::Array<ResolutionSyncInfo>& outVSyncModes) const override final;
			virtual void enumRefreshRates(uint32_t displayIndex, const ResolutionInfo& info, base::Array<int>& outRefreshRates) const override final;

			//--

		private:
			base::Array<base::Rect> m_monitorRects;
			HWND m_hFakeWnd = NULL;

			struct CachedDisplayMode
			{
				uint16_t m_width;
				uint16_t m_height;
				uint16_t m_refreshRate;
			};

			struct CachedDisplay
			{
				DisplayInfo m_displayInfo;
				base::Array<CachedDisplayMode> m_displayModes;
			};

			base::Array<CachedDisplay> m_cachedDisplayInfos;

			base::Array<WindowWinApi*> m_windows;
			base::Mutex m_windowsLock;

			base::Array<uint64_t> m_windowsToCloseOnMainThread;

			bool m_duringUpdate = false;

			void cacheDisplayModes();
			void enumerateMonitors();

			static LRESULT CALLBACK DummyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		};

		//--

	} // api
} // rendering