/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiDevice.h"
#include "apiThread.h"
#include "apiCopyQueue.h"
#include "apiObjectRegistry.h"
#include "apiWindow.h"
#include "apiOutput.h"
#include "apiBuffer.h"
#include "apiShaders.h"
#include "apiSampler.h"
#include "apiImage.h"
#include "apiGraphicsPassLayout.h"
#include "apiGraphicsRenderStates.h"

#include "rendering/device/include/renderingPipeline.h"

#ifdef PLATFORM_WINAPI
	#include "apiWindowWinApi.h"
#else
	#include "apiWindowNull.h"
#endif

namespace rendering
{
    namespace api
    {
		//--

		RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IBaseDevice);
		RTTI_END_TYPE();

		IBaseDevice::IBaseDevice()
		{}

		IBaseDevice::~IBaseDevice()
		{
			DEBUG_CHECK_EX(m_thread == nullptr, "IBaseDevice was shut down incorectly");
			DEBUG_CHECK_EX(m_windows == nullptr, "IBaseDevice was shut down incorectly");
		}

		base::Point IBaseDevice::maxRenderTargetSize() const
		{
			base::InplaceArray<base::Rect, 8> displays;
			m_windows->enumMonitorAreas(displays);

			base::Point ret;
			for (auto& info : displays)
			{
				ret.x = std::max<int>(ret.x, info.width());
				ret.y = std::max<int>(ret.y, info.height());
			}

			ret.x = std::max<int>(ret.x, 1920);
			ret.y = std::max<int>(ret.y, 1080);

			return ret;
		}

		bool IBaseDevice::initialize(const base::app::CommandLine& cmdLine)
		{
			// create window manager
			m_windows = createOptimalWindowManager(cmdLine);
			if (!m_windows)
			{
				TRACE_ERROR("Rendering API failed to create internal window manager");
				return false;
			}

			// create the runtime thread
			auto thread = createOptimalThread(cmdLine);
			if (!thread)
			{
				TRACE_ERROR("Rendering API failed to create internal thread");

				delete m_windows;
				m_windows = nullptr;

				return false;
			}

			// start the runtime thread
			if (!thread->startThread(cmdLine))
			{
				TRACE_ERROR("Rendering API failed to start internal thread");

				thread->stopThread();
				delete thread;

				delete m_windows;
				m_windows = nullptr;

				return false;
			}

			// we are initialized now
			TRACE_INFO("Rendering device initialized");
			return true;
		}

		void IBaseDevice::shutdown()
		{
			// stop everything
			if (m_thread)
			{
				m_thread->stopThread();
				delete m_thread;
				m_thread = nullptr;
			}

			// close the window manager
			delete m_windows;
			m_windows = nullptr;
		}

		void IBaseDevice::advanceFrame()
		{
			PC_SCOPE_LVL1(AdvanceFrame);
			m_windows->updateWindows();
			m_thread->advanceFrame();
		}

		void IBaseDevice::sync()
		{
			PC_SCOPE_LVL1(DriverSync);
			m_thread->sync();
		}

		//--

		WindowManager* IBaseDevice::createOptimalWindowManager(const base::app::CommandLine& cmdLine)
		{
			return createDefaultPlatformWindowManager(cmdLine);
		}

		WindowManager* IBaseDevice::createDefaultPlatformWindowManager(const base::app::CommandLine& cmdLine)
		{
#ifdef PLATFORM_WINAPI
			auto ret = new WindowManagerWinApi();
#else
			auto ret = new WindowManagerNull();
#endif

			if (!ret->initialize(true))
			{
				delete ret;
				return nullptr;
			}

			return ret;
		}

		OutputObjectPtr IBaseDevice::createOutput(const OutputInitInfo& info)
		{
			if (auto* swapchain = m_thread->createOptimalSwapchain(info))
			{
				auto* output = new Output(m_thread, swapchain);
				return base::RefNew<OutputObjectProxy>(output->handle(), m_thread->objectRegistry(), swapchain->flipped(), swapchain->windowInterface());
			}

			return nullptr;
		}

