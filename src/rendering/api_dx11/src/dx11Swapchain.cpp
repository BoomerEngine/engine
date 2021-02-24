/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "dx11Thread.h"
#include "dx11Swapchain.h"

BEGIN_BOOMER_NAMESPACE(rendering::api)
					IDXGIOutput* dxTarget = nullptr;
					BOOL bFullscreen = FALSE;
					if (SUCCEEDED(dxSwapchain->GetFullscreenState(&bFullscreen, &dxTarget)))
					{
						dxTarget->Release();
					}
					else
					{
						bFullscreen = FALSE;
					}

					if (bFullscreen)
						dxSwapchain->SetFullscreenState(FALSE, NULL);
				}
			}

			static void EnterFullscreen(IDXGISwapChain* dxSwapchain, HWND hWnd)
			{
				if (dxSwapchain)
				{
					IDXGIOutput* dxTarget = nullptr;
					BOOL bFullscreen = FALSE;
					if (SUCCEEDED(dxSwapchain->GetFullscreenState(&bFullscreen, &dxTarget)))
					{
						dxTarget->Release();
					}
					else
					{
						bFullscreen = FALSE;
					}

					if (!bFullscreen)
					{
						ShowWindow(hWnd, SW_MINIMIZE);
						ShowWindow(hWnd, SW_RESTORE);
						dxSwapchain->SetFullscreenState(TRUE, NULL);
					}
				}
			}

			//--

			Swapchain::Swapchain(OutputClass cls, const WindowSetup& setup, IDXGISwapChain* dxSwapchain, ID3D11Device* dxDevice, ID3D11Texture2D* dxColorBackBuffer, ID3D11RenderTargetView* dxColorBackBufferRTV)
				: IBaseWindowedSwapchain(cls, setup)
				, m_dxSwapchain(dxSwapchain)
				, m_dxDevice(dxDevice)
				, m_dxColorBackbuffer(dxColorBackBuffer)
				, m_dxColorBackbufferRTV(dxColorBackBufferRTV)
			{
				DXGI_SWAP_CHAIN_DESC desc;
				memzero(&desc, sizeof(desc));
				DXGI_PROTECT(m_dxSwapchain->GetDesc(&desc));

				m_swapchainBackbufferWidth_ClientApi = desc.BufferDesc.Width;
				m_swapchainBackbufferHeight_ClientApi = desc.BufferDesc.Height;
			}

			Swapchain::~Swapchain()
			{
				DisableFullscreen(m_dxSwapchain, (HWND)m_windowHandle);

				DX_RELEASE(m_dxDepthBackbufferRTV);
				DX_RELEASE(m_dxDepthBackbuffer);

				DX_RELEASE(m_dxColorBackbufferRTV);
				DX_RELEASE(m_dxColorBackbuffer);

				DX_RELEASE(m_dxSwapchain);
			}

			bool Swapchain::prepare_ClientApi(SwapchainState& outState)
			{
				if (!IBaseWindowedSwapchain::prepare_ClientApi(outState))
					return false;

				if (m_swapchainBackbufferWidth_ClientApi != outState.width
					|| m_swapchainBackbufferHeight_ClientApi != outState.height)
				{
					TRACE_INFO("Resizing swapchain [{}x{}] -> [{}x{}]", m_swapchainBackbufferWidth_ClientApi, m_swapchainBackbufferHeight_ClientApi, outState.width, outState.height);
					m_swapchainBackbufferWidth_ClientApi = outState.width;
					m_swapchainBackbufferHeight_ClientApi = outState.height;

					DWORD dwFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
					DX_PROTECT(m_dxSwapchain->ResizeBuffers(2, outState.width, outState.height, DXGI_FORMAT_UNKNOWN, dwFlags));
				}

				return true;
			}

			bool Swapchain::acquire()
			{
				// get swapchain info
				DXGI_SWAP_CHAIN_DESC desc;
				memzero(&desc, sizeof(desc));
				HRESULT hRet = m_dxSwapchain->GetDesc(&desc);
				if (hRet != S_OK)
					return false;

				// create depth buffer
				if (m_depthFormat != ImageFormat::UNKNOWN)
				{
					// size changed
					if (desc.BufferDesc.Width != m_depthBufferWidth || desc.BufferDesc.Height != m_depthBufferHeight)
					{
						DX_RELEASE(m_dxDepthBackbufferRTV);
						DX_RELEASE(m_dxDepthBackbuffer);
						TRACE_INFO("Resizing depth buffer [{}x{}] -> [{}x{}]", m_depthBufferWidth, m_depthBufferHeight, desc.BufferDesc.Width, desc.BufferDesc.Height);
					}

					// create missing depth buffer
					if (!m_dxDepthBackbuffer)
					{
						D3D11_TEXTURE2D_DESC desc;
						memzero(&desc, sizeof(desc));
						desc.Width = m_depthBufferWidth;
						desc.Height = m_depthBufferHeight;
						desc.SampleDesc.Count = m_samples;
						desc.MipLevels = 1;
						desc.ArraySize = 1;
						desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
						desc.Usage = D3D11_USAGE_DEFAULT;
						desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

						ID3D11Texture2D* dxDepthBufferTex = nullptr;
						DX_PROTECT(m_dxDevice->CreateTexture2D(&desc, NULL, &dxDepthBufferTex));
						if (dxDepthBufferTex)
						{
							// get the depth RTV
							D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
							memzero(&rtvDesc, sizeof(rtvDesc));
							rtvDesc.Format = desc.Format;
							rtvDesc.Texture2D.MipSlice = 0;

							ID3D11RenderTargetView* dxDepthBufferRTV = nullptr;
							DX_PROTECT(m_dxDevice->CreateRenderTargetView(dxDepthBufferTex, &rtvDesc, &dxDepthBufferRTV));
							if (dxDepthBufferRTV)
							{
								m_dxDepthBackbuffer = dxDepthBufferTex;
								m_dxDepthBackbufferRTV = dxDepthBufferRTV;
							}
							else
							{
								DX_RELEASE(dxDepthBufferTex);
							}
						}
					}

					// if we don't have depth surface then don't render
					if (!m_dxDepthBackbuffer)
						return false;
				}

				// we can render only if we have all data
				return true;
			}

			void Swapchain::present(bool forReal)
			{
				if (forReal)
					m_dxSwapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
			}

			//--

		} // dx11
    } // api
} // rendering
