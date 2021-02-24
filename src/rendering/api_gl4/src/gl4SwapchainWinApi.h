/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: platform\winapi #]
* [# platform: winapi #]
***/

#pragma once

#include "rendering/api_common/include/apiSwapchain.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::gl4)

//--

class ThreadWinApi;

// WinAPI specific OpenGL 4 driver thread
class SwapchainWinApi : public IBaseWindowedSwapchain
{
public:
	SwapchainWinApi(OutputClass cls, const WindowSetup& setup, void* hWnd, void* hDC, ThreadWinApi* thread);
	virtual ~SwapchainWinApi();

	virtual bool acquire() override final;
	virtual void present(bool swap = true) override final;

private:
	ThreadWinApi* m_thread = nullptr;
	void* m_hWnd = NULL;
	void* m_hDC = NULL;
};

//--

END_BOOMER_NAMESPACE(rendering::api::gl4)