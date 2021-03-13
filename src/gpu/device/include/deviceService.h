/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: service #]
***/

#pragma once

#include "core/app/include/localService.h"
#include "core/app/include/commandline.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

///---

class DeviceGlobalObjects;

/// service that manages and controls rendering device
class GPU_DEVICE_API DeviceService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(DeviceService, IService);

public:
    DeviceService();
    virtual ~DeviceService();

    //--

    /// get API for created rendering device
    /// NOTE: once created the device is never recreated
    INLINE IDevice* device() const { return m_device; }

	/// device caps
	INLINE const DeviceCaps& caps() const { return m_caps; }

	/// get global object (common textures, samplers and other resources)
	INLINE const DeviceGlobalObjects& globals() const { return *m_globals; }

    //--

    /// flush any scheduled/stalled operations and wait for all rendering to finish
    /// after this call all that was scheduled to render was rendered, all queries should be complete, all data uploads finished, etc
    void sync();

    //--

private:
    virtual bool onInitializeService(const CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

	//--

    IDevice* m_device = nullptr;

	DeviceGlobalObjects* m_globals = nullptr;

	DeviceCaps m_caps;

	//--

    void reloadShaders();

    //--
};

//---

END_BOOMER_NAMESPACE_EX(gpu)

using DeviceService = boomer::gpu::DeviceService;
