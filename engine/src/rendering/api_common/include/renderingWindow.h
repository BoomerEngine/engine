/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

namespace rendering
{
    //---

    struct DriverDisplayInfo;
    struct DriverResolutionInfo;
    struct DriverResolutionSyncInfo;
    struct DriverOutputInitInfo;

    // window manager for rendering
    // NOTE: all windows are strictly main thread only
    class RENDERING_API_COMMON_API WindowManager : public base::NoCopy
    {
    public:
        virtual ~WindowManager();

        //--

        // get the "off screen window" handle
        virtual uint64_t offscreenWindow() = 0;

        // update all windows
        virtual void updateWindows() = 0;

        // create output window interface
        virtual uint64_t createWindow(ObjectID owner, const DriverOutputInitInfo& initInfo) = 0;

        // request to close window, by handle
        virtual void closeWindow(uint64_t handle) = 0;

        // prepare window for rendering
        virtual bool prepareWindowForRendering(uint64_t handle, uint16_t& outWidth, uint16_t& outHeight) = 0;

        // notify window a frame was rendered
        virtual void finishWindowRendering(uint64_t handle) = 0;

        // disconnect window from any further rendering
        virtual void disconnectWindow(uint64_t handle) = 0;

        // get window interface
        virtual IDriverNativeWindowInterface* windowInterface(uint64_t handle) = 0;

        //--
        
        // enumerate monitors
        virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const = 0;

        // enumerate display areas
        virtual void enumDisplays(base::Array<DriverDisplayInfo>& outDisplayInfos) const = 0;

        // enumerate resolutions available at given display index
        virtual void enumResolutions(uint32_t displayIndex, base::Array<DriverResolutionInfo>& outResolutions) const = 0;

        // enumerate vsync modes
        virtual void enumVSyncModes(uint32_t displayIndex, base::Array<DriverResolutionSyncInfo>& outVSyncModes) const = 0;

        // enumerate refresh rates
        virtual void enumRefreshRates(uint32_t displayIndex, const DriverResolutionInfo& info, base::Array<int>& outRefreshRates) const = 0;

        //--

        // create platform specific window manager
        // NOTE: some APIs (like OpenGL) need a window for rendering, even off-screen, set apiNeedsWindowForOutput if that's the case
        static WindowManager* Create(bool apiNeedsWindowForOutput);
    };

    //---

} // driver