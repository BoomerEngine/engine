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

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//--

struct ThreadSharedContextWinApi
{
	RTTI_DECLARE_POOL(POOL_API_RUNTIME);

public:
	ThreadSharedContextWinApi(HDC dc, HGLRC rc);
	~ThreadSharedContextWinApi();

	void activate();
	void deactivate();

private:
	HDC hDC = NULL;
	HGLRC hRC = NULL;
};

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
	virtual IBaseBackgroundQueue* createOptimalBackgroundQueue(const app::CommandLine& cmdLine) override final;

	//--

	ThreadSharedContextWinApi* createSharedContext();

	//--

private:
	HGLRC m_hRC = NULL;
	HDC m_hFakeDC = NULL;
	HWND m_hFakeHWND = NULL;
	HDC m_hActiveDC = NULL;

	virtual bool threadStartup(const app::CommandLine& cmdLine, DeviceCaps& outCaps) override final;
	virtual void threadFinish() override final;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
