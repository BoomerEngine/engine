/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\process\winapi #]
* [#platform: windows #]
***/

#include "build.h"
#include "pipe.h"
#include "processWindows.h"

namespace base
{
    namespace process
    {
        namespace prv
        {

            //--

            namespace helper
            {
                class StdHandleReader
                {
                public:
                    StdHandleReader(IOutputCallback* callback);
                    ~StdHandleReader();

                    INLINE HANDLE GetHandle() { return m_hStdOutWrite; }

                private:
                    HANDLE m_hStdOutRead;
                    HANDLE m_hStdOutWrite;

                    HANDLE m_hThread;
                    volatile bool m_exitThread;

                    IOutputCallback* m_callback;

                    class LineBuilder
                    {
                    public:
                        LineBuilder();
                        void append(IOutputCallback* callback, const uint8_t* data, uint32_t dataSize);

                    private:
                        std::vector<uint8_t> m_buffer;
                    };

                    static DWORD WINAPI ThreadProc(LPVOID pData);
                };

                StdHandleReader::LineBuilder::LineBuilder()
                {
                }

                void StdHandleReader::LineBuilder::append(IOutputCallback* callback, const uint8_t* data, uint32_t dataSize)
                {
                    for (uint32_t i = 0; i < dataSize; ++i)
                    {
                        uint8_t ch = data[i];
                        if (ch == '\r')
                        {
                            if (!m_buffer.empty())
                            {
                                m_buffer.push_back(0);
                                callback->processData(m_buffer.data(), (uint32_t)m_buffer.size());
                            }

                            m_buffer.clear();
                        }

                        if (ch >= ' ')
                            m_buffer.push_back((char)ch);
                    }
                }

                StdHandleReader::StdHandleReader(IOutputCallback* callback)
                    : m_exitThread(false)
                    , m_hStdOutRead(NULL)
                    , m_hStdOutWrite(NULL)
                    , m_hThread(NULL)
                    , m_callback(callback)
                {
                    SECURITY_ATTRIBUTES saAttr;
                    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
                    saAttr.bInheritHandle = TRUE;
                    saAttr.lpSecurityDescriptor = NULL;

                    CreatePipe(&m_hStdOutRead, &m_hStdOutWrite, &saAttr, 0);

                    m_hThread = CreateThread(NULL, 64 << 10, &ThreadProc, this, 0, NULL);
                }

                StdHandleReader::~StdHandleReader()
                {
                    m_exitThread = true;
                    WaitForSingleObject(m_hThread, 2000);
                    CloseHandle(m_hThread);

                    CloseHandle(m_hStdOutRead);
                    m_hStdOutRead = NULL;

                    CloseHandle(m_hStdOutWrite);
                    m_hStdOutWrite = NULL;
                }

                DWORD WINAPI StdHandleReader::ThreadProc(LPVOID pData)
                {
                    StdHandleReader* log = (StdHandleReader*)pData;

                    LineBuilder lineBuilder;

                    while (1)
                    {
                        // buffer size
                        BYTE readBuffer[128];

                        // has data ?
                        DWORD numAvaiable = 0;
                        PeekNamedPipe(log->m_hStdOutRead, NULL, NULL, NULL, &numAvaiable, NULL);
                        if (!numAvaiable)
                        {
                            if (log->m_exitThread)
                                break;

                            Sleep(10);
                            continue;
                        }

                        // read data
                        DWORD numRead = 0;
                        DWORD numToRead = numAvaiable;
                        if (numToRead > sizeof(readBuffer))
                            numToRead = sizeof(readBuffer);

                        if (!ReadFile(log->m_hStdOutRead, readBuffer, sizeof(readBuffer), &numRead, NULL))
                            break;

                        // append to line printer
                        lineBuilder.append(log->m_callback, readBuffer, numRead);
                    }

                    return 0;
                }

            } // helper

            //--

            WinProcess::WinProcess()
                : m_hProcess(NULL)
                , m_stdReader(nullptr)
                , m_id(0)
            {}

            WinProcess::~WinProcess()
            {
                if (!wait(500))
                    terminate();

                ASSERT_EX(m_hProcess == NULL, "Process not properly closed");
                ASSERT_EX(m_stdReader == NULL, "Process not properly closed");
            }

            bool WinProcess::wait(uint32_t timeoutMS)
            {
                if (m_hProcess == NULL)
                    return true;

                int ret = WaitForSingleObject(m_hProcess, timeoutMS);
                if (ret != WAIT_OBJECT_0)
                    return false;

                if (m_stdReader != nullptr)
                {
                    delete m_stdReader;
                    m_stdReader = nullptr;
                }

                CloseHandle(m_hProcess);
                m_hProcess = NULL;
                return true;
            }

            void WinProcess::terminate()
            {
                // stop the reader
                if (m_stdReader != nullptr)
                {
                    delete m_stdReader;
                    m_stdReader = nullptr;
                }

                // stop the process
                if (m_hProcess != NULL)
                {
                    TerminateProcess(m_hProcess, 666);
                    CloseHandle(m_hProcess);
                    m_hProcess = NULL;
                }
            }

            ProcessID WinProcess::id() const
            {
                return m_id;
            }

            bool WinProcess::isRunning() const
            {
                DWORD exitCode = 0;
                if (!GetExitCodeProcess(m_hProcess, &exitCode))
                    return false;
                return (exitCode == STILL_ACTIVE);
            }

            bool WinProcess::exitCode(int& outExitCode) const
            {
                DWORD exitCode = 0;
                return STILL_ACTIVE != GetExitCodeProcess(m_hProcess, &exitCode);
            }

            namespace helper
            {
                static HANDLE CreateClosingJob()
                {
                    HANDLE ghJob = CreateJobObject(NULL, NULL); // GLOBAL
                    if (ghJob != NULL)
                    {
                        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };

                        // Configure all child processes associated with the job to terminate when the
                        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                        if (0 != SetInformationJobObject(ghJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
                        {
                            return ghJob;
                        }
                    }

                    return NULL;
                }
            }

            WinProcess* WinProcess::Create(const ProcessSetup& setup)
            {
                static const HANDLE hClosingJob = helper::CreateClosingJob();

                PROCESS_INFORMATION pinfo;
                memset(&pinfo, 0, sizeof(pinfo));

                STARTUPINFOW startupInfo;
                memset(&startupInfo, 0, sizeof(startupInfo));
                startupInfo.cb = sizeof(startupInfo);

                // create the stdout capturer
                std::unique_ptr<helper::StdHandleReader> stdOutReader;
                if (setup.m_stdOutCallback != nullptr)
                {
                    stdOutReader.reset(new helper::StdHandleReader(setup.m_stdOutCallback));
                    startupInfo.hStdOutput = stdOutReader->GetHandle();
                    startupInfo.hStdError = stdOutReader->GetHandle();
                    startupInfo.dwFlags |= STARTF_USESTDHANDLES;
                }

                // no window show
                if (!setup.m_showWindow)
                {
                    startupInfo.wShowWindow = SW_HIDE;
                    startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
                }

                // prepare arguments
                std::wstring temp;

                // append process name
                {
                    temp += L"\"";
                    if (setup.m_processPath.empty())
                    {
                        WCHAR str[MAX_PATH];
                        GetModuleFileNameW(NULL, str, MAX_PATH);
                        temp += str;
                    }
                    else
                    {
                        temp += setup.m_processPath.c_str();
                    }
                }

                // append rest of the command line stuff
				for (auto& arg : setup.m_arguments)
				{
					temp += L"\" ";
					temp += arg.uni_str().c_str();
				}

                // create argument buffer
                auto argsSize = temp.length();
                auto args  = (wchar_t*)malloc(sizeof(wchar_t) * (argsSize + 1));
                if (!args)
                    return nullptr;
                wcscpy_s(args, argsSize + 1, temp.c_str());//m_arguments.c_str());

                // start process
                if (!CreateProcessW(
                    NULL,   // lpApplicationName
                    args,   // lpCommandLine
                    NULL,   // lpProcessAttributes
                    NULL,   // lpThreadAttributes
                    TRUE,   // bInheritHandles
                    0,      // dwCreationFlags
                    NULL,   // lpEnvironment
                    NULL,   // lpCurrentDirectory
                    &startupInfo, // lpStartupInfo
                    &pinfo))
                {
                    auto errorCode = GetLastError();
                    TRACE_ERROR("Failed to start '{}': error code {}", setup.m_processPath, Hex(errorCode));
                    return nullptr;
                }

                // make sure the child process is closed whenever we are closed
                AssignProcessToJobObject(hClosingJob, pinfo.hProcess);

                // create the object
                auto ret  = MemNew(WinProcess);
                ret->m_hProcess = pinfo.hProcess;
                ret->m_id = pinfo.dwProcessId;
                ret->m_stdReader = stdOutReader.release();
                return ret;
            }

        } // prv
    } // process
} // base