		BufferObjectPtr IBaseDevice::createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData, base::fibers::WaitCounter initializationFinished)
		{
			BufferObject::Setup setup;
			if (!validateBufferCreationSetup(info, setup))
				return nullptr;

			if (auto* obj = m_thread->createOptimalBuffer(info))
			{
				if (sourceData)
				{
					ResourceCopyRange range;
					range.buffer.offset = 0;
					range.buffer.size = info.size;
					m_thread->copyQueue()->schedule(obj, range, sourceData, initializationFinished);
				}

				return base::RefNew<BufferObjectProxy>(obj->handle(), m_thread->objectRegistry(), setup);
			}

			return nullptr;
		}

		ShaderObjectPtr IBaseDevice::createShaders(const ShaderLibraryData* shaders, PipelineIndex index)
		{
			if (auto* obj = m_thread->createOptimalShaders(shaders, index))
				return base::RefNew<ShadersObjectProxy>(obj->handle(), m_thread->objectRegistry(), shaders, index);

			return nullptr;
		}

		ImageObjectPtr IBaseDevice::createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData, base::fibers::WaitCounter initializationFinished)
		{
			ImageObject::Setup setup;
			if (!validateImageCreationSetup(info, setup))
				return nullptr;

			if (auto* obj = m_thread->createOptimalImage(info))
			{
				if (sourceData)
				{
					ResourceCopyRange range;
					range.image.firstMip = 0;
					range.image.firstSlice = 0;
					range.image.numMips = info.numMips;
					range.image.numSlices= info.numSlices;
					m_thread->copyQueue()->schedule(obj, range, sourceData, initializationFinished);
				}

				return base::RefNew<ImageObjectProxy>(obj->handle(), m_thread->objectRegistry(), setup);
			}

			return nullptr;
		}

		SamplerObjectPtr IBaseDevice::createSampler(const SamplerState& info)
		{
			if (auto* obj = m_thread->createOptimalSampler(info))
				return base::RefNew<SamplerObject>(obj->handle(), m_thread->objectRegistry());
			
			return nullptr;
		}

		GraphicsPassLayoutObjectPtr IBaseDevice::createGraphicsPassLayout(const GraphicsPassLayoutSetup& info)
		{
			if (auto* obj = m_thread->createOptimalPassLayout(info))
				return base::RefNew<GraphicsPassLayoutObject>(obj->handle(), m_thread->objectRegistry(), info, obj->key());

			return nullptr;
		}

		GraphicsRenderStatesObjectPtr IBaseDevice::createGraphicsRenderStates(const StaticRenderStatesSetup& states)
		{
			if (auto* obj = m_thread->createOptimalRenderStates(states))
				return base::RefNew<GraphicsRenderStatesObject>(obj->handle(), m_thread->objectRegistry(), states, obj->key());

			return nullptr;
		}

		//--

		void IBaseDevice::enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const
		{
			return m_windows->enumMonitorAreas(outMonitorAreas);
		}

		void IBaseDevice::enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const
		{
			return m_windows->enumDisplays(outDisplayInfos);
		}

		void IBaseDevice::enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const
		{
			m_windows->enumResolutions(displayIndex, outResolutions);
		}

		void IBaseDevice::enumVSyncModes(uint32_t displayIndex, base::Array<ResolutionSyncInfo>& outVSyncModes) const
		{
			m_windows->enumVSyncModes(displayIndex, outVSyncModes);
		}

		void IBaseDevice::enumRefreshRates(uint32_t displayIndex, const ResolutionInfo& info, base::Array<int>& outRefreshRates) const
		{
			m_windows->enumRefreshRates(displayIndex, info, outRefreshRates);
		}

		//--

		bool IBaseDevice::asyncCopy(const IDeviceObject* object, const ResourceCopyRange& range, const ISourceDataProvider* sourceData, base::fibers::WaitCounter initializationFinished /*= base::fibers::WaitCounter()*/)
		{
			DEBUG_CHECK_RETURN_V(object != nullptr, false);
			DEBUG_CHECK_RETURN_V(sourceData != nullptr, false);

			auto* baseObject = m_thread->objectRegistry()->resolveStatic(object->id(), ObjectType::Unknown);
			DEBUG_CHECK_RETURN_V(baseObject != nullptr, false);

			auto* baseCopiable = baseObject->toCopiable();
			DEBUG_CHECK_RETURN_V(baseCopiable != nullptr, false);

			return m_thread->copyQueue()->schedule(baseCopiable, range, sourceData, initializationFinished);
		}

		void IBaseDevice::submitWork(command::CommandBuffer* masterCommandBuffer, bool background)
		{
			(void)background; // no background rendering jobs on OpenGL :(
			m_thread->submit(masterCommandBuffer);
		}

		//--

    } // api
} // rendering
