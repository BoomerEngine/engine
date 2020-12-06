/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: service #]
***/

#include "build.h"
#include "renderingDeviceService.h"

#include "base/app/include/localServiceContainer.h"
#include "base/containers/include/hashSet.h"

#include "renderingDeviceApi.h"
#include "renderingOutput.h"
#include "renderingDeviceGlobalObjects.h"

namespace rendering
{
    //--

    base::ConfigProperty<bool> cvSyncRenderingDeviceEveryFrame("Rendering", "ForceSync", false);
    base::ConfigProperty<bool> cvResetDeviceNextFrame("Rendering", "ForceResetNextFrame", false);
    base::ConfigProperty<base::StringBuf> cvDeviceName("Rendering", "DeviceName", "GL4");

    //--

    RTTI_BEGIN_TYPE_CLASS(DeviceService);
    RTTI_END_TYPE();

    DeviceService::DeviceService()
    {
    }

    DeviceService::~DeviceService()
    {
		DEBUG_CHECK_EX(m_globals == nullptr, "Global resources not freed properly");
		DEBUG_CHECK_EX(m_device == nullptr, "Device not closed properly");
    }

    base::app::ServiceInitializationResult DeviceService::onInitializeService(const base::app::CommandLine &cmdLine)
    {
        // read the name of the device from config
        auto deviceToInitializeName = cvDeviceName.get();
        TRACE_INFO("Driver name loaded from config: '{}'", deviceToInitializeName);

        // override with command line
        if (cmdLine.hasParam("device"))
        {
            deviceToInitializeName = cmdLine.singleValue("device");
            TRACE_INFO("Using commandline specified rendering device '{}'", deviceToInitializeName);
        }

        // find the device class
        m_device = createAndInitializeDevice(deviceToInitializeName, cmdLine);
        if (!m_device)
        {
            TRACE_WARNING("Unable to start '{}', using NULL device", deviceToInitializeName);

            m_device = createAndInitializeDevice("Null", cmdLine);
            if (!m_device)
            {
                TRACE_ERROR("Null rendering device nto found or disabled. Exiting.");
                return base::app::ServiceInitializationResult::FatalError;
            }
        }

		// create default objects
		m_globals = new DeviceGlobalObjects(m_device);

        // canvas renderer initialized
        return base::app::ServiceInitializationResult::Finished;
    }

    void DeviceService::onShutdownService()
    {
		if (m_globals)
		{
			delete m_globals;
			m_globals = nullptr;
		}

        if (m_device)
        {
            m_device->sync(true);
            m_device->shutdown();

            delete m_device;
            m_device = nullptr;
        }
    }

    void DeviceService::sync()
    {
        PC_SCOPE_LVL1(DeviceSync);
        m_device->sync(true);
    }

    void DeviceService::onSyncUpdate()
    {
        PC_SCOPE_LVL1(DeviceUpdate);

        // sync with the rendering thread and GPU, with optional flush
        m_device->sync(cvSyncRenderingDeviceEveryFrame.get());
    }

    //--
    
    IDevice* DeviceService::createAndInitializeDevice(base::StringView deviceName, const base::app::CommandLine& cmdLine) const
    {
        base::Array<base::SpecificClassType<IDevice>> deviceClasses;
        RTTI::GetInstance().enumClasses(deviceClasses);

        for (const auto& deviceClass : deviceClasses)
        {
            if (auto deviceNameMetadata = deviceClass->findMetadata<DeviceNameMetadata>())
            {
                if (deviceName == deviceNameMetadata->name())
                {
                    if (auto device = deviceClass->createPointer<IDevice>())
                    {
                        if (device->initialize(cmdLine))
                            return device;

                        device->shutdown();
                        delete device;
                    }
                }
            }
        }

        return nullptr;
    }

    //--

	const DeviceGlobalObjects& Globals()
	{
		return base::GetService<DeviceService>()->globals();
	}

	//--

} // rendering

