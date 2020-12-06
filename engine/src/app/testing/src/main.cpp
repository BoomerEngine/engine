/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "static_init.inl"

#include "base/test/include/gtest/gtest.h"
#include "base/fibers/include/fiberSystem.h"
#include "base/io/include/ioSystem.h"
#include "base/app/include/commandline.h"

void* GCurrentModuleHandle = nullptr;

#ifdef PLATFORM_POSIX
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[1;31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[1;33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define KFATA "\x1B[1;30;41m"
#define RESET "\x1B[0m"
#else
#define KNRM
#define KRED
#define KGRN
#define KYEL
#define KBLU
#define KMAG
#define KCYN
#define KWHT
#define KFATA
#define RESET
#endif

class TestLogSink : public base::logging::ILogSink
{
public:
    virtual bool print(base::logging::OutputLevel level, const char* file, uint32_t line, const char* module, const char* context, const char* text) override
    {
        if (level == base::logging::OutputLevel::Spam)
        {
            fprintf(stdout, RESET "%s" RESET "\n", text);
        }
        else if (level == base::logging::OutputLevel::Info)
        {
            fprintf(stdout, KWHT "%s" RESET "\n", text);
        }
        else if (level == base::logging::OutputLevel::Warning)
        {
            fprintf(stdout, KYEL "%s" RESET "\n", text);
        }
        else if (level == base::logging::OutputLevel::Error)
        {
            fprintf(stderr, KRED "%s" RESET "\n", text);
        }
        else if (level == base::logging::OutputLevel::Fatal)
        {
            fprintf(stderr, KRED "%s" RESET "\n", text);
        }

        return false;
    }
};

class BasicErrorHandler : public base::logging::IErrorHandler
{
public:
    virtual void handleFatalError(const char* fileName, uint32_t fileLine, const char* txt) override final
    {
        fprintf(stderr, KRED "%s(%u): fatal error: %s\n" RESET, fileName, fileLine, txt);
    }

    virtual void handleAssert(bool isFatal, const char* fileName, uint32_t fileLine, const char* expr, const char* msg, bool* isEnabled) override final
    {
        fprintf(stderr, KRED "Assertion failed: %s\n" RESET, expr);
        fprintf(stderr, KRED "%s(%u): error: %s\n" RESET, fileName, fileLine, msg && *msg ? msg : expr);
    }
};

int main(int argc, char **argv)
{
    InitializeStaticDependencies();

    Fibers::GetInstance().initialize(base::app::CommandLine());
    base::profiler::Block::InitializeDisabled();
	base::socket::Initialize();

    BasicErrorHandler testErrorHandler;
    base::logging::IErrorHandler::BindListener(&testErrorHandler);

    TestLogSink testSink;
    base::logging::Log::AttachGlobalSink(&testSink);

#ifdef PLATFORM_LINUX
    signal(SIGPIPE, SIG_IGN);
#endif

    testing::InitGoogleTest(&argc, argv);
    auto ret  = RUN_ALL_TESTS();
     
    base::logging::Log::DetachGlobalSink(&testSink);

    return ret;
}
