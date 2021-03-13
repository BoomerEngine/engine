/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\windows #]
* [# platform: winapi #]
***/

#include "build.h"

#include <Windows.h>
#include "winOutput.h"
#include "winErrorWindow.h"

#include "core/system/include/debug.h"
#include "core/containers/include/stringBuilder.h"

//-----------------------------------------------------------------------------

// Crash report file
#define CRASH_REPORT_FILE       "BoomerCrash.txt"

// Mini dump file
#define CRASH_DUMP_FILE         "BoomerCrash.dmp"

//-----------------------------------------------------------------------------

// GetModuleBaseName
#include <psapi.h>
#include <DbgHelp.h>

#pragma comment ( lib, "dbghelp.lib" )
#pragma comment ( lib, "Psapi.lib" )

//-----------------------------------------------------------------------------

BEGIN_BOOMER_NAMESPACE()

namespace win
{

    //-----------------------------------------------------------------------------

    namespace helper
    {
        static void GetExceptionDesc(EXCEPTION_POINTERS* pExcPtrs, const std::function<void(const char* buffer)>& func)
        {
            DWORD exceptionCode = pExcPtrs->ExceptionRecord->ExceptionCode;
            switch (exceptionCode)
            {
                case EXCEPTION_ACCESS_VIOLATION:
                {
                    if (pExcPtrs->ExceptionRecord->NumberParameters >= 2)
                    {
                        const char* operation = (pExcPtrs->ExceptionRecord->ExceptionInformation[0] == 0) ? "reading" : "writing";
                        auto address  = pExcPtrs->ExceptionRecord->ExceptionInformation[1];
                        func(TempString("EXCEPTION_ACCESS_VIOLATION, Error {} location {}", operation, address));
                    }
                    else
                    {
                        func("EXCEPTION_ACCESS_VIOLATION");
                    }
                    break;
                }

                case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
                {
                    func("EXCEPTION_ARRAY_BOUNDS_EXCEEDED");
                    break;
                }

                case EXCEPTION_BREAKPOINT:
                {
                    func("EXCEPTION_BREAKPOINT");
                    break;
                }

                case EXCEPTION_DATATYPE_MISALIGNMENT:
                {
                    func("EXCEPTION_DATATYPE_MISALIGNMENT");
                    break;
                }

                case EXCEPTION_FLT_DENORMAL_OPERAND:
                {
                    func("EXCEPTION_FLT_DENORMAL_OPERAND");
                    break;
                }

                case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                {
                    func("EXCEPTION_FLT_DIVIDE_BY_ZERO");
                    break;
                }

                case EXCEPTION_FLT_INEXACT_RESULT:
                {
                    func("EXCEPTION_FLT_INEXACT_RESULT");
                    break;
                }

                case EXCEPTION_FLT_INVALID_OPERATION:
                {
                    func("EXCEPTION_FLT_INVALID_OPERATION");
                    break;
                }

                case EXCEPTION_FLT_OVERFLOW:
                {
                    func("EXCEPTION_FLT_OVERFLOW");
                    break;
                }

                case EXCEPTION_FLT_STACK_CHECK:
                {
                    func("EXCEPTION_FLT_STACK_CHECK");
                    break;
                }

                case EXCEPTION_FLT_UNDERFLOW:
                {
                    func("EXCEPTION_FLT_UNDERFLOW");
                    break;
                }

                case EXCEPTION_GUARD_PAGE:
                {
                    func("EXCEPTION_GUARD_PAGE");
                    break;
                }

                case EXCEPTION_ILLEGAL_INSTRUCTION:
                {
                    func("EXCEPTION_ILLEGAL_INSTRUCTION");
                    break;
                }

                case EXCEPTION_IN_PAGE_ERROR:
                {
                    func("EXCEPTION_IN_PAGE_ERROR");
                    break;
                }

                case EXCEPTION_INT_DIVIDE_BY_ZERO:
                {
                    func("EXCEPTION_INT_DIVIDE_BY_ZERO");
                    break;
                }

                case EXCEPTION_INT_OVERFLOW:
                {
                    func("EXCEPTION_INT_OVERFLOW");
                    break;
                }

                case EXCEPTION_INVALID_DISPOSITION:
                {
                    func("EXCEPTION_INVALID_DISPOSITION");
                    break;
                }

                case EXCEPTION_INVALID_HANDLE:
                {
                    func("EXCEPTION_INVALID_HANDLE");
                    break;
                }

                case EXCEPTION_NONCONTINUABLE_EXCEPTION:
                {
                    func("EXCEPTION_NONCONTINUABLE_EXCEPTION");
                    break;
                }

                case EXCEPTION_PRIV_INSTRUCTION:
                {
                    func("EXCEPTION_PRIV_INSTRUCTION");
                    break;
                }

                case EXCEPTION_SINGLE_STEP:
                {
                    func("EXCEPTION_SINGLE_STEP");
                    break;
                }

                case EXCEPTION_STACK_OVERFLOW:
                {
                    func("EXCEPTION_STACK_OVERFLOW");
                    break;
                }

                default:
                {
                    func("<unknown-exception>");
                    break;
                }
            }
        }

        class ScopeLock
        {
        public:
            INLINE ScopeLock(CRITICAL_SECTION& section)
                : m_section(&section)
            {
                EnterCriticalSection(m_section);
            }

            INLINE ~ScopeLock()
            {
                LeaveCriticalSection(m_section);
            }
                        
        private:
            CRITICAL_SECTION*   m_section;
        };


    } // helper

    //-----------------------------------------------------------------------------

    static void FormatLineForDebugOutputPrinting(IFormatStream& f, const logging::OutputLevel level, const char* file, uint32_t line, const char* module, const char* context, const char* text)
    {
        if (file && *file)
        {
            f.append(file);

            if (line != 0)
                f.appendf("({}): ", line);
            else
                f.append(": ");
        }

        switch (level)
        {
            case logging::OutputLevel::Spam: f.append("spam"); break;
            case logging::OutputLevel::Info: f.append("info"); break;
            case logging::OutputLevel::Warning: f.append("warning"); break;
            case logging::OutputLevel::Error: f.append("error"); break;
            case logging::OutputLevel::Fatal: f.append("fatal"); break;
        }

		if (module && *module || context && *context)
		{
			f.append(" (");

			if (module && *module)
				f.append(module);

			if (module && *module && context && *context)
				f.append(",");

			if (context && *context)
				f.append(context);

			f.append("): ");
		}
		else
		{
			f.append(": ");
		}

        f.append(text);
    }

    static void FormatLineForConsolePrinting(IFormatStream& f, const logging::OutputLevel level, const char* file, uint32_t line, const char* module, const char* context, const char* text)
    {
        switch (level)
        {
            case logging::OutputLevel::Spam: f.append("[S]"); break;
            case logging::OutputLevel::Info: f.append("[I]"); break;
            case logging::OutputLevel::Warning: f.append("[W]"); break;
            case logging::OutputLevel::Error: f.append("[E]"); break;
            case logging::OutputLevel::Fatal: f.append("[F]"); break;
        }

		if (module && *module)
			f.appendf("[{}]", module);

        if (context && *context)
            f.appendf("[{}] ", context);
        else
            f.append(" ");

        f.append(text);
        f.append("\r\n");
    }

    class LocalStringBuffer : public IFormatStream
    {
    public:
        static const auto MAX_SIZE = 1200; // max log + file name

        INLINE void reset()
        {
            m_index = 0;
        }

        INLINE const char* c_str() const
        {
            return m_buffer;
        }

        INLINE uint32_t length() const
        {
            return m_index;
        }

        virtual IFormatStream& append(const char* str, uint32_t len /*= INDEX_MAX*/) override
        {
            if (str && *str)
            {
                if (len == INDEX_MAX)
                    len = strlen(str);

                auto maxWrite = std::min<uint32_t>(MAX_SIZE - m_index, len);
                memcpy(m_buffer + m_index, str, maxWrite);
                m_index += maxWrite;
                m_buffer[m_index + 0] = '\r';
                m_buffer[m_index + 1] = '\n';
                m_buffer[m_index + 2] = 0;
            }

            return *this;
        }

    private:
        char m_buffer[MAX_SIZE+4];
        int m_index = 0;
    };

    //-----------------------------------------------------------------------------

    GenericOutput::GenericOutput(bool openConsole, bool verbose)
        : m_stdOut(INVALID_HANDLE_VALUE)
        , m_numErrors(0)
        , m_numWarnings(0)
        , m_verbose(verbose)
		, m_debugEcho(false)
    {
        // Get the master thread ID
        m_mainThreadID = GetCurrentThreadId();

        // Open additional console
        if (openConsole)
        {
            ::AllocConsole();
        }

        // Get the output handle
        m_stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
				
		// Echo to debug stream
		m_debugEcho = IsDebuggerPresent();

        // Create the critical section
        InitializeCriticalSection(&m_lock);
    }

    GenericOutput::~GenericOutput()
    {
    }

    TYPE_TLS LocalStringBuffer* GPrintBuffer = nullptr;

    bool GenericOutput::print(const logging::OutputLevel level, const char* file, uint32_t line, const char* module, const char* context, const char* text)
    {
        if (level == logging::OutputLevel::Meta)
            return false;

        if (level == logging::OutputLevel::Spam && !m_verbose)
            return false;

        if (m_stdOut != INVALID_HANDLE_VALUE)
        {
            if (level == logging::OutputLevel::Fatal)
            {
                SetConsoleTextAttribute(m_stdOut, BACKGROUND_RED | 0); // black text on red background
                m_numErrors += 1;
            }
            else if (level == logging::OutputLevel::Error)
            {
                SetConsoleTextAttribute(m_stdOut, 12);
                m_numErrors += 1;
            }
            else if (level == logging::OutputLevel::Warning)
            {
                SetConsoleTextAttribute(m_stdOut, 14);
                m_numWarnings += 1;
            }
            else if (level == logging::OutputLevel::Spam)
            {
                SetConsoleTextAttribute(m_stdOut, 8); // dark gray text
                m_numWarnings += 1;
            }
            else
            {
                SetConsoleTextAttribute(m_stdOut, 7); // gray text
            }
        }

        {
            if (!GPrintBuffer)
                GPrintBuffer = new LocalStringBuffer();

            if (m_debugEcho)
            {
                GPrintBuffer->reset();
                FormatLineForDebugOutputPrinting(*GPrintBuffer, level, file, line, module, context, text);
                OutputDebugStringA(GPrintBuffer->c_str());
            }

            if (m_stdOut != INVALID_HANDLE_VALUE)
            {
                GPrintBuffer->reset();
                FormatLineForConsolePrinting(*GPrintBuffer, level, file, line, module, context, text);

                // Write to stdout
                DWORD written = 0;
                WriteFile(m_stdOut, GPrintBuffer->c_str(), GPrintBuffer->length(), &written, NULL);
            }
        }

        if (m_stdOut != INVALID_HANDLE_VALUE)
        {
            if ((uint8_t)level >= (uint8_t)logging::OutputLevel::Warning)
                FlushFileBuffers(m_stdOut);

            if (level != logging::OutputLevel::Info)
                SetConsoleTextAttribute(m_stdOut, 7); // gray text
        }

        return false;
    }

    void GenericOutput::handleDialog(ErrorHandlerDlg& dlg, EXCEPTION_POINTERS* pExcPtrs, bool* isEnabledFlag)
    {
        // We cannot disable the assert if we have no flag
        if (isEnabledFlag == nullptr)
            dlg.m_canDisable = false;

        // Show dialog
        auto res  = dlg.showDialog();

        // Process result
        switch (res)
        {
            case ErrorHandlerDlg::EResult::Disable:
                if (isEnabledFlag) *isEnabledFlag = false;
                break;

            case ErrorHandlerDlg::EResult::Ignore:
                break;

            case ErrorHandlerDlg::EResult::Break:
                breakIntoDebugger(pExcPtrs); // may do actual breaking or defer
                break;

            case ErrorHandlerDlg::EResult::Exit:
                crash(pExcPtrs, false);
                break;
        }
    }

    //----------------------------------------------------------------------------- 

    void GenericOutput::handleAssert(bool isFatal, const char* fileName, uint32_t fileLine, const char* expr, const char* msg, bool* isEnabled)
    {
        helper::ScopeLock lock(m_lock);

        ErrorHandlerDlg dlg;

        // we cannot disable fatal asserts
        dlg.m_canContinue = !isFatal;
        dlg.m_canDisable = !isFatal;

        // Set message
        dlg.assertMessage(fileName, fileLine, expr, msg);

        // Set callstack
        debug::Callstack callstack;
        if (debug::GrabCallstack(3, nullptr, callstack))
        {
            dlg.callstack(callstack);
        }

        // Display and process the dialog
        handleDialog(dlg, nullptr, isEnabled);
    }           

    void GenericOutput::handleFatalError(const char* fileName, uint32_t fileLine, const char *message)
    {
        helper::ScopeLock lock(m_lock);

        ErrorHandlerDlg dlg;
        dlg.m_canDisable = false;

#ifdef BUILD_RELEASE
        dlg.m_canContinue = false;
#else
        dlg.m_canContinue = true;
#endif

        // Format message
        dlg.message(message);

        // Set callstack
        debug::Callstack callstack;
        if (debug::GrabCallstack(2, nullptr, callstack))
        {
            dlg.callstack(callstack);
        }

        // Display and process the dialog
        handleDialog(dlg, nullptr, nullptr);
    }           

    //-----------------------------------------------------------------------------

    int GenericOutput::handleException(EXCEPTION_POINTERS* exception)
    {
        // no exception information
        if (exception == nullptr)
            return EXCEPTION_CONTINUE_SEARCH;

        helper::ScopeLock lock(m_lock);

        ErrorHandlerDlg dlg;
        dlg.m_canDisable = false;

#ifdef BUILD_RELEASE
        dlg.m_canContinue = false;
#else
        dlg.m_canContinue = true;
#endif

        // Format message based on exception rules
        helper::GetExceptionDesc(exception, [&dlg](const char* txt) { dlg.message(txt); });

        // Set callstack
        debug::Callstack callstack;
        if (debug::GrabCallstack(2, nullptr, callstack))
        {
            dlg.callstack(callstack);
        }

        // Display and process the dialog
        handleDialog(dlg, exception, nullptr);
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    //-----------------------------------------------------------------------------

    bool GenericOutput::generateDump(EXCEPTION_POINTERS* pExcPtrs)
    {
        // Open dump file
        HANDLE hFile = CreateFileA(CRASH_DUMP_FILE, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // SetupMetadata exception info
            MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
            exceptionInfo.ThreadId = GetCurrentThreadId();
            exceptionInfo.ExceptionPointers = pExcPtrs;
            exceptionInfo.ClientPointers = FALSE;

            // Write mini dump of the crash :)
            bool state = 0 != MiniDumpWriteDump(
                GetCurrentProcess(),
                GetCurrentProcessId(),
                hFile,
                MiniDumpWithDataSegs,
                pExcPtrs ? &exceptionInfo : NULL,
                NULL, NULL);

            // Close dump file
            CloseHandle(hFile);

            // Dump saved
            return state;
        }

        // Dump not saved
        return false;
    }

    //-----------------------------------------------------------------------------

    void GenericOutput::crash(EXCEPTION_POINTERS* pExcPtrs, bool panic)
    {
        if (panic)
        {
            if (pExcPtrs)
            {
                __try
                {
                    ::RaiseFailFastException(pExcPtrs->ExceptionRecord, pExcPtrs->ContextRecord, 0);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                }
            }

            ::RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
        }
        else
        {
            ::SetErrorMode(::GetErrorMode() | SEM_NOGPFAULTERRORBOX);
            ::TerminateProcess(::GetCurrentProcess(), 1);
        }
    }

    static void ToggleWERPopup(bool value)
    {
        HKEY hWerKey = nullptr;
        if (::RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Windows Error Reporting", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hWerKey, nullptr) == ERROR_SUCCESS)
        {
            DWORD regVal = value ? 0 : 1; // if enable, then zero to NOT disable
            ::RegSetValueExW(hWerKey, L"DontShowUI", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&regVal), sizeof(regVal));
            ::CloseHandle(hWerKey);
            hWerKey = nullptr;
        }
    }

    void GenericOutput::breakIntoDebugger(EXCEPTION_POINTERS* pExcPtrs)
    {
        if (!IsDebuggerPresent())
        {
            ToggleWERPopup(true);
            if (pExcPtrs)
            {
                ::RaiseFailFastException(pExcPtrs->ExceptionRecord, pExcPtrs->ContextRecord, 0);
            }
            else
            {
                ::RaiseFailFastException(nullptr, nullptr, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
            }
        }
        else
        {
            DebugBreak();
        }
    }

} // win

END_BOOMER_NAMESPACE()
