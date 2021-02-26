/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\output #]
***/

#include "build.h"
#include "output.h"
#include "outputStream.h"

BEGIN_BOOMER_NAMESPACE_EX(logging)

//--

static TYPE_TLS LineAssembler* GLogLineStream = nullptr;

//--

ILogSink::~ILogSink()
{}

//--

LocalLogSink::LocalLogSink()
{
    m_previousLocalSink = Log::MountLocalSink(this);
}

LocalLogSink::~LocalLogSink()
{
    auto* prev = Log::MountLocalSink(m_previousLocalSink);
    DEBUG_CHECK_EX(prev == this, "Something went wrong with local log sinks attach/detach order");
}

bool LocalLogSink::print(OutputLevel level, const char* file, uint32_t line, const char* module, const char* context, const char* text)
{
    if (m_previousLocalSink)
        return m_previousLocalSink->print(level, file, line, module, context, text);
    else
        return false;
}

//--

GlobalLogSink::GlobalLogSink()
{
    Log::AttachGlobalSink(this);
}

GlobalLogSink::~GlobalLogSink()
{
    Log::DetachGlobalSink(this);
}

//--

void Log::Print(OutputLevel level, const char* file, uint32_t line, const char* module, const char* context, const char* text)
{
    SinkTable::GetInstance().print(level, file, line, module, context, text);
}

IFormatStream& Log::Stream(OutputLevel level /*= OutputLevel::Info*/, const char* moduleName /*= nullptr*/, const char* contextFile /*= nullptr*/, uint32_t contextLine /*= 0*/)
{
    if (nullptr == GLogLineStream)
        GLogLineStream = new LineAssembler(); // never released, but it's a global log FFS, don't bother

    GLogLineStream->takeOwnership(level, moduleName, contextFile, contextLine);
    return *GLogLineStream;

    // TODO: return proxy object that will call "release ownership"
}

void Log::SetThreadContextName(const char* name)
{
    if (nullptr == GLogLineStream)
        GLogLineStream = new LineAssembler(); // never released, but it's a global log FFS, don't bother

    GLogLineStream->changeContext(name);
}

ILogSink* Log::MountLocalSink(ILogSink* sink)
{
    if (nullptr == GLogLineStream)
        GLogLineStream = new LineAssembler(); // never released, but it's a global log FFS, don't bother


    return GLogLineStream->mountLocalSink(sink);
}

ILogSink* Log::GetCurrentLocalSink()
{
    if (nullptr != GLogLineStream)
        return GLogLineStream->localSink();
    return nullptr;
}

void Log::AttachGlobalSink(ILogSink* sink)
{
    SinkTable::GetInstance().attach(sink);
}

void Log::DetachGlobalSink(ILogSink* sink)
{
    SinkTable::GetInstance().detach(sink);
}

//--

static IErrorHandler* GErrorListener = nullptr;

void IErrorHandler::BindListener(IErrorHandler* listener)
{
    GErrorListener = listener;
}

void IErrorHandler::Assert(bool isFatal, const char *fileName, uint32_t fileLine, const char* expr, const char* message, bool* isEnabled)
{
    if (GErrorListener)
    {
        GErrorListener->handleAssert(isFatal, fileName, fileLine, expr, message, isEnabled);
    }
    else
    {
        fprintf(stderr, "Early initialization assertion failed: %s\n", expr);
        fprintf(stderr, "%s(%u): error: %s\n", fileName, fileLine, message);

        debug::Break();
    }
}

void IErrorHandler::FatalError(const char* fileName, uint32_t fileLine, const char *text)
{
    if (GErrorListener)
    {
        GErrorListener->handleFatalError(fileName, fileLine, text);
    }
    else
    {
        fprintf(stderr, "%s(%u): fatal error: %s\n", fileName, fileLine, text);
        debug::Break();
    }
}

END_BOOMER_NAMESPACE_EX(logging)
