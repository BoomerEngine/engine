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

namespace rendering
{
	namespace api
	{
		namespace nul
		{
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

		} // nul
	} // api
} // rendering
