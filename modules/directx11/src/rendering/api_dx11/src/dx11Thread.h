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
		namespace dx11
		{

			//---
			
			// Null device thread, ticks at simulated rate of 60FPS
			class Thread : public IBaseThread
			{
			public:
				Thread(Device* drv, WindowManager* windows, DXGIHelper* dxgi);
				virtual ~Thread();

				//--

				INLINE ID3D11Device* device() const { return m_dxDevice; }
				INLINE ID3D11DeviceContext* deviceContext() const { return m_dxDeviceContext; }

				//--

				virtual void syncGPU_Thread()  override final;
				virtual void execute_Thread(uint64_t frameIndex, PerformanceStats& stats, command::CommandBuffer* masterCommandBuffer, const FrameExecutionData& data) override final;

				//--

				virtual IBaseSwapchain* createOptimalSwapchain(const OutputInitInfo& info) override final;
				virtual IBaseBuffer* createOptimalBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData) override final;
				virtual IBaseImage* createOptimalImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData) override final;
				virtual IBaseSampler* createOptimalSampler(const SamplerState& state) override final;
				virtual IBaseShaders* createOptimalShaders(const ShaderData* data) override final;

				virtual IBaseObjectCache* createOptimalObjectCache(const base::app::CommandLine& cmdLine) override final;

				virtual IBaseBackgroundQueue* createOptimalBackgroundQueue(const base::app::CommandLine& cmdLine) override final;

				virtual ObjectRegistry* createOptimalObjectRegistry(const base::app::CommandLine& cmdLine) override final;

				//--

			private:
				uint32_t m_adapterIndex = 0;

				DXGIHelper* m_dxgi = nullptr; // not owned

				ID3D11Device* m_dxDevice = nullptr;
				ID3D11DeviceContext* m_dxDeviceContext = nullptr;

				//--

				struct FrameFence
				{
					uint64_t frameIndex = 0;
					ID3D11Query* dxQuery = nullptr;
				};

				base::SpinLock m_fencesLock;
				uint64_t m_fencesLastFrame = 0;
				base::Queue<FrameFence> m_fences;

				//--

				virtual bool threadStartup(const base::app::CommandLine& cmdLine, DeviceCaps& outCaps) override final;
				virtual void threadFinish() override final;

				virtual void insertGpuFrameFence_Thread(uint64_t frameIndex) override final;
				virtual bool checkGpuFrameFence_Thread(uint64_t& outFrameIndex) override final;

			};

			//---

		} // dx11
    } // api
} // rendering