/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\pipe #]
***/

#include "build.h"
#include "pipe.h"
#include "process.h"

#ifdef PLATFORM_WINDOWS
    #include "pipeWindows.h"
#elif defined(PLATFORM_POSIX)
    #include "pipePOSIX.h"
#endif

BEGIN_BOOMER_NAMESPACE(base::process)

//---

IOutputCallback::~IOutputCallback()
{}

//---

IPipeReader::~IPipeReader()
{}

IPipeWriter::~IPipeWriter()
{}

IPipeWriter* IPipeWriter::Create()
{
#if defined(PLATFORM_WINDOWS)
    return prv::WinPipeWriter::Create();
#elif defined(PLATFORM_POSIX)
    return prv::POSIXPipeWriter::Create();
#else
    return nullptr;
#endif
}

IPipeWriter* IPipeWriter::Open(const char* pipeName)
{
#if defined(PLATFORM_WINDOWS)
    return prv::WinPipeWriter::Open(pipeName);
#elif defined(PLATFORM_POSIX)
    return prv::POSIXPipeWriter::Open(pipeName);
#else
    return nullptr;
#endif
}

IPipeReader* IPipeReader::Create(IOutputCallback* callback)
{
#if defined(PLATFORM_WINDOWS)
    return prv::WinPipeReader::Create(callback);
#elif defined(PLATFORM_POSIX)
    return prv::POSIXPipeReader::Create(callback);
#else
    return nullptr;
#endif
}

IPipeReader* IPipeReader::Open(const char* pipeName, IOutputCallback* callback)
{
#if defined(PLATFORM_WINDOWS)
    return prv::WinPipeReader::Open(pipeName, callback);
#elif defined(PLATFORM_POSIX)
    return prv::POSIXPipeReader::Open(pipeName, callback);
#else
    return nullptr;
#endif
}

//---

END_BOOMER_NAMESPACE(base::process)