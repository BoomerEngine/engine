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
			virtual void sync(bool flush) override;

			virtual DeviceSyncInfo querySyncInfo() const override;
			virtual bool registerCompletionCallback(DeviceCompletionType type, IDeviceCompletionCallback* callback) override;

			virtual OutputObjectPtr createOutput(const OutputInitInfo& info) override;
			virtual BufferObjectPtr createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData) override;
			virtual ShaderObjectPtr createShaders(const ShaderData* shaders) override;
			virtual ImageObjectPtr createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData) override;
			virtual SamplerObjectPtr createSampler(const SamplerState& info) override;
			virtual DownloadAreaObjectPtr createDownloadArea(uint32_t size) override;
			virtual GraphicsPassLayoutObjectPtr createGraphicsPassLayout(const GraphicsPassLayoutSetup& info) override;
			virtual GraphicsRenderStatesObjectPtr createGraphicsRenderStates(const GraphicsRenderStatesSetup& states) override;

			virtual void submitWork(command::CommandBuffer* masterCommandBuffer, bool background) override;

			virtual void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const override;
			virtual void enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const override;
			virtual void enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const override;

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