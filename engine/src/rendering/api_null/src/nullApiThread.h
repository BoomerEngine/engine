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

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			//---
			
			// Null device thread, ticks at simulated rate of 60FPS
			class Thread : public IBaseThread
			{
			public:
				Thread(Device* drv, WindowManager* windows);
				virtual ~Thread();

				//--

				virtual void syncGPU_Thread()  override final;
				virtual void execute_Thread(uint64_t frameIndex, PerformanceStats& stats, command::CommandBuffer* masterCommandBuffer, const FrameExecutionData& data) override final;

				virtual void insertGpuFrameFence_Thread(uint64_t frameIndex) override final;
				virtual bool checkGpuFrameFence_Thread(uint64_t& outFrameIndex) override final;

				//--

				virtual IBaseSwapchain* createOptimalSwapchain(const OutputInitInfo& info) override final;
				virtual IBaseBuffer* createOptimalBuffer(const BufferCreationInfo& info) override final;
				virtual IBaseImage* createOptimalImage(const ImageCreationInfo& info) override final;
				virtual IBaseSampler* createOptimalSampler(const SamplerState& state) override final;
				virtual IBaseShaders* createOptimalShaders(const ShaderData* data) override final;
				virtual IBaseDownloadArea* createOptimalDownloadArea(uint32_t size) override final;

				virtual IBaseCopyQueue* createOptimalCopyQueue(const base::app::CommandLine& cmdLine) override final;
				virtual IBaseObjectCache* createOptimalObjectCache(const base::app::CommandLine& cmdLine) override final;

				virtual IBaseBackgroundQueue* createOptimalBackgroundQueue(const base::app::CommandLine& cmdLine) override final;

				virtual ObjectRegistry* createOptimalObjectRegistry(const base::app::CommandLine& cmdLine) override final;

				//--

			private:
				struct FakeFence
				{
					uint64_t m_fameIndex = 0;
					base::NativeTimePoint m_scheduled;
				};

				base::Queue<FakeFence> m_fakeFences;
				base::SpinLock m_fakeFencesLock;
			};

			//---

		} // nul
    } // api
} // rendering