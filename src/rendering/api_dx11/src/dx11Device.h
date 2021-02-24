/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiDevice.h"
#include "rendering/api_dxgi/include/dxgiHelper.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::dx11)

//--

// a NULL implementation of the rendering api
class RENDERING_API_DX11_API Device : public IBaseDevice
{
	RTTI_DECLARE_VIRTUAL_CLASS(Device, IBaseDevice);

public:
	Device();
	virtual ~Device();

	//--

	virtual base::StringBuf name() const override final;

	//--

private:
	virtual bool initialize(const base::app::CommandLine& cmdLine, DeviceCaps& outCaps) override final;
	virtual void shutdown() override final;

	virtual IBaseThread* createOptimalThread(const base::app::CommandLine& cmdLine) override final;

	DXGIHelper* m_dxgi = nullptr;
	uint32_t m_adapterIndex = 0;
};

//--

END_BOOMER_NAMESPACE(rendering::api::dx11)