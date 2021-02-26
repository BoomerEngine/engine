/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gpu/api_common/include/apiSwapchain.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

///---

class Swapchain : public IBaseWindowedSwapchain
{
public:
	Swapchain(OutputClass cls, const WindowSetup& setup);
	virtual ~Swapchain(); // can be destroyed only on render thread

	virtual bool acquire() override final;
	virtual void present(bool swap = true) override final;

private:
	bool m_bound = false;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
