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
#include "nullApiFrameFence.h"
#include "nullApiCopyQueue.h"
#include "nullApiCopyPool.h"
#include "nullApiObjectCache.h"
#include "nullApiTransientBuffer.h"
#include "nullApiSwapchain.h"
#include "nullApiExecutor.h"
#include "nullApiGraphicsPassLayout.h"
#include "nullApiShaders.h"
#include "nullApiSampler.h"
#include "nullApiImage.h"
#include "nullApiBuffer.h"

#include "rendering/api_common/include/apiObjectRegistry.h"
#include "rendering/api_common/include/apiSwapchain.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

	        //--

		    base::ConfigProperty<float> cvFakeGPUWorkTime("NULL", "FakeGPUWorkTime", 10);

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
				base::Sleep(200);
			}

			void Thread::execute_Thread(Frame& frame, PerformanceStats& stats, command::CommandBuffer* masterCommandBuffer, RuntimeDataAllocations& data)
			{
				FrameExecutor executor(this, &frame, &stats);
				executor.prepare(data);
				executor.execute(masterCommandBuffer);
			}

			IBaseSwapchain* Thread::createOptimalSwapchain(const OutputInitInfo& info)
			{
				if (info.m_class == OutputClass::Window)
				{
					auto window = m_windows->createWindow(info)~;
					DEBUG_CHECK_RETURN_EX(window, "Window not created", nullptr);

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

			IBaseBuffer* Thread::createOptimalBuffer(const BufferCreationInfo& info)
			{
				return new Buffer(this, info);
			}

			IBaseImage* Thread::createOptimalImage(const ImageCreationInfo& info)
			{
				return new Image(this, info);
			}

			IBaseSampler* Thread::createOptimalSampler(const SamplerState& state)
			{
				return new Sampler(this, state);
			}

			IBaseShaders* Thread::createOptimalShaders(const ShaderData* data)
			{
				return new Shaders(this, data);
			}

			IBaseGraphicsPassLayout* Thread::createOptimalPassLayout(const GraphicsPassLayoutSetup& info)
			{
				return new GraphicsPassLayout(this, info);
			}

			IBaseFrameFence* Thread::createOptimalFrameFence()
			{
				const auto timeout = cvFakeGPUWorkTime.get() * 0.001f;
				return new FrameFence(timeout);
			}

			//--

			IBaseStagingPool* Thread::createOptimalStagingPool(uint32_t size, uint32_t pageSize, const base::app::CommandLine& cmdLine)
			{
				return new CopyPool(size, pageSize);
			}

			IBaseCopyQueue* Thread::createOptimalCopyQueue(const base::app::CommandLine& cmdLine)
			{
				auto* pool = static_cast<CopyPool*>(copyPool());
				return new CopyQueue(this, pool, objectRegistry());
			}

			ObjectRegistry* Thread::createOptimalObjectRegistry(const base::app::CommandLine& cmdLine)
			{
				return new ObjectRegistry(this);
			}

			IBaseObjectCache* Thread::createOptimalObjectCache(const base::app::CommandLine& cmdLine)
			{
				return new ObjectCache(this);
			}

			IBaseTransientBufferPool* Thread::createOptimalTransientStagingPool(const base::app::CommandLine& cmdLine)
			{
				return new TransientBufferPool(this, TransientBufferType::Staging);
			}

			IBaseTransientBufferPool* Thread::createOptimalTransientConstantPool(const base::app::CommandLine& cmdLine)
			{
				return new TransientBufferPool(this, TransientBufferType::Constants);
			}

			//--
	
		} // nul
    } // api
} // rendering
