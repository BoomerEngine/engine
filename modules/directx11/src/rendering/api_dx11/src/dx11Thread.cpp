/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"

#include "dx11Device.h"
#include "dx11Thread.h"
#include "dx11FrameFence.h"
#include "dx11CopyQueue.h"
#include "dx11CopyPool.h"
#include "dx11ObjectCache.h"
#include "dx11TransientBuffer.h"
#include "dx11Swapchain.h"
#include "dx11Executor.h"
#include "dx11GraphicsPassLayout.h"
#include "dx11Shaders.h"
#include "dx11Sampler.h"
#include "dx11Image.h"
#include "dx11Buffer.h"

#include "rendering/api_common/include/apiObjectRegistry.h"
#include "rendering/api_common/include/apiSwapchain.h"
#include "base/app/include/commandline.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

	        //--

		    base::ConfigProperty<float> cvFakeGPUWorkTime("NULL", "FakeGPUWorkTime", 10);

			//--

			Thread::Thread(Device* drv, WindowManager* windows, DXGIHelper* dxgi)
				: IBaseThread(drv, windows)
				, m_dxgi(dxgi)
			{
			}

			Thread::~Thread()
			{
			}

			//--

			bool Thread::threadStartup(const base::app::CommandLine& cmdLine)
			{
				// determine device type
				D3D_DRIVER_TYPE deviceType = D3D_DRIVER_TYPE_HARDWARE;
				{
					const auto value = cmdLine.singleValue("dxDriverType");
					if (value == "software")
						deviceType = D3D_DRIVER_TYPE_SOFTWARE;
					else if (value == "reference")
						deviceType = D3D_DRIVER_TYPE_REFERENCE;
					else if (value == "hardware")
						deviceType = D3D_DRIVER_TYPE_HARDWARE;
					else if (value == "null")
						deviceType = D3D_DRIVER_TYPE_NULL;
					else if (value == "warp")
						deviceType = D3D_DRIVER_TYPE_WARP;
					else
						TRACE_ERROR("Unknown DX device type '{}'", value);
				}

				// flags
				DWORD flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
				if (cmdLine.singleValue("dxMultithreaded"))
					flags &= ~D3D11_CREATE_DEVICE_SINGLETHREADED;
				if (cmdLine.singleValue("dxNoThreads"))
					flags |= D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;				
				if (cmdLine.singleValue("dxDebuggable"))
					flags |= D3D11_CREATE_DEVICE_DEBUGGABLE;
				if (cmdLine.singleValue("dxNoGpuTimeout"))
					flags |= D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;				

				// debug
#ifdef BUILD_RELEASE
				if (cmdLine.singleValue("dxDebug"))
					flags |= D3D11_CREATE_DEVICE_DEBUG;
#else
				if (!cmdLine.singleValue("dxNoDebug"))
					flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
				

				D3D_FEATURE_LEVEL requestedFeatureLevels[2] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
				D3D_FEATURE_LEVEL supportedFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_1_0_CORE;
				ID3D11Device* dxDevice = nullptr;
				ID3D11DeviceContext* dxDeviceContext = nullptr;
				DX_PROTECT(D3D11CreateDevice(m_dxgi->dxAdapter(), deviceType, NULL, flags, &supportedFeatureLevel, 1, D3D11_SDK_VERSION, &dxDevice, &supportedFeatureLevel, &dxDeviceContext));

				if (!dxDevice)
				{
					TRACE_ERROR("Failed to initialize DirectX 11");
					return false;
				}

				if (supportedFeatureLevel == D3D_FEATURE_LEVEL_11_1)
				{
					TRACE_INFO("DirectX supports feature level 11.1");
				}
				else
				{
					TRACE_INFO("DirectX supports feature level 11.0");
				}

				m_dxDevice = dxDevice;
				m_dxDeviceContext = dxDeviceContext;

				return IBaseThread::threadStartup(cmdLine);
			}

			void Thread::threadFinish()
			{
				IBaseThread::threadFinish();

				DX_RELEASE(m_dxDeviceContext);
				DX_RELEASE(m_dxDevice);
			}

			//--

			void Thread::syncGPU_Thread()
			{
				PC_SCOPE_LVL0(SyncGPU);

				base::ScopeTimer timer;

				D3D11_QUERY_DESC desc;
				memzero(&desc, sizeof(desc));
				desc.Query = D3D11_QUERY_EVENT;

				ID3D11Query* dxQuery = nullptr;
				DX_PROTECT(m_dxDevice->CreateQuery(&desc, &dxQuery));
				DEBUG_CHECK_RETURN_EX(dxQuery != nullptr, "Query not created");

				// wait
				for (;;)
				{
					BOOL data = FALSE;
					HRESULT hRet = m_dxDeviceContext->GetData(dxQuery, &data, sizeof(data), 0);
					if (hRet == S_OK)
					{
						TRACE_INFO("GPU flushed after {}", timer)
						break;
					}
					else if (hRet != S_FALSE)
					{
						TRACE_ERROR("GPU flushing error: {} (0x{})", TranslateDXError(hRet), Hex(hRet));
						break;
					}

					// yield - we are not flushing in the middle of the game so freeing some CPU is not a bad ide
					::Sleep(0);
				}
			}

			void Thread::execute_Thread(Frame& frame, PerformanceStats& stats, command::CommandBuffer* masterCommandBuffer, RuntimeDataAllocations& data)
			{
				FrameExecutor executor(this, &frame, &stats);
				executor.prepare(data);
				executor.execute(masterCommandBuffer);
			}

			IBaseSwapchain* Thread::createOptimalSwapchain(const OutputInitInfo& info)
			{
				if (info.m_class == OutputClass::Window)
				{
					// we need window for rendering
					auto window = m_windows->createWindow(info);
					DEBUG_CHECK_RETURN_EX_V(window, "Window not created", nullptr);

					// describe swapchan to create
					DXGI_SWAP_CHAIN_DESC desc;
					memzero(&desc, sizeof(desc));

					desc.BufferCount = 2;
					desc.BufferDesc.Width = 0;
					desc.BufferDesc.Height = 0;
					desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					desc.SampleDesc.Count = 1;
					desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
					desc.OutputWindow = (HWND)window;
					desc.Windowed = TRUE;
					desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

					// create swapchain
					IDXGISwapChain* dxSwapchain = nullptr;
					DXGI_PROTECT(m_dxgi->dxFactory()->CreateSwapChain(m_dxDevice, &desc, &dxSwapchain));
					if (!dxSwapchain)
					{
						TRACE_ERROR("Unable to create swapchain");
						m_windows->closeWindow(window);
						return nullptr;
					}

					// get swapchain texture
					ID3D11Texture2D* dxBackBufferTexture = nullptr;
					DXGI_PROTECT(dxSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&dxBackBufferTexture));
					if (!dxBackBufferTexture)
					{
						TRACE_ERROR("Unable to get swapchain's texture");
						DX_RELEASE(dxSwapchain);
						m_windows->closeWindow(window);
						return nullptr;
					}

					// create the RTV for the swapchain's texture
					D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
					memzero(&rtvDesc, sizeof(rtvDesc));
					rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
					rtvDesc.Format = desc.BufferDesc.Format;
					rtvDesc.Texture2D.MipSlice = 0;
					ID3D11RenderTargetView* dxBackBufferRTV = nullptr;
					DXGI_PROTECT(m_dxDevice->CreateRenderTargetView(dxBackBufferTexture, &rtvDesc, &dxBackBufferRTV));
					if (!dxBackBufferRTV)
					{
						TRACE_ERROR("Unable to create swapchain's RTV");
						DX_RELEASE(dxBackBufferTexture);
						DX_RELEASE(dxSwapchain);
						m_windows->closeWindow(window);
						return nullptr;
					}

					// create desc
					{
						IBaseWindowedSwapchain::WindowSetup setup;
						setup.colorFormat = ImageFormat::RGBA8_UNORM;
						setup.depthFormat = ImageFormat::D24S8;
						setup.samples = 1;
						setup.flipped = false;
						setup.deviceHandle = 0;
						setup.windowHandle = window;
						setup.windowManager = m_windows;
						setup.windowInterface = m_windows->windowInterface(window);

						return new Swapchain(info.m_class, setup, dxSwapchain, m_dxDevice, dxBackBufferTexture, dxBackBufferRTV);
					}
				}

				return nullptr;
			}

			IBaseBuffer* Thread::createOptimalBuffer(const BufferCreationInfo& info)
			{
				return new Buffer(this, info);
			}

			IBaseImage* Thread::createOptimalImage(const ImageCreationInfo& info)
			{
				return new Image(this, info);
			}

			IBaseSampler* Thread::createOptimalSampler(const SamplerState& state)
			{
				return new Sampler(this, state);
			}

			IBaseShaders* Thread::createOptimalShaders(const ShaderData* data)
			{
				return new Shaders(this, data);
			}

			IBaseGraphicsPassLayout* Thread::createOptimalPassLayout(const GraphicsPassLayoutSetup& info)
			{
				return new GraphicsPassLayout(this, info);
			}

			IBaseFrameFence* Thread::createOptimalFrameFence()
			{
				D3D11_QUERY_DESC desc;
				memzero(&desc, sizeof(desc));
				desc.Query = D3D11_QUERY_EVENT;

				ID3D11Query* dxQuery = nullptr;
				DX_PROTECT(m_dxDevice->CreateQuery(&desc, &dxQuery));
				
				return new FrameFence(m_dxDeviceContext, dxQuery);
			}

			//--

			IBaseStagingPool* Thread::createOptimalStagingPool(uint32_t size, uint32_t pageSize, const base::app::CommandLine& cmdLine)
			{
				return new CopyPool(size, pageSize);
			}

			IBaseCopyQueue* Thread::createOptimalCopyQueue(const base::app::CommandLine& cmdLine)
			{
				auto* pool = static_cast<CopyPool*>(copyPool());
				return new CopyQueue(this, pool, objectRegistry());
			}

			ObjectRegistry* Thread::createOptimalObjectRegistry(const base::app::CommandLine& cmdLine)
			{
				return new ObjectRegistry(this);
			}

			IBaseObjectCache* Thread::createOptimalObjectCache(const base::app::CommandLine& cmdLine)
			{
				return new ObjectCache(this);
			}

			IBaseTransientBufferPool* Thread::createOptimalTransientStagingPool(const base::app::CommandLine& cmdLine)
			{
				return new TransientBufferPool(this, TransientBufferType::Staging);
			}

			IBaseTransientBufferPool* Thread::createOptimalTransientConstantPool(const base::app::CommandLine& cmdLine)
			{
				return new TransientBufferPool(this, TransientBufferType::Constants);
			}

			//--
	
		} // dx11
    } // api
} // rendering
