/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: service #]
***/

#include "build.h"
#include "renderingDeviceService.h"
#include "renderingDeviceApi.h"
#include "renderingOutput.h"
#include "renderingDeviceGlobalObjects.h"
#include "renderingShaderReloadNotifier.h"

#include "core/app/include/localServiceContainer.h"
#include "core/containers/include/hashSet.h"
#include "core/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

ConfigProperty<bool> cvSyncRenderingDeviceEveryFrame("Rendering", "ForceSync", false);
ConfigProperty<bool> cvResetDeviceNextFrame("Rendering", "ForceResetNextFrame", false);
ConfigProperty<StringBuf> cvDeviceName("Rendering", "DeviceName", "GL4");

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

static SpecificClassType<IDevice> FindDeviceClass(StringView name)
{
	Array<SpecificClassType<IDevice>> deviceClasses;
	RTTI::GetInstance().enumClasses(deviceClasses);

	for (const auto& deviceClass : deviceClasses)
		if (auto deviceNameMetadata = deviceClass->findMetadata<DeviceNameMetadata>())
			if (name == deviceNameMetadata->name())
				return deviceClass;

	return nullptr;
}

app::ServiceInitializationResult DeviceService::onInitializeService(const app::CommandLine &cmdLine)
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

	// find device class
	const auto deviceClass = FindDeviceClass(deviceToInitializeName);
	if (!deviceClass)
	{
		TRACE_ERROR("Unknown device class '{}'", deviceToInitializeName);
		return app::ServiceInitializationResult::FatalError;
	}

	// initialize the device
	auto device = deviceClass->createPointer<IDevice>();
	if (!device->initialize(cmdLine, m_caps))
	{
		TRACE_ERROR("Failed to initialize rendering device");
		device->shutdown();
		delete device;

		return app::ServiceInitializationResult::FatalError;
	}

	// print caps
	TRACE_INFO("Rendering device '{}' initialized", device->name());
	TRACE_INFO("Device geometry tier: {}", m_caps.geometry);
	TRACE_INFO("Device transparency tier: {}", m_caps.transparency);
	TRACE_INFO("Device raytracing tier: {}", m_caps.raytracing);
	TRACE_INFO("Device VRAm size: {}", MemSize(m_caps.vramSize));

	// create default objects
	m_device = device;
	m_globals = new DeviceGlobalObjects(m_device);

    // canvas renderer initialized
    return app::ServiceInitializationResult::Finished;
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

    // reload shaders ?
    if (input::CheckInputKeyPressed(input::KeyCode::KEY_F11))
        reloadShaders();
}

void DeviceService::reloadShaders()
{
    m_device->sync(true);
    ShaderReloadNotifier::NotifyAll();
}

//--

const DeviceGlobalObjects& Globals()
{
	return GetService<DeviceService>()->globals();
}

//--

END_BOOMER_NAMESPACE_EX(gpu)

