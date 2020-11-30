/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiSwapchain.h"
#include "apiWindow.h"

#include "rendering/device/include/renderingGraphicsPassLayout.h"

namespace rendering
{
    namespace api
    {
		//--

		bool SwapchainState::operator==(const SwapchainState& other) const
		{
			return (width == other.width) && (height == other.height) && (samples == other.samples) && (colorFormat == other.colorFormat) && (depthFormat == other.depthFormat);
		}

		void SwapchainState::print(base::IFormatStream& f) const
		{
			f.appendf("[{}x{}] {} {}", width, height, colorFormat, depthFormat);

			if (samples)
				f.appendf(" MSAA x{}", samples);
			else
				f.appendf(" non-MSAA");		
		}

		//--

		IBaseSwapchain::IBaseSwapchain(OutputClass cls, bool flipped, INativeWindowInterface* window)
			: m_window(window)
			, m_outputClass(cls)
			, m_flipped(flipped)
		{}

		IBaseSwapchain::~IBaseSwapchain()
		{}

        //--

		IBaseWindowedSwapchain::IBaseWindowedSwapchain(OutputClass cls, const WindowSetup& setup)
			: IBaseSwapchain(cls, setup.flipped, setup.windowInterface)
			, m_windowManager(setup.windowManager)
			, m_windowHandle(setup.windowHandle)
			, m_deviceHandle(setup.deviceHandle)
			, m_colorFormat(setup.colorFormat)
			, m_depthFormat(setup.depthFormat)
			, m_samples(setup.samples)
		{}

		IBaseWindowedSwapchain::~IBaseWindowedSwapchain()
		{
			if (m_windowManager)
			{
				m_windowManager->closeWindow(m_windowHandle);
				m_windowManager = nullptr;
				m_windowHandle = 0;
			}
		}

		void IBaseWindowedSwapchain::disconnect_ClientApi()
		{
			ASSERT(m_disconnected.exchange(1) == 0);

			if (m_windowManager)
				m_windowManager->disconnectWindow(m_windowHandle);
		}

		bool IBaseWindowedSwapchain::prepare_ClientApi(SwapchainState& outState)
		{
			DEBUG_CHECK_RETURN_V(m_disconnected == 0, false);

			if (!m_windowManager)
				return false;

			uint16_t windowWidth = 0, windowHeight = 0;
			if (!m_windowManager->prepareWindowForRendering(m_windowHandle, windowWidth, windowHeight))
				return false;

			if (!windowWidth || !windowHeight)
				return false;

			outState.colorFormat = m_colorFormat;
			outState.depthFormat = m_depthFormat;
			outState.width = windowWidth;
			outState.height = windowHeight;
			outState.samples = m_samples;
			return true;
		}

		void IBaseWindowedSwapchain::queryLayout(GraphicsPassLayoutSetup& outLayout) const
		{
			outLayout.samples = m_samples;

			if (m_colorFormat != ImageFormat::UNKNOWN)
			{
				outLayout.color[0].format = m_colorFormat;
				outLayout.color[0].loadOp = LoadOp::DontCare;
				outLayout.color[0].storeOp = StoreOp::DontCare;
			}

			if (m_depthFormat != ImageFormat::UNKNOWN)
			{
				outLayout.depth.format = m_depthFormat;
				outLayout.depth.loadOp = LoadOp::DontCare;
				outLayout.depth.storeOp = StoreOp::DontCare;
			}
		}

		//--

    } // api
} // rendering
