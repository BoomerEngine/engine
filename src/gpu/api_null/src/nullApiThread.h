/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/api_common/include/apiWindow.h"
#include "gpu/api_common/include/apiThread.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

//---
			
// Null device thread, ticks at simulated rate of 60FPS
class Thread : public IBaseThread
{
public:
	Thread(Device* drv, WindowManager* windows);
	virtual ~Thread();

	//--

	virtual void syncGPU_Thread()  override final;
	virtual void execute_Thread(uint64_t frameIndex, PerformanceStats& stats, CommandBuffer* masterCommandBuffer, const FrameExecutionData& data) override final;

	virtual void insertGpuFrameFence_Thread(uint64_t frameIndex) override final;
	virtual bool checkGpuFrameFence_Thread(uint64_t& outFrameIndex) override final;

	//--

	virtual IBaseSwapchain* createOptimalSwapchain(const OutputInitInfo& info) override final;
	virtual IBaseBuffer* createOptimalBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData) override final;
	virtual IBaseImage* createOptimalImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData) override final;
	virtual IBaseSampler* createOptimalSampler(const SamplerState& state) override final;
	virtual IBaseShaders* createOptimalShaders(const ShaderData* data) override final;

	virtual IBaseObjectCache* createOptimalObjectCache(const app::CommandLine& cmdLine) override final;

	virtual IBaseBackgroundQueue* createOptimalBackgroundQueue(const app::CommandLine& cmdLine) override final;

	virtual ObjectRegistry* createOptimalObjectRegistry(const app::CommandLine& cmdLine) override final;

	//--

private:
	struct FakeFence
	{
		uint64_t m_fameIndex = 0;
		NativeTimePoint m_scheduled;
	};

	Queue<FakeFence> m_fakeFences;
	SpinLock m_fakeFencesLock;
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
