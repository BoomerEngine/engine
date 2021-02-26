/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"

#include "nullApiDevice.h"
#include "nullApiThread.h"
#include "nullApiObjectCache.h"
#include "nullApiSwapchain.h"
#include "nullApiExecutor.h"
#include "nullApiShaders.h"
#include "nullApiSampler.h"
#include "nullApiImage.h"
#include "nullApiBuffer.h"
#include "nullApiBackgroundQueue.h"

#include "gpu/api_common/include/apiObjectRegistry.h"
#include "gpu/api_common/include/apiSwapchain.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

//--

ConfigProperty<float> cvFakeGPUWorkTime("NULL", "FakeGPUWorkTime", 10);

//--

Thread::Thread(Device* drv, WindowManager* windows)
	: IBaseThread(drv, windows)
{
}

Thread::~Thread()
{

}

//--

void Thread::syncGPU_Thread()
{
	Sleep(200);
}

void Thread::execute_Thread(uint64_t frameIndex, PerformanceStats& stats, CommandBuffer* masterCommandBuffer, const FrameExecutionData& data)
{
	FrameExecutor executor(this, &stats);
	executor.execute(masterCommandBuffer);
}

void Thread::insertGpuFrameFence_Thread(uint64_t frameIndex)
{
	auto lock = CreateLock(m_fakeFencesLock);

	//auto& fakeFence = m_fakeFences.emplaceBack();

}

bool Thread::checkGpuFrameFence_Thread(uint64_t& outFrameIndex)
{
	return false;
}

IBaseSwapchain* Thread::createOptimalSwapchain(const OutputInitInfo& info)
{
	if (info.m_class == OutputClass::Window)
	{
		auto window = m_windows->createWindow(info);
		DEBUG_CHECK_RETURN_EX_V(window, "Window not created", nullptr);

		IBaseWindowedSwapchain::WindowSetup setup;
		setup.colorFormat = ImageFormat::RGBA8_UNORM;
		setup.depthFormat = ImageFormat::D24S8;
		setup.samples = 1;
		setup.flipped = false;
		setup.deviceHandle = 0;
		setup.windowHandle = window;
		setup.windowManager = m_windows;
		setup.windowInterface = m_windows->windowInterface(window);

		return new Swapchain(info.m_class, setup);
	}

	return nullptr;
}

IBaseBuffer* Thread::createOptimalBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData)
{
	return new Buffer(this, info, sourceData);
}

IBaseImage* Thread::createOptimalImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData)
{
	return new Image(this, info, sourceData);
}

IBaseSampler* Thread::createOptimalSampler(const SamplerState& state)
{
	return new Sampler(this, state);
}

IBaseShaders* Thread::createOptimalShaders(const ShaderData* data)
{
	return new Shaders(this, data);
}

ObjectRegistry* Thread::createOptimalObjectRegistry(const app::CommandLine& cmdLine)
{
	return new ObjectRegistry(this);
}

IBaseObjectCache* Thread::createOptimalObjectCache(const app::CommandLine& cmdLine)
{
	return new ObjectCache(this);
}

IBaseBackgroundQueue* Thread::createOptimalBackgroundQueue(const app::CommandLine& cmdLine)
{
	return new BackgroundQueue();
}

//--
	
END_BOOMER_NAMESPACE_EX(gpu::api::nul)
