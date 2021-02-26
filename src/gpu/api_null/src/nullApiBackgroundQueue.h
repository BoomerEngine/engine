/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/api_common/include/apiBackgroundJobs.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

//---

class BackgroundQueue : public IBaseBackgroundQueue
{
public:
	BackgroundQueue();

	virtual bool createWorkerThreads(uint32_t requestedCount, uint32_t& outNumCreated) override final;
	virtual void stopWorkerThreads() override final;
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
