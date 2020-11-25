/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\winapi #]
* [# platform: winapi #]
***/

#include "build.h"
#include "apiWindowNull.h"

namespace rendering
{
	namespace api
	{
      
		//--

		WindowNull::WindowNull()
		{
			static std::atomic<uint32_t> st_WindowHandle = 100;
			m_handle = st_WindowHandle++;
		}

		WindowNull::~WindowNull()
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be accessed on main thread");
		}

		bool WindowNull::prepareWindowForRendering(uint16_t& outWidth, uint16_t& outHeight)
		{
			auto lock = CreateLock(m_windowLock);

			if (m_minimized || !m_visible)
				return false;

			outWidth = m_width;
			outHeight = m_height;
			return true;
		}

		void WindowNull::disconnectWindow()
		{
			TRACE_INFO("NullWindow {} disconnected", m_handle);
			m_callback = nullptr;			
		}

		void WindowNull::releaseWindowFromRendering()
		{
		}

		void WindowNull::update()
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be accessed on main thread");
		}

		WindowNull* WindowNull::Create(const OutputInitInfo& creationInfo)
		{
			ASSERT_EX(Fibers::GetInstance().isMainThread(), "Windows can only be created on main thread");

			auto ret = new WindowNull();

			if (creationInfo.m_class == OutputClass::Fullscreen || creationInfo.m_windowMaximized)
			{
				ret->m_width = 2560;
				ret->m_height = 1440;
				ret->m_maximized = true;
			}
			else
			{
				ret->m_width = creationInfo.m_width;
				ret->m_height = creationInfo.m_height;
			}

			ret->m_placement.x = creationInfo.m_windowPlacementX;
			ret->m_placement.y = creationInfo.m_windowPlacementY;
			ret->m_minimized = creationInfo.m_windowMinimized;
			ret->m_callback = creationInfo.m_windowCallback;
			ret->m_visible = creationInfo.m_windowShow;

			/*if (creationInfo.m_windowCreateInputContext)
				ret->m_inputContext = base::input::IContext::CreateNativeContext((uint64_t)hWnd, 0, creationInfo.m_windowInputContextGameMode);*/

			return ret;
		}

		//--

		void WindowNull::windowAbandon()
		{
			m_callback = nullptr;
		}

		void WindowNull::windowMinimize()
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");

			if (!m_minimized)
			{
				m_minimized = true;
				TRACE_INFO("NullWindow {} minimized", m_handle);
			}
		}

		void WindowNull::windowMaximize()
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");

			if (!m_maximized)
			{
				m_minimized = false;
				m_maximized = true;
				m_width = 2560;
				m_height = 1440;

				TRACE_INFO("NullWindow {} maximized to [{}x{}]", m_handle, m_width, m_handle);
			}
		}

		void WindowNull::windowRestore()
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");

			if (m_maximized || m_minimized)
			{
				m_minimized = false;
				m_maximized = false;
				m_width = 1920;
				m_height = 1080;
			}

			TRACE_INFO("NullWindow {} restored to [{}x{}]", m_handle, m_width, m_handle);
		}

		void WindowNull::windowActivate()
		{
			DEBUG_CHECK_RETURN_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");

			if (!m_active)
			{
				m_active = true;
				TRACE_INFO("NullWindow {} activated", m_handle);
			}
		}

		void WindowNull::windowEnable(bool enabled)
		{
			DEBUG_CHECK_RETURN_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");

			if (m_enabled != enabled)
			{
				m_enabled = enabled;
				TRACE_INFO("NullWindow {} enabled {}", m_handle, enabled);
			}
		}

		void WindowNull::windowShow(bool bringToFront)
		{
			DEBUG_CHECK_RETURN_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");

			if (!m_visible)
			{
				TRACE_INFO("NullWindow {} shown", m_handle);
				m_visible = true;

				if (!m_active && bringToFront)
				{
					TRACE_INFO("NullWindow {} activated", m_handle);
					m_active = true;
				}
			}
		}

		void WindowNull::windowHide()
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");

			if (!m_visible)
			{
				TRACE_INFO("NullWindow {} hidden", m_handle);
				m_visible = false;
			}
		}

		uint64_t WindowNull::windowGetNativeHandle() const
		{
			return m_handle;
		}

		uint64_t WindowNull::windowGetNativeDisplay() const
		{
			return 0; // X11 stuff, not used on WinApi
		}

		float WindowNull::windowGetPixelScale() const
		{
			return 1.0f;
		}

		base::Point WindowNull::windowGetClientPlacement() const
		{
			DEBUG_CHECK_RETURN_EX_V(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread", base::Point(0,0));

			if (m_maximized)
				return base::Point(0, 0);
			else
				return m_placement;
		}

		base::Point WindowNull::windowGetClientSize() const
		{
			DEBUG_CHECK_RETURN_EX_V(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread", base::Point(0, 0));

			return base::Point(m_width, m_height);
		}

		base::Point WindowNull::windowGetWindowPlacement() const
		{
			DEBUG_CHECK_RETURN_EX_V(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread", base::Point(0, 0));

			if (m_maximized)
				return base::Point(0, 0);
			else
				return m_placement;
		}

		base::Point WindowNull::windowGetWindowSize() const
		{
			DEBUG_CHECK_RETURN_EX_V(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread", base::Point(0, 0));

			return base::Point(m_width, m_height);
		}

		bool WindowNull::windowHasCloseRequest() const
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
			return m_windowCloseRequest.load();
		}

		bool WindowNull::windowIsActive() const
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
			return m_active;
		}

		bool WindowNull::windowIsVisible() const
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
			return m_visible;
		}

		bool WindowNull::windowIsMaximized() const
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
			return m_maximized;
		}

		bool WindowNull::windowIsMinimized() const
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
			return m_minimized;
		}

		void WindowNull::windowSetTitle(const base::StringBuf& title)
		{
			DEBUG_CHECK_RETURN_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
		}

		void WindowNull::windowAdjustClientPlacement(const base::Rect& clientRect)
		{
			DEBUG_CHECK_RETURN_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");

			if (!m_maximized)
			{
				m_placement = clientRect.min;
				m_width = clientRect.width();
				m_height = clientRect.height();

				TRACE_INFO("NullWindow {} placement adjusted to [{}x{}] at [{}x{}]", m_handle, m_width, m_height, m_placement.x, m_placement.y);
			}
		}

		void WindowNull::windowAdjustWindowPlacement(const base::Rect& windowRect)
		{
			windowAdjustClientPlacement(windowRect);
		}
        
		void WindowNull::windowSetAlpha(float alpha)
		{
			DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");			
		}

		void WindowNull::windowCancelCloseRequest()
		{
			if (1 == m_windowCloseRequest.exchange(0))
			{
				TRACE_INFO("NullWindow {} close request canceled", m_handle);
			}
		}

		base::input::ContextPtr WindowNull::windowGetInputContext()
		{
			return nullptr;
		}

		//--

		WindowManagerNull::WindowManagerNull()
		{}

		WindowManagerNull::~WindowManagerNull()
		{
			m_windows.clearPtr();
		}

		uint64_t WindowManagerNull::offscreenWindow()
		{
			return 1;
		}

		void WindowManagerNull::updateWindows()
		{
			DEBUG_CHECK_RETURN_EX(Fibers::GetInstance().isMainThread(), "Windows can only be updated from main thread");

			for (auto* window : m_windows)
				window->update();
		}

		void WindowManagerNull::bindWindowOwner(uint64_t handle, ObjectID owner)
		{

		}

		uint64_t WindowManagerNull::createWindow(const OutputInitInfo& initInfo)
		{
			DEBUG_CHECK_RETURN_EX_V(Fibers::GetInstance().isMainThread(), "Windows can only be created from main thread", 0);

			auto wnd = WindowNull::Create(initInfo);
			if (wnd)
			{
				auto lock = CreateLock(m_windowsLock);
				m_windows.pushBack(wnd);
				return (uint64_t)wnd->handle();
			}

			return 0;
		}

		void WindowManagerNull::closeWindow(uint64_t handle)
		{
			DEBUG_CHECK_RETURN_EX(Fibers::GetInstance().isMainThread(), "Windows can only be closed from main thread");

			auto lock = CreateLock(m_windowsLock);

			for (auto wnd : m_windows)
			{
				if (wnd->handle() == handle)
				{
					m_windows.remove(wnd);
					delete wnd;
				}
			}
		}

		bool WindowManagerNull::prepareWindowForRendering(uint64_t handle, uint16_t& outWidth, uint16_t& outHeight)
		{
			auto lock = CreateLock(m_windowsLock);

			for (auto wnd : m_windows)
				if (wnd->handle() == handle)
					return wnd->prepareWindowForRendering(outWidth, outHeight);

			return false;
		}

		void WindowManagerNull::disconnectWindow(uint64_t handle)
		{
			auto lock = CreateLock(m_windowsLock);

			for (auto wnd : m_windows)
				if (wnd->handle() == handle)
					return wnd->disconnectWindow();
		}

		void WindowManagerNull::finishWindowRendering(uint64_t handle)
		{
			auto lock = CreateLock(m_windowsLock);

			for (auto wnd : m_windows)
				if (wnd->handle() == handle)
					return wnd->releaseWindowFromRendering();
		}

		INativeWindowInterface* WindowManagerNull::windowInterface(uint64_t handle)
		{
			auto lock = CreateLock(m_windowsLock);

			for (auto wnd : m_windows)
				if (wnd->handle() == handle)
					return wnd; // TODO: this is unsafe!

			return nullptr;
		}

		//--

		void WindowManagerNull::enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const
		{
			outMonitorAreas.emplaceBack(0, 0, 2560, 1440);
		}

		void WindowManagerNull::enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const
		{
			auto& info = outDisplayInfos.emplaceBack();
			info.active = true;
			info.desktopWidth = 2560;
			info.desktopHeight = 1440;
			info.attached = false;
			info.name = "NullDesktop";
			info.primary = true;
		}

		void WindowManagerNull::enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const
		{
			if (displayIndex == 0)
			{
				auto& outInfo = outResolutions.emplaceBack();
				outInfo.width = 2560;
				outInfo.height = 1440;
			}
		}

		void WindowManagerNull::enumVSyncModes(uint32_t displayIndex, base::Array<ResolutionSyncInfo>& outVSyncModes) const
		{
			ResolutionSyncInfo info;
			info.value = 0;
			info.name = "No VSync";
			outVSyncModes.pushBack(info);

			info.value = 1;
			info.name = "VSync";
			outVSyncModes.pushBack(info);
		}

		void WindowManagerNull::enumRefreshRates(uint32_t displayIndex, const ResolutionInfo& info, base::Array<int>& outRefreshRates) const
		{
			outRefreshRates.pushBack(60);
		}

		//--

		WindowManager* WindowManager::CreateNullManager()
		{
			return new WindowManagerNull();
		}

		//--

	} // api
} // rendering
