/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/device/include/device.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//---

/// common code for device implementation
class GPU_API_COMMON_API IBaseDevice : public IDevice
{
	RTTI_DECLARE_VIRTUAL_CLASS(IBaseDevice, IDevice);

public:
	IBaseDevice();
	virtual ~IBaseDevice();

	//--

	// get internal thread
	INLINE IBaseThread* thread() const { return m_thread; }

	// get internal window manager
	INLINE WindowManager* windows() const { return m_windows; }

	//--

	virtual Point maxRenderTargetSize() const override;

	virtual bool initialize(const CommandLine& cmdLine, DeviceCaps& outCaps) override;
	virtual void shutdown() override;
	virtual void sync(bool flush) override;

	virtual DeviceSyncInfo querySyncInfo() const override;
	virtual bool registerCompletionCallback(DeviceCompletionType type, IDeviceCompletionCallback* callback) override;

	virtual OutputObjectPtr createOutput(const OutputInitInfo& info) override;
	virtual BufferObjectPtr createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData) override;
	virtual ShaderObjectPtr createShaders(const ShaderData* shaders) override;
	virtual ImageObjectPtr createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData) override;
	virtual SamplerObjectPtr createSampler(const SamplerState& info) override;
	virtual GraphicsRenderStatesObjectPtr createGraphicsRenderStates(const GraphicsRenderStatesSetup& states) override;

	virtual void submitWork(CommandBuffer* masterCommandBuffer, bool background) override;

	virtual void enumMonitorAreas(Array<Rect>& outMonitorAreas) const override;
	virtual void enumDisplays(Array<DisplayInfo>& outDisplayInfos) const override;
	virtual void enumResolutions(uint32_t displayIndex, Array<ResolutionInfo>& outResolutions) const override;

	//--

	virtual IBaseThread* createOptimalThread(const CommandLine& cmdLine) = 0;
	virtual WindowManager* createOptimalWindowManager(const CommandLine& cmdLine);

	//--

	// create best window manager for the current platform
	WindowManager* createDefaultPlatformWindowManager(const CommandLine& cmdLine);

	//--

private:
	IBaseThread* m_thread = nullptr;
	WindowManager* m_windows = nullptr;
};

		
//---

END_BOOMER_NAMESPACE_EX(gpu::api)
