/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

namespace rendering
{
	struct DisplayInfo;
	struct ResolutionInfo;
	struct ResolutionSyncInfo;
	struct OutputInitInfo;

	namespace api
	{
		//---


		// window manager for rendering
		// NOTE: all windows are strictly main thread only
		class RENDERING_API_COMMON_API WindowManager : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_WINDOW)

		public:
			virtual ~WindowManager();

			//--

			// get the "off screen window" handle
			virtual uint64_t offscreenWindow() = 0;

			// update all windows
			virtual void updateWindows() = 0;

			// create output window interface
			virtual uint64_t createWindow(const OutputInitInfo& initInfo) = 0;

			// request to close window, by handle
			// NOTE: can be called on any thread but window is deleted only on main thread
			virtual void closeWindow(uint64_t handle) = 0;

			// prepare window for rendering
			virtual bool prepareWindowForRendering(uint64_t handle, uint16_t& outWidth, uint16_t& outHeight) = 0;

			// disconnect window from any further rendering
			// NOTE: can be called on any thread, disconnects window's callback to other parts of engine
			virtual void disconnectWindow(uint64_t handle) = 0;

			// get window interface
			virtual INativeWindowInterface* windowInterface(uint64_t handle) = 0;

			//--

			// enumerate monitors
			virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const = 0;

			// enumerate display areas
			virtual void enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const = 0;

			// enumerate resolutions available at given display index
			virtual void enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const = 0;

			//--

			// create platform specific window manager
			// NOTE: some APIs (like OpenGL) need a window for rendering, even off-screen, set apiNeedsWindowForOutput if that's the case
			static WindowManager* Create(bool apiNeedsWindowForOutput);

			// create NULL window manager
			static WindowManager* CreateNullManager();
		};

		//---

	} // api
} // rendering