/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiSwapchain.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{
			///---

			class Swapchain : public IBaseWindowedSwapchain
			{
			public:
				Swapchain(OutputClass cls, const WindowSetup& setup, IDXGISwapChain* dxSwapchain, ID3D11Device* dxDevice, ID3D11Texture2D* dxColorBackBuffer, ID3D11RenderTargetView* dxColorBackBufferRTV);
				virtual ~Swapchain(); // can be destroyed only on render thread

				virtual bool acquire() override final;
				virtual void present(bool swap = true) override final;

				virtual bool prepare_ClientApi(SwapchainState& outState) override;

			private:
				ID3D11Device* m_dxDevice = nullptr;
				IDXGISwapChain* m_dxSwapchain = nullptr;

				ID3D11Texture2D* m_dxColorBackbuffer = nullptr;
				ID3D11RenderTargetView* m_dxColorBackbufferRTV = nullptr;

				ID3D11Texture2D* m_dxDepthBackbuffer = nullptr;
				ID3D11RenderTargetView* m_dxDepthBackbufferRTV = nullptr;

				uint32_t m_depthBufferWidth = 0;
				uint32_t m_depthBufferHeight = 0;

				//--

				uint32_t m_swapchainBackbufferWidth_ClientApi = 0;
				uint32_t m_swapchainBackbufferHeight_ClientApi = 0;
			};

			//--

		} // dx11
    } // api
} // rendering

