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

	class DeviceGlobalObjects;

    /// service that manages and controls rendering device
    class RENDERING_DEVICE_API DeviceService : public base::app::ILocalService
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DeviceService, base::app::ILocalService);

    public:
        DeviceService();
        virtual ~DeviceService();

        //--

        /// get API for created rendering device
        /// NOTE: once created the device is never recreated
        INLINE IDevice* device() const { return m_device; }

		/// get global object (common textures, samplers and other resources)
		INLINE const DeviceGlobalObjects& globals() const { return *m_globals; }

        //--

        /// flush any scheduled/stalled operations and wait for all rendering to finish
        /// after this call all that was scheduled to render was rendered, all queries should be complete, all data uploads finished, etc
        void sync();

        //--

    private:
        virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
        virtual void onShutdownService() override final;
        virtual void onSyncUpdate() override final;

		//--

        IDevice* m_device = nullptr;

		DeviceGlobalObjects* m_globals = nullptr;

		//--

        IDevice* createAndInitializeDevice(base::StringView name, const base::app::CommandLine& cmdLine) const;
    };

    //---

} // rendering

using DeviceService = rendering::DeviceService;
