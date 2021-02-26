/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/api_common/include/apiDevice.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

// a NULL implementation of the rendering api
class GPU_API_NULL_API Device : public IBaseDevice
{
	RTTI_DECLARE_VIRTUAL_CLASS(Device, IBaseDevice);

public:
	Device();
	virtual ~Device();

	//--

	virtual StringBuf name() const override final;

	//--

private:
	virtual IBaseThread* createOptimalThread(const app::CommandLine& cmdLine) override final;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
