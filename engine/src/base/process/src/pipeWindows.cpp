/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\pipe\winapi #]
* [#platform: windows #]
***/

#include "build.h"
#include "process.h"
#include "pipeWindows.h"
#include "base/system/include/thread.h"

#include <combaseapi.h>

namespace base
{
    namespace process
    {
        namespace prv
        {

            //--

            namespace helper
            {
                static const char* SystemNameToPipeName(const char* fullName)
                {
                    ASSERT(0 == strncmp(fullName, "\\\\.\\pipe\\", 9));
                    return fullName + 9;
                }

                static void CreateFullPipeName(const char* partName, char* outName)
                {
                    if (partName == nullptr)
                    {
                        GUID g;
                        CoCreateGuid(&g);
                        
                        OLECHAR str[128];
                        StringFromGUID2(g, str, ARRAY_COUNT(str));

                        char ansiStr[128];
                        WideCharToMultiByte(CP_ACP, 0, str, -1, ansiStr, 128, NULL, NULL);

                        sprintf_s(outName, 128, "\\\\.\\pipe\\%s", ansiStr);
                    }
                    else
                    {
                        sprintf_s(outName, 128, "\\\\.\\pipe\\%s", partName);
                    }
                }
            } // helper

            //--

            WinPipeReader::WinPipeReader()
                : m_hPipe(NULL)
                , m_hReadThread(NULL)
                , m_callback(nullptr)
                , m_requestExit(false)
            {}

            WinPipeReader::~WinPipeReader()
            {
                close();

                ASSERT_EX(m_hReadThread == NULL, "Read thread not closed");
                ASSERT_EX(m_hPipe == NULL, "Write thread not closed");
            }

            WinPipeReader* WinPipeReader::Create(IOutputCallback* callback)
            {
                // format proper pipe name
                char fullName[256] = { 0 };
                helper::CreateFullPipeName(nullptr, fullName);

                // create the pipe
                HANDLE hPipe = CreateNamedPipeA(fullName,
                    PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
                    PIPE_TYPE_BYTE | /*PIPE_NOWAIT | */PIPE_REJECT_REMOTE_CLIENTS,
                    2, 1 << 20, 1 << 20, 100, NULL);

                // no pipe
                if (NULL == hPipe)
                {
                    auto errorCode = GetLastError();
                    TRACE_ERROR("Unable to create named pipe '{}', error: {}", fullName, errorCode);
                    return nullptr;
                }

                // info
                TRACE_INFO("Created reading end of pipe '{}'", fullName);

                // create pipeline
                WinPipeReader* pipeline = MemNew(WinPipeReader);
                pipeline->m_hPipe = hPipe;
                pipeline->m_asyncBuffer.resize(64 * 1024);
                strcpy_s(pipeline->m_name, fullName);

                // if we are in the callback mode, create the reader thread
                if (callback)
                {
                    // create the thread
                    pipeline->m_callback = callback;
                    pipeline->m_hReadThread = CreateThread(NULL, 32768, &WinPipeReader::ReadPipeProc, pipeline, 0, NULL);
                    if (NULL == pipeline->m_hReadThread)
                    {
                        auto errorCode = GetLastError();
                        TRACE_ERROR("Unable to create read thread for named pipe '{}', error: {}", fullName, errorCode);
                        delete pipeline;
                        return nullptr;;
                    }
                }

                // return pipeline handle
                return pipeline;                    
            }

            WinPipeReader* WinPipeReader::Open(const char* pipeName, IOutputCallback* callback)
            {
                // format proper pipe name
                char fullName[256] = { 0 };
                helper::CreateFullPipeName(pipeName, fullName);

                // open file
                HANDLE hPipe = CreateFileA(fullName, GENERIC_READ, 0, NULL, OPEN_EXISTING, (callback ? FILE_FLAG_OVERLAPPED : 0), NULL);
                if (INVALID_HANDLE_VALUE == hPipe)
                {
                    auto errorCode = GetLastError();
                    TRACE_ERROR("Unable to open named pipe '{}', error: {}", fullName, errorCode);
                    return nullptr;
                }

                // info
                TRACE_INFO("Opened reading end of pipe '{}'", fullName);

                // create pipeline
                WinPipeReader* pipeline = MemNew(WinPipeReader);
                pipeline->m_hPipe = hPipe;
                pipeline->m_asyncBuffer.resize(64 * 1024);
                strcpy_s(pipeline->m_name, fullName);

                // if we are in the callback mode, create the reader thread
                if (callback)
                {
                    // create the thread
                    pipeline->m_callback = callback;
                    pipeline->m_hReadThread = CreateThread(NULL, 32768, &WinPipeReader::ReadPipeProc, pipeline, 0, NULL);
                    if (NULL == pipeline->m_hReadThread)
                    {
                        auto errorCode = GetLastError();
                        TRACE_ERROR("Unable to create read thread for named pipe '{}', error: {}", fullName, errorCode);
                        delete pipeline;
                        return nullptr;
                    }
                }

                // return pipeline handle
                return pipeline;
            }

            void WinPipeReader::close()
            {
                // request the pipeline thread to close
                TRACE_INFO("Closing reading end of pipe '{}'", name());
                m_requestExit = true;

                // stop thread
                if (NULL != m_hReadThread)
                {
                    if (WAIT_TIMEOUT == WaitForSingleObject(m_hReadThread, 1000))
                    {
                        TerminateThread(m_hReadThread, 0);
                    }

                    CloseHandle(m_hReadThread);
                    m_hReadThread = NULL;

                    TRACE_INFO("Closed reading thread of pipe '{}'", name());

                }
                                
                // close pipe
                if (NULL != m_hPipe)
                {                   
                    CloseHandle(m_hPipe);
                    m_hPipe = NULL;

                    TRACE_INFO("Closed reading end of pipe '{}'", name());
                }               
            }

            const char* WinPipeReader::name() const
            {
                return helper::SystemNameToPipeName(m_name);
            }

            bool WinPipeReader::isOpened() const
            {
                return (m_hPipe != NULL);
            }

            uint32_t WinPipeReader::read(void* data, uint32_t size)
            {
                // pipe reads happen via callback
                if (m_callback)
                    return 0;

                // read data
                DWORD numBytesRead = 0;
                if (m_requestExit.load() == 0)
                {
                    // peek for data
                    if (!PeekNamedPipe(m_hPipe, data, size, &numBytesRead, NULL, NULL))
                    {
                        if (GetLastError() == ERROR_BROKEN_PIPE)
                        {
                            TRACE_ERROR("Failed to read {} bytes to pipe {}", size, name());
                            close();
                        }
                        return 0;
                    }
                }

                return numBytesRead;                
            }


            void WINAPI WinPipeReader::ProcessOverlappedResult(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
            {
                auto pipe  = (WinPipeReader*)base::OffsetPtr(lpOverlapped, -(ptrdiff_t)offsetof(WinPipeReader, m_overlapped));

                if (dwErrorCode != 0)
                {
                    if (dwErrorCode == ERROR_BROKEN_PIPE)
                    {
                        TRACE_WARNING("Pipe '{}' closed remotely", pipe->name());
                        pipe->m_requestExit = true;
                    }
                    else if (dwErrorCode != ERROR_PIPE_LISTENING)
                    {
                        TRACE_ERROR("IO error when reading pipe '{}', error {}", pipe->name(), Hex(dwErrorCode));
                        pipe->m_requestExit = true;
                    }
                }
                else if (dwNumberOfBytesTransfered > 0)
                {
                    auto data  = pipe->m_asyncBuffer.data();
                    TRACE_INFO("Received {} bytes on pipe {}", dwNumberOfBytesTransfered, pipe->m_name);
                    pipe->m_callback->processData(data, dwNumberOfBytesTransfered);
                }

                pipe->m_overlappedReadCompleted = 1;
            }

            DWORD WINAPI WinPipeReader::ReadPipeProc(LPVOID lpThreadParameter)
            {
                auto pipe  = (WinPipeReader*)lpThreadParameter;

                // make sure pipe thread is named
                TRACE_INFO("Started pipe '{}' read thread", pipe->name());
                SetThreadName("PipeReadThread");

                // process the reads
                while (!pipe->m_requestExit.load())
                {
                    // read for data
                    pipe->m_overlappedReadCompleted = 0;
                    memset(&pipe->m_overlapped, 0, sizeof(pipe->m_overlapped));                 

                    // read some data from the pipe
                    if (!ReadFileEx(pipe->m_hPipe, pipe->m_asyncBuffer.data(), (DWORD)pipe->m_asyncBuffer.size(), &pipe->m_overlapped, &WinPipeReader::ProcessOverlappedResult))
                    {
                        auto error = GetLastError();

                        if (error == ERROR_BROKEN_PIPE)
                        {
                            TRACE_INFO("Pipe '{}' closed during reading", pipe->name());
                        }
                        else if (error == ERROR_NO_DATA)
                        {
                            Sleep(10);
                            continue;
                        }
                        else if (error == ERROR_PIPE_LISTENING)
                        {
                            Sleep(10);
                            continue;
                        }
                        else
                        {
                            TRACE_ERROR("IO error when reading pipe '{}', error {}", pipe->name(), Hex(error));
                        }

                        pipe->m_requestExit = 0;
                        pipe->close();
                    }
                    else
                    {
                        // enter waiting state
                        while (!pipe->m_requestExit.load() && !pipe->m_overlappedReadCompleted.load())
                        {
                            SleepEx(10, true);
                        }
                    }
                }

                TRACE_INFO("Closed pipe '{}' read thread", pipe->name());
                return 0;
            }

            //--

            WinPipeWriter::WinPipeWriter()
                : m_hPipe(NULL)
            {}

            WinPipeWriter::~WinPipeWriter()
            {
                close();

                ASSERT_EX(m_hPipe == NULL, "Write thread not closed");
            }

            WinPipeWriter* WinPipeWriter::Create()
            {
                // format proper pipe name
                char fullName[256] = { 0 };
                helper::CreateFullPipeName(nullptr, fullName);

                // create the pipe
                HANDLE hPipe = CreateNamedPipeA(fullName,
                    PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE,
                    PIPE_TYPE_BYTE | /*PIPE_NOWAIT | */PIPE_REJECT_REMOTE_CLIENTS,
                    2, 1 << 20, 1 << 20, 100, NULL);

                // no pipe
                if (NULL == hPipe)
                {
                    auto errorCode = GetLastError();
                    TRACE_ERROR("Unable to create named pipe '{}', error: {}", fullName, errorCode);
                    return nullptr;
                }

                // info
                TRACE_INFO("Created writing end of named pipe '{}'", fullName);

                // create pipeline
                WinPipeWriter* pipeline = MemNew(WinPipeWriter);
                pipeline->m_hPipe = hPipe;
                strcpy_s(pipeline->m_name, fullName);

                // return pipeline handle
                return pipeline;
            }

            WinPipeWriter* WinPipeWriter::Open(const char* pipeName)
            {
                // format proper pipe name
                char fullName[256] = { 0 };
                helper::CreateFullPipeName(pipeName, fullName);

                // open file
                HANDLE hPipe = CreateFileA(fullName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
                if (INVALID_HANDLE_VALUE == hPipe)
                {
                    auto errorCode = GetLastError();
                    TRACE_ERROR("Unable to open named pipe '{}', error: {}", fullName, errorCode);
                    return nullptr;
                }

                // info
                TRACE_INFO("Opened writing end of named pipe '{}'", fullName);

                // create pipeline
                WinPipeWriter* pipeline = MemNew(WinPipeWriter);
                pipeline->m_hPipe = hPipe;
                strcpy_s(pipeline->m_name, fullName);

                // return pipeline handle
                return pipeline;
            }

            void WinPipeWriter::close()
            {
                // close pipe
                if (NULL != m_hPipe)
                {
                    TRACE_INFO("Closed writing end of pipe '{}'", name());
                    CloseHandle(m_hPipe);
                    m_hPipe = NULL;
                }
            }

            const char* WinPipeWriter::name() const
            {
                return helper::SystemNameToPipeName(m_name);
            }

            bool WinPipeWriter::isOpened() const
            {
                return (NULL != m_hPipe);
            }

            uint32_t WinPipeWriter::write(const void* data, uint32_t size)
            {
                DWORD numBytesWritten = 0;
                if (NULL != m_hPipe)
                {
                    if (!WriteFile(m_hPipe, data, size, &numBytesWritten, NULL))
                    {
                        TRACE_ERROR("Failed to write {} bytes to pipe {}", size, name());
                        close();
                        return 0;
                    }
                    else
                    {
                        TRACE_INFO("Written {} bytes on pipe {}", numBytesWritten, m_name);
                    }
                }

                return numBytesWritten;
            }


        } // prv
    } // process
} // base
