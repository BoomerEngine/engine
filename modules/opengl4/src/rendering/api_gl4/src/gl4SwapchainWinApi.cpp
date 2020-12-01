/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: platform\winapi #]
***/

#include "build.h"
#include "gl4ThreadWinApi.h"
#include "gl4SwapchainWinApi.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			SwapchainWinApi::SwapchainWinApi(OutputClass cls, const WindowSetup& setup, void* hWnd, void* hDC, ThreadWinApi* thread)
				: IBaseWindowedSwapchain(cls, setup)
				, m_thread(thread)
				, m_hWnd(hWnd)
				, m_hDC(hDC)
			{}

			SwapchainWinApi::~SwapchainWinApi()
			{
				m_thread->releaseSwapchain(m_hWnd, m_hDC);
			}

			bool SwapchainWinApi::acquire()
			{
				// TODO: device lost?

				m_thread->acquireSwapchain(m_hWnd, m_hDC);
				return true;
			}

			void SwapchainWinApi::present(bool forReal)
			{
				if (forReal)
					m_thread->presentSwapchain(m_hWnd, m_hDC);
			}

			//--

		} // gl4
    } // api
} // rendering
