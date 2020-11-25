/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\output #]
***/

#include "build.h"
#include "glDevice.h"
#include "glDeviceThread.h"
#include "glObjectRegistry.h"
#include "glOutput.h"
#include "rendering/device/include/renderingImage.h"

namespace rendering
{
    namespace gl4
    {
		//--

		OutputRenderTarget::OutputRenderTarget(Device* drv, Output* output, ImageFormat format, uint8_t samples, bool depth)
			: Object(drv, ObjectType::OutputRenderTargetView)
			, m_output(output)
			, m_format(format)
			, m_samples(samples)
			, m_depth(depth)
		{}

        //--

        Output::Output(Device* drv, DeviceThread* th, WindowManager* windows, OutputClass cls, bool flipped)
            : Object(drv, ObjectType::Output)
            , m_outputClass(cls)
            , m_windows(windows)
            , m_thread(th)
            , m_flipped(flipped)
        {
        }

        Output::~Output()
        {
			// release color surface
			if (m_colorTarget)
			{
				delete m_colorTarget;
				m_colorTarget = nullptr;
			}

			// release depth surface
			if (m_depthTarget)
			{
				delete m_depthTarget;
				m_depthTarget = nullptr;
			}

            // delete DC, if used
            if (m_deviceHandle)
            {
                m_thread->prepareWindowForDeletion_Thread(m_windowHandle, m_deviceHandle);
                m_deviceHandle = 0;
            }

            // delete window, if used
            if (m_windowHandle)
            {
                m_thread->postWindowForDeletion_Thread(m_windowHandle);
                m_windowHandle = 0;
            }
        
            DEBUG_CHECK_EX(m_deviceHandle == 0, "Output still has a device context");
            DEBUG_CHECK_EX(m_windowHandle == 0, "Output still has a window");
        }

        void Output::bind(uint64_t windowHandle, uint64_t deviceHandle, ImageFormat colorFormat, ImageFormat depthFormat, uint8_t numSamples, INativeWindowInterface* window)
        {
            DEBUG_CHECK_RETURN(m_windowHandle == 0);
            DEBUG_CHECK_RETURN(m_deviceHandle == 0);
			DEBUG_CHECK_RETURN(m_colorTarget == nullptr);
			DEBUG_CHECK_RETURN(m_depthTarget == nullptr);

			if (colorFormat != ImageFormat::UNKNOWN)
			{
				const auto formatClass = GetImageFormatInfo(colorFormat).formatClass;
				DEBUG_CHECK_RETURN(formatClass == ImageFormatClass::UNORM || formatClass == ImageFormatClass::SRGB || formatClass == ImageFormatClass::FLOAT);
				m_colorTarget = new OutputRenderTarget(device(), this, colorFormat, numSamples, false);
			}
			
			if (depthFormat != ImageFormat::UNKNOWN)
			{
				const auto formatClass = GetImageFormatInfo(depthFormat).formatClass;
				DEBUG_CHECK_RETURN(formatClass == ImageFormatClass::DEPTH);
				m_depthTarget = new OutputRenderTarget(device(), this, depthFormat, numSamples, true);
			}

            m_windowHandle = windowHandle;
            m_deviceHandle = deviceHandle;
            m_windowInterface = window;
        }
		
        bool Output::prepare(IDeviceObject* owner, RenderTargetViewPtr* outColorRT, RenderTargetViewPtr* outDepthRT, base::Point& outViewport)
        {
            uint16_t width = 0, height = 0;
            if (!m_windows->prepareWindowForRendering(m_windowHandle, width, height))
                return false;

            outViewport.x = width;
            outViewport.y = height;

			if (m_renderTargets.lastWidth != width && m_renderTargets.lastHeight != height)
			{
				TRACE_INFO("GL: Recreating internal render targets [{}x{}] -> [{}x{}]",
					m_renderTargets.lastWidth, m_renderTargets.lastHeight, width, height);

				m_renderTargets.color.reset();
				m_renderTargets.depth.reset();
				m_renderTargets.lastWidth = width;
				m_renderTargets.lastHeight = height;

				if (width && height)
				{
					RenderTargetView::Setup setup;
					setup.firstSlice = 0;
					setup.numSlices = 1;
					setup.width = width;
					setup.height = height;
					setup.mip = 0;
					setup.arrayed = false;
					setup.flipped = true;
					setup.swapchain = true;

					if (m_colorTarget)
					{
						setup.format = m_colorTarget->format();
						setup.samples = m_colorTarget->samples();
						setup.msaa = m_colorTarget->samples() > 1;
						setup.samples = m_colorTarget->samples();
						setup.msaa = m_colorTarget->samples() > 1;
						setup.depth = false;
						m_renderTargets.color = base::RefNew<RenderTargetView>(m_colorTarget->handle(), owner, device()->objectRegistry().proxy(), setup);
					}

					if (m_depthTarget)
					{
						setup.format = m_depthTarget->format();
						setup.samples = m_depthTarget->samples();
						setup.msaa = m_depthTarget->samples() > 1;
						setup.samples = m_depthTarget->samples();
						setup.msaa = m_depthTarget->samples() > 1;
						setup.depth = true;
						m_renderTargets.depth = base::RefNew<RenderTargetView>(m_depthTarget->handle(), owner, device()->objectRegistry().proxy(), setup);
					}
					
				}
			}

			if (outColorRT)
				*outColorRT = m_renderTargets.color;

			if (outDepthRT)
				*outDepthRT = m_renderTargets.depth;

            return true;
        }

        //--

        OutputObjectProxy::OutputObjectProxy(ObjectID id, ObjectRegistryProxy* impl, bool flipped, INativeWindowInterface* window)
            : rendering::IOutputObject(id, impl, flipped, window)
            , m_proxy(impl)
        {}

        bool OutputObjectProxy::prepare(RenderTargetViewPtr* outColorRT, RenderTargetViewPtr* outDepthRT, base::Point& outViewport)
        {
            bool ret = false;

            if (auto proxy = m_proxy.lock())
            {
                proxy->run<Output>(id(), [this, &ret, outColorRT, outDepthRT, &outViewport](Output* ptr)
                    {
                        ret = ptr->prepare(this, outColorRT, outDepthRT, outViewport);
                    });
            }

            return ret;
        }

        //--

    } // gl4
} // rendering
