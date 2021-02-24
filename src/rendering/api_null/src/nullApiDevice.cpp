/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "nullApiDevice.h"
#include "nullApiThread.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::nul)

//--

RTTI_BEGIN_TYPE_CLASS(Device);
	RTTI_METADATA(DeviceNameMetadata).name("NULL");
RTTI_END_TYPE();

Device::Device()
{
}

Device::~Device()
{
}
        
base::StringBuf Device::name() const
{
	return "NullDevice";
}

IBaseThread* Device::createOptimalThread(const base::app::CommandLine& cmdLine)
{
	return new Thread(this, windows());
}

//---

END_BOOMER_NAMESPACE(rendering::api::nul)