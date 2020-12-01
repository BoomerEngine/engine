/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: platform\winapi #]
* [# platform: winapi #]
***/

#pragma once

#include "gl4Thread.h"

#include <Windows.h>

namespace rendering
{
	namespace api
	{
		namespace gl4
		{

			//--

			// WinAPI specific OpenGL 4 driver thread
			class ThreadWinApi : public Thread
			{
			public:
				ThreadWinApi(Device* drv, WindowManager* windows);
				virtual ~ThreadWinApi();

				//--

				void acquireSwapchain(void* hWnd, void* hDC);
				void presentSwapchain(void* hWnd, void* hDC);
				void releaseSwapchain(void* hWnd, void* hDC);

				//--

				virtual IBaseSwapchain* createOptimalSwapchain(const OutputInitInfo& info) override final;

			private:
				HGLRC m_hRC = NULL;
				HDC m_hFakeDC = NULL;
				HWND m_hFakeHWND = NULL;
				HDC m_hActiveDC = NULL;

				virtual bool threadStartup(const base::app::CommandLine& cmdLine) override final;
				virtual void threadFinish() override final;
			};

			//--

		} // gl4
	} // api
} // rendering