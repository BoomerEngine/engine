/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\process #]
***/

#include "build.h"
#include "process.h"

#ifdef PLATFORM_WINDOWS
    #include "processWindows.h"
#elif defined(PLATFORM_POSIX)
    #include "processPOSIX.h"
#endif

BEGIN_BOOMER_NAMESPACE(base::process)

//---

ProcessSetup::ProcessSetup()
{}

//---

IProcess::~IProcess()
{
}

IProcess* IProcess::Create(const ProcessSetup& setup)
{
#if defined(PLATFORM_WINDOWS)
    return prv::WinProcess::Create(setup);
#elif defined(PLATFORM_POSIX)
    return prv::POSIXProcess::Create(setup);
#else
    return nullptr; // no thread needed apparently :P
#endif
}

//---

END_BOOMER_NAMESPACE(base::process)