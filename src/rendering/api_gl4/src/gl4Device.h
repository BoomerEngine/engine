/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiDevice.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::gl4)

// a NULL implementation of the rendering api
class Device : public IBaseDevice
{
	RTTI_DECLARE_VIRTUAL_CLASS(Device, IBaseDevice);

public:
	Device();
	virtual ~Device();

	//--

	virtual base::StringBuf name() const override final;

	//--

private:
	virtual IBaseThread* createOptimalThread(const base::app::CommandLine& cmdLine) override final;
};

//--

END_BOOMER_NAMESPACE(rendering::api::gl4)