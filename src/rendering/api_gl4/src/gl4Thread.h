/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiWindow.h"
#include "rendering/api_common/include/apiThread.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::gl4)

//---
			
// Null device thread, ticks at simulated rate of 60FPS
class Thread : public IBaseThread
{
public:
	Thread(Device* drv, WindowManager* windows);
	virtual ~Thread();

	//--

	INLINE ObjectCache* objectCache() const { return (ObjectCache*) IBaseThread::objectCache(); }

	INLINE UniformBufferPool* uniformPool() const { return m_uniformPool; }

	//--

	virtual void syncGPU_Thread()  override final;
	virtual void execute_Thread(uint64_t frameIndex, PerformanceStats& stats, GPUCommandBuffer* masterCommandBuffer, const FrameExecutionData& data) override final;

	virtual void insertGpuFrameFence_Thread(uint64_t frameIndex) override final;
	virtual bool checkGpuFrameFence_Thread(uint64_t& outCompletedFrameIndex) override final;

	virtual bool threadStartup(const base::app::CommandLine& cmdLine, DeviceCaps& outCaps) override; // called on thread to initialize API
	virtual void threadFinish() override; // called on thread to initialize API

	//--

	virtual IBaseBuffer* createOptimalBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData) override final;
	virtual IBaseImage* createOptimalImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData) override final;
	virtual IBaseSampler* createOptimalSampler(const SamplerState& state) override final;
	virtual IBaseShaders* createOptimalShaders(const ShaderData* data) override final;

	virtual IBaseObjectCache* createOptimalObjectCache(const base::app::CommandLine& cmdLine) override final;


	virtual ObjectRegistry* createOptimalObjectRegistry(const base::app::CommandLine& cmdLine) override final;

	//--

private:
	struct FrameFence
	{
		uint64_t frameIndex = 0;
		GLsync glFence = 0;
	};

	base::SpinLock m_fencesLock;
	uint64_t m_fencesLastFrame = 0;
	base::Queue<FrameFence> m_fences;

	//--

	UniformBufferPool* m_uniformPool = nullptr;

	//--
};

//---

END_BOOMER_NAMESPACE(rendering::api::gl4)