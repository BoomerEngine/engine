/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\debug #]
* [#platform: windows #]
***/

#include "build.h"
#include "debug.h"

#include <Windows.h>
#include <DbgHelp.h>

#pragma comment ( lib, "dbghelp.lib" )
#pragma comment ( lib, "Psapi.lib" )

BEGIN_BOOMER_NAMESPACE(base::debug)

void Break()
{
    ::DebugBreak();
}

class DebugHelper
{
public:
    static bool Initialize()
    {
        static DebugHelper theInstance;
        return theInstance.m_initialized;
    }

public:
    DebugHelper()
    {
        // Initialize symbols
        if (SymInitialize(GetCurrentProcess(), 0, TRUE))
        {
            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEBUG);
            m_initialized = true;
        }
        else
        {
            uint32_t errorCode = GetLastError();
            TRACE_ERROR("Failed to initalize symbol access: 0x%X", errorCode);
            m_initialized = false;
        }
    }

    ~DebugHelper()
    {
        ::SymCleanup(GetCurrentProcess());
    }

    bool m_initialized;
};

bool DebuggerPresent()
{
    static bool flag = IsDebuggerPresent(); // expensive to evaluate and we usually start attached
    return flag;
}

bool GrabCallstack(uint32_t skipInitialFunctions, const void* exptr, Callstack& outCallstack)
{
    size_t functionAddresses[Callstack::MAX_FRAMES];
    auto count = RtlCaptureStackBackTrace(skipInitialFunctions, Callstack::MAX_FRAMES - 1, (void**)functionAddresses, NULL);
    if (!count)
        return false;

    outCallstack.reset();
    for (uint32_t i = 0; i < count; ++i)
        outCallstack.push(functionAddresses[i]);

    return true;
}

bool TranslateSymbolName(uint64_t functionAddress, SymbolName& outSymbolName, SymbolName& outFileName)
{
    // Initialize to some default info
    outSymbolName.set("<unknown-symbol>");
    outFileName.set("<unknown-file>");

    // initialize symbol access
    if (!DebugHelper::Initialize())
        return false;

    // get file name
    {
        IMAGEHLP_LINE64 lineInfo = { 0 };
        lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        uint32_t displacement = 0;

        // Get symbol location
        bool result = SymGetLineFromAddr64(GetCurrentProcess(), functionAddress, (PDWORD)&displacement, &lineInfo) == TRUE;
        if (result)
        {
            // Get file name
            char* offset = strstr(lineInfo.FileName, "\\sources\\");
            char* fileName = offset ? offset + 9 : lineInfo.FileName;

            // Append info
            outFileName.set(base::TempString("{}({})", fileName, lineInfo.LineNumber));
        }
    }

    // get symbol name
    {
        uint8_t symbolBuffer[sizeof(SYMBOL_INFO) + 1024];
        ZeroMemory(symbolBuffer, sizeof(symbolBuffer));

        PSYMBOL_INFO symbolInfo = (PSYMBOL_INFO)symbolBuffer;
        symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbolInfo->MaxNameLen = 1024;
        DWORD64 displacement = 0;

        // Get symbol name
        bool result = SymFromAddr(GetCurrentProcess(), functionAddress, &displacement, symbolInfo) == TRUE;
        if (result)
            outSymbolName.set(base::TempString("{}()+{}", symbolInfo->Name, displacement));
    }

    // done
    return true;
}

END_BOOMER_NAMESPACE(base::debug)