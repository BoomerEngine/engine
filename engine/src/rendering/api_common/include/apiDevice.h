/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/device/include/renderingDeviceApi.h"

namespace rendering
{
    namespace api
    {
		//---

		/// common code for device implementation
		class RENDERING_API_COMMON_API IBaseDevice : public IDevice
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

			virtual base::Point maxRenderTargetSize() const override;

			virtual bool initialize(const base::app::CommandLine& cmdLine) override;
			virtual void shutdown() override;
			virtual void sync() override;
			virtual void advanceFrame() override;

			virtual OutputObjectPtr createOutput(const OutputInitInfo& info) override;
			virtual BufferObjectPtr createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData, base::fibers::WaitCounter initializationFinished) override;
			virtual ShaderObjectPtr createShaders(const ShaderLibraryData* shaders, PipelineIndex index) override;
			virtual ImageObjectPtr createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData, base::fibers::WaitCounter initializationFinished) override;
			virtual SamplerObjectPtr createSampler(const SamplerState& info) override;
			virtual GraphicsPassLayoutObjectPtr createGraphicsPassLayout(const GraphicsPassLayoutSetup& info) override;
			virtual GraphicsRenderStatesObjectPtr createGraphicsRenderStates(const StaticRenderStatesSetup& states) override;

			virtual bool asyncCopy(const IDeviceObject* object, const ResourceCopyRange& range, const ISourceDataProvider* sourceData, base::fibers::WaitCounter initializationFinished /*= base::fibers::WaitCounter()*/) override;

			virtual void submitWork(command::CommandBuffer* masterCommandBuffer, bool background) override;

			virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const override;
			virtual void enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const override;
			virtual void enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const override;
			virtual void enumVSyncModes(uint32_t displayIndex, base::Array<ResolutionSyncInfo>& outVSyncModes) const override;
			virtual void enumRefreshRates(uint32_t displayIndex, const ResolutionInfo& info, base::Array<int>& outRefreshRates) const override;

			//--

			virtual IBaseThread* createOptimalThread(const base::app::CommandLine& cmdLine) = 0;
			virtual WindowManager* createOptimalWindowManager(const base::app::CommandLine& cmdLine);

			//--

			// create best window manager for the current platform
			WindowManager* createDefaultPlatformWindowManager(const base::app::CommandLine& cmdLine);

			//--

		private:
			IBaseThread* m_thread = nullptr;
			WindowManager* m_windows = nullptr;
		};

		
		//---

    } // api
} // rendering