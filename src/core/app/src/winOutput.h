/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\windows #]
* [# platform: winapi #]
***/

#pragma once

#include "core/system/include/output.h"

BEGIN_BOOMER_NAMESPACE_EX(platform)

namespace win
{
    class ErrorHandlerDlg;

    // WinAPI console log output
    class GenericOutput : public logging::ILogSink, public logging::IErrorHandler
    {
    public:
        GenericOutput(bool openConsole, bool verbose);
        ~GenericOutput();

        //! Throw exception
        int handleException(EXCEPTION_POINTERS* exception);

    private:
		// IOutputListener interface
        virtual bool print(const logging::OutputLevel level, const char* file, uint32_t line, const char* module, const char* context, const char* text) override final;

		// IErrorListener interface
		virtual void handleFatalError(const char* fileName, uint32_t fileLine, const char* txt) override final;
		virtual void handleAssert(bool isFatal, const char* fileName, uint32_t fileLine, const char* expr, const char* msg, bool* isEnabled) override final;

    private:
        //----

        //! Generate mini dump
        bool generateDump(EXCEPTION_POINTERS* pExcPtrs);

        //----

        //! Handle dialog and it's result
        void handleDialog(ErrorHandlerDlg& dlg, EXCEPTION_POINTERS* pExcPtrs, bool* isEnabledFlag);

        //! Break if no external debugger is attached
        void breakIntoDebugger(EXCEPTION_POINTERS* pExcPtrs);

        //! Crash application at current point
        void crash(EXCEPTION_POINTERS* pExcPtrs, bool panic);

        //----

        HINSTANCE m_hInstance;

        HANDLE  m_stdOut;
        DWORD   m_mainThreadID;

        uint32_t  m_numErrors;
        uint32_t  m_numWarnings;

		bool m_debugEcho = false;
        bool m_verbose = false;

        CRITICAL_SECTION m_lock;

        static void GetCrashReportPath(wchar_t* outPath);
        static void GetCrashDumpPath(wchar_t* outPath);
    };

} // win

END_BOOMER_NAMESPACE_EX(platform)
