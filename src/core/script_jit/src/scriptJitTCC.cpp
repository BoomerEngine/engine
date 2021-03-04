/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "scriptJitTypeLib.h"
#include "scriptJitTCC.h"

#include "core/io/include/io.h"
#include "core/process/include/process.h"
#include "core/system/include/thread.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//--

RTTI_BEGIN_TYPE_CLASS(JITTCC);
RTTI_END_TYPE();

JITTCC::JITTCC()
{}

StringBuf JITTCC::FindTCCCompiler()
{
    const auto& basePath = SystemPath(PathCategory::ExecutableDir);
    return TempString("{}tcc.exe", basePath);
}

StringBuf JITTCC::FindGCCCompiler()
{
    return "/usr/bin/gcc";
}

class CompilerErrorPrinter : public process::IOutputCallback
{
public:
    virtual void processData(const void* data, uint32_t dataSize) override final
    {
        auto line  = StringView((const char*)data, dataSize);
        TRACE_INFO("JIT: {}", line);
    }
};

bool JITTCC::compile(const IJITNativeTypeInsight& typeInsight, const CompiledProjectPtr& project, StringView outputModulePath, const Settings& settings)
{
    static bool useGCC = false;

    // get path to compiler
    auto compilerPath  = useGCC ? FindGCCCompiler() : FindTCCCompiler();

    // generate code
    if (!TBaseClass::compile(typeInsight, project, outputModulePath, settings))
        return false;

    // write the temp file
    auto tempFile = writeTempSourceFile();
    if (tempFile.empty())
    {
        TRACE_ERROR("JIT: Unable to export generated source code");
        return false;
    }

    process::ProcessSetup processSetup;
    processSetup.m_processPath = compilerPath;
    processSetup.m_showWindow = false;

    // create STDOUT forwarded
    CompilerErrorPrinter stdOut;
    processSetup.m_stdOutCallback = &stdOut;

    // setup commandline
    processSetup.m_arguments.pushBack("-O3");
    processSetup.m_arguments.pushBack("-nostdlib");
    processSetup.m_arguments.pushBack("-shared");
    processSetup.m_arguments.pushBack("-m64");

    if (settings.emitSymbols)
        processSetup.m_arguments.pushBack("-g");

    if (useGCC)
    {
        processSetup.m_arguments.pushBack("-fPIC");
    }
    else
    {
        processSetup.m_arguments.pushBack("-nostdinc");
        processSetup.m_arguments.pushBack("-rdynamic");
    }

    processSetup.m_arguments.pushBack(TempString("-o {}", outputModulePath));
    processSetup.m_arguments.pushBack(tempFile);

    // delete output
    if (FileExists(outputModulePath))
    {
        if (!DeleteFile(outputModulePath))
        {
            TRACE_ERROR("JIT: Output '{}' already exists and can't be deleted (probably in use)", outputModulePath);
            return false;
        }
    }

    // create the process with thumbnail service
    auto process  = process::IProcess::Create(processSetup);
    if (!process)
    {
        TRACE_ERROR("JIT: Failed to start TCC compiler");
        return false;
    }

    // wait for the compiler to finish
    while (process->isRunning())
        Sleep(50);

    // failed ?
    int exitCode = 0;
    if (!process->exitCode(exitCode))
    {
        TRACE_ERROR("JIT: Failed to get exit code from TCC compiler");
        return false;
    }

    // failed ?
    if (exitCode != 0)
    {
        TRACE_ERROR("JIT: TCC compiler finished with exit code {}", exitCode);
        return false;
    }

    // output generated ?
    if (!FileExists(outputModulePath))
    {
        TRACE_ERROR("JIT: No output found after compiled finished");
        return false;
    }

    // done
    TRACE_INFO("JIT: TCC compiler finished with no errors");
    return true;
}

//--

END_BOOMER_NAMESPACE_EX(script)
