/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "gl4Device.h"
#include "gl4Thread.h"

namespace rendering
{
	namespace api
	{
		namespace gl4
		{
			//--

			RTTI_BEGIN_TYPE_CLASS(Device);
				RTTI_METADATA(DeviceNameMetadata).name("GL4");
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

		} // gl4
	} // api
} // rendering
