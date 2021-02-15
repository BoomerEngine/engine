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
#include "apiObjectRegistry.h"
#include "apiWindow.h"
#include "apiOutput.h"
#include "apiBuffer.h"
#include "apiShaders.h"
#include "apiSampler.h"
#include "apiImage.h"
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

		bool IBaseDevice::initialize(const base::app::CommandLine& cmdLine, DeviceCaps& outCaps)
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
			if (!thread->startThread(cmdLine, outCaps))
			{
				TRACE_ERROR("Rendering API failed to start internal thread");

				thread->stopThread();
				delete thread;

				delete m_windows;
				m_windows = nullptr;

				return false;
			}

			// store thread
			m_thread = thread;

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

		DeviceSyncInfo IBaseDevice::querySyncInfo() const
		{
			return m_thread->syncInfo();
		}

		bool IBaseDevice::registerCompletionCallback(DeviceCompletionType type, IDeviceCompletionCallback* callback)
		{
			return m_thread->registerCompletionCallback(type, callback);
		}

		void IBaseDevice::sync(bool flush)
		{
			PC_SCOPE_LVL1(DriverSync);
			if (!flush)
				m_windows->updateWindows();
			m_thread->sync(flush);
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
				swapchain->windowInterface()->windowBindOwner(output->handle());
				return base::RefNew<OutputObjectProxy>(output->handle(), m_thread->objectRegistry(), swapchain->flipped(), swapchain->windowInterface());
			}

			return nullptr;
		}

		BufferObjectPtr IBaseDevice::createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData)
		{
			BufferObject::Setup setup;
			if (!validateBufferCreationSetup(info, setup))
				return nullptr;

			if (auto* obj = m_thread->createOptimalBuffer(info, sourceData))
			{
				return base::RefNew<BufferObjectProxy>(obj->handle(), m_thread->objectRegistry(), setup);
			}

			return nullptr;
		}

		ShaderObjectPtr IBaseDevice::createShaders(const ShaderData* shader)
		{
			if (auto* obj = m_thread->createOptimalShaders(shader))
				return base::RefNew<ShadersObjectProxy>(obj->handle(), m_thread->objectRegistry(), shader->metadata());

			return nullptr;
		}

		ImageObjectPtr IBaseDevice::createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData)
		{
			ImageObject::Setup setup;
			if (!validateImageCreationSetup(info, setup))
				return nullptr;

			if (auto* obj = m_thread->createOptimalImage(info, sourceData))
			{
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

		GraphicsRenderStatesObjectPtr IBaseDevice::createGraphicsRenderStates(const GraphicsRenderStatesSetup& states)
		{
			// TODO: share "cache" ?

			auto* obj = new IBaseGraphicsRenderStates(m_thread, states);
			return base::RefNew<GraphicsRenderStatesObject>(obj->handle(), m_thread->objectRegistry(), states, obj->key());
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

		//--

		void IBaseDevice::submitWork(command::CommandBuffer* masterCommandBuffer, bool background)
		{
			(void)background; // no background rendering jobs on OpenGL :(
			m_thread->submit(masterCommandBuffer);
		}

		//--

    } // api
} // rendering
