/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "gl4Device.h"

#ifdef PLATFORM_WINDOWS
#pragma comment(lib, "opengl32.lib")
#endif

#if defined(PLATFORM_WINAPI)
	#include "gl4ThreadWinApi.h"
	typedef rendering::api::gl4::ThreadWinApi ThreadClass;
#elif defined(PLATFORM_LINUX)
	#include "gl4ThreadX11.h"
	typedef rendering::api::gl4::ThreadX11 ThreadClass;
#endif

BEGIN_BOOMER_NAMESPACE(rendering::api::gl4)

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
	return "GL4";
}

IBaseThread* Device::createOptimalThread(const base::app::CommandLine& cmdLine)
{
	return new ThreadClass(this, windows());
}

//---

END_BOOMER_NAMESPACE(rendering::api::gl4)