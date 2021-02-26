/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/api_common/include/apiDevice.h"
#include "gpu/api_dxgi/include/dxgiHelper.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::dx11)

//--

// a NULL implementation of the rendering api
class GPU_API_DX11_API Device : public IBaseDevice
{
	RTTI_DECLARE_VIRTUAL_CLASS(Device, IBaseDevice);

public:
	Device();
	virtual ~Device();

	//--

	virtual StringBuf name() const override final;

	//--

private:
	virtual bool initialize(const app::CommandLine& cmdLine, DeviceCaps& outCaps) override final;
	virtual void shutdown() override final;

	virtual IBaseThread* createOptimalThread(const app::CommandLine& cmdLine) override final;

	DXGIHelper* m_dxgi = nullptr;
	uint32_t m_adapterIndex = 0;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::dx11)
