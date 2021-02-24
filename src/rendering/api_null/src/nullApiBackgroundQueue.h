/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiBackgroundJobs.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::nul)

//---

class BackgroundQueue : public IBaseBackgroundQueue
{
public:
	BackgroundQueue();

	virtual bool createWorkerThreads(uint32_t requestedCount, uint32_t& outNumCreated) override final;
	virtual void stopWorkerThreads() override final;
};

//---

END_BOOMER_NAMESPACE(rendering::api::nul)