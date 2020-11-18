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

namespace rendering
{
    namespace gl4
    {

        //--

        Output::Output(Device* drv, DeviceThread* th, WindowManager* windows, DriverOutputClass cls, bool flipped)
            : Object(drv, ObjectType::Output)
            , m_outputClass(cls)
            , m_windows(windows)
            , m_thread(th)
            , m_flipped(flipped)
        {
            m_colorID = ObjectID::AllocTransientID();
            m_depthID = ObjectID::AllocTransientID();
        }

        Output::~Output()
        {
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

        void Output::bind(uint64_t windowHandle, uint64_t deviceHandle, ImageFormat colorFormat, ImageFormat depthFormat, INativeWindowInterface* window)
        {
            DEBUG_CHECK_RETURN(m_windowHandle == 0);
            DEBUG_CHECK_RETURN(m_deviceHandle == 0);
            DEBUG_CHECK_RETURN(m_colorFormat == ImageFormat::UNKNOWN);
            DEBUG_CHECK_RETURN(m_depthFormat == ImageFormat::UNKNOWN);

            m_windowHandle = windowHandle;
            m_deviceHandle = deviceHandle;
            m_colorFormat = colorFormat;
            m_depthFormat = depthFormat;
            m_windowInterface = window;
        }

        bool Output::prepare(ImageView* outColorRT, ImageView* outDepthRT, base::Point& outViewport) const
        {
            uint16_t width = 0, height = 0;
            if (!m_windows->prepareWindowForRendering(m_windowHandle, width, height))
                return false;

            outViewport.x = width;
            outViewport.y = height;

            const auto flipFlag = flipped() ? ImageViewFlag::FlippedY : (ImageViewFlag)0;

            if (outColorRT)
            {
                if (m_colorFormat != ImageFormat::UNKNOWN)
                {
                    ImageViewFlags colorFlags = { ImageViewFlag::RenderTarget, ImageViewFlag::SwapChain, flipFlag };
                    *outColorRT = ImageView(m_colorID, ImageViewKey(m_colorFormat, ImageViewType::View2D, 0, 1, 0, 1), 1, width, height, 1, colorFlags, ObjectID::DefaultPointSampler());
                }
                else
                {
                    *outColorRT = ImageView();
                }
            }

            if (outDepthRT)
            {
                if (m_depthFormat != ImageFormat::UNKNOWN)
                {
                    ImageViewFlags depthFlags = { ImageViewFlag::RenderTarget, ImageViewFlag::SwapChain, ImageViewFlag::Depth, flipFlag };
                    *outDepthRT = ImageView(m_depthID, ImageViewKey(m_depthFormat, ImageViewType::View2D, 0, 1, 0, 1), 1, width, height, 1, depthFlags, ObjectID::DefaultPointSampler());
                }
                else
                {
                    *outDepthRT = ImageView();
                }
            }

            return true;
        }

        //--

        OutputObjectProxy::OutputObjectProxy(ObjectID id, ObjectRegistryProxy* impl, bool flipped, INativeWindowInterface* window)
            : rendering::IOutputObject(id, impl, flipped, window)
            , m_proxy(impl)
        {}

        bool OutputObjectProxy::prepare(ImageView* outColorRT, ImageView* outDepthRT, base::Point& outViewport)
        {
            bool ret = false;

            if (auto proxy = m_proxy.lock())
            {
                proxy->run<Output>(id(), [this, &ret, outColorRT, outDepthRT, &outViewport](Output* ptr)
                    {
                        ret = ptr->prepare(outColorRT, outDepthRT, outViewport);
                    });
            }

            return ret;
        }

        //--

    } // gl4
} // device
