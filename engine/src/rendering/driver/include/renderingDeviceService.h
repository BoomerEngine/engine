/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: service #]
***/

#pragma once

#include "base/app/include/localService.h"
#include "base/app/include/commandline.h"

namespace rendering
{

    ///---

    /// service that manages and controls rendering device
    class RENDERING_DRIVER_API DeviceService : public base::app::ILocalService
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DeviceService, base::app::ILocalService);

    public:
        DeviceService();
        virtual ~DeviceService();

        //--

        /// get current rendering device
        /// NOTE: this is safe to access only from main thread
        INLINE IDriver* device() const { return m_device; }

        //--

        /// flush any scheduled/stalled operations and wait for all rendering to finish
        /// after this call all that was scheduled to render was rendered, all queries should be complete, all data uploads finished, etc
        void sync();

        //--

    private:
        virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
        virtual void onShutdownService() override final;
        virtual void onSyncUpdate() override final;

        //---

        // the rendering device, always valid (but may be a NULL device)
        IDriver* m_device = nullptr;

        // command line used to create the device
        base::app::CommandLine m_driverCommandLine;

        //--

        IDriver* createAndInitializeDevice(base::StringView name, const base::app::CommandLine& cmdLine) const;
    };

    //---

} // rendering

using DeviceService = rendering::DeviceService;