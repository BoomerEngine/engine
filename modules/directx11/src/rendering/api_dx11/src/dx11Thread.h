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
				virtual void execute_Thread(Frame& frame, PerformanceStats& stats, command::CommandBuffer* masterCommandBuffer, RuntimeDataAllocations& data) override final;

				//--

				virtual IBaseSwapchain* createOptimalSwapchain(const OutputInitInfo& info) override final;
				virtual IBaseFrameFence* createOptimalFrameFence() override final;
				virtual IBaseBuffer* createOptimalBuffer(const BufferCreationInfo& info) override final;
				virtual IBaseImage* createOptimalImage(const ImageCreationInfo& info) override final;
				virtual IBaseSampler* createOptimalSampler(const SamplerState& state) override final;
				virtual IBaseShaders* createOptimalShaders(const ShaderData* data) override final;
				virtual IBaseGraphicsPassLayout* createOptimalPassLayout(const GraphicsPassLayoutSetup& info) override final;

				virtual IBaseStagingPool* createOptimalStagingPool(uint32_t size, uint32_t pageSize, const base::app::CommandLine& cmdLine) override final;
				virtual IBaseCopyQueue* createOptimalCopyQueue(const base::app::CommandLine& cmdLine) override final;
				virtual IBaseObjectCache* createOptimalObjectCache(const base::app::CommandLine& cmdLine) override final;

				virtual IBaseBackgroundQueue* createOptimalBackgroundQueue(const base::app::CommandLine& cmdLine) override final;
				virtual IBaseTransientBufferPool* createOptimalTransientStagingPool(const base::app::CommandLine& cmdLine) override final;
				virtual IBaseTransientBufferPool* createOptimalTransientConstantPool(const base::app::CommandLine& cmdLine) override final;

				virtual ObjectRegistry* createOptimalObjectRegistry(const base::app::CommandLine& cmdLine) override final;

				//--

			private:
				uint32_t m_adapterIndex = 0;

				DXGIHelper* m_dxgi = nullptr; // not owned

				ID3D11Device* m_dxDevice = nullptr;
				ID3D11DeviceContext* m_dxDeviceContext = nullptr;

				virtual bool threadStartup(const base::app::CommandLine& cmdLine) override final;
				virtual void threadFinish() override final;
			};

			//---

		} // dx11
    } // api
} // rendering