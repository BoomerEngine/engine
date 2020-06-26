/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\pipe\posix #]
* [#platform: posix #]
***/

#include "build.h"
#include "process.h"
#include "base/system/include/thread.h"
#include "pipePOSIX.h"
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <fcntl.h>

namespace base
{
    namespace process
    {
        namespace prv
        {

            //--

            namespace helper
            {
                static void GetSystemPipeName(const char *pipeName, char *fullName)
                {
                    // compose a pipe name
                    if (!pipeName)
                    {
                        uuid_t id;
                        uuid_generate_random(id);

                        strcpy_s(fullName, "/tmp/");
                        uuid_unparse_lower(id, fullName + 5);
                    }
                    else
                    {
                        sprintf(fullName, "/tmp/%s", pipeName);
                    }
                }

                static const char* GetEnginePipeName(const char* systemPipeName)
                {
                    ASSERT(0 == strncmp(systemPipeName, "/tmp/", 5));
                    return systemPipeName + 5;
                }
            }

            //--

            POSIXPipeWriter::POSIXPipeWriter()
                : m_handle(-1)
            {}

            POSIXPipeWriter::~POSIXPipeWriter()
            {
                close();

                ASSERT_EX(m_handle == -1, "Pipe not closed properly");
            }

            POSIXPipeWriter* POSIXPipeWriter::Create()
            {
                // get the system name for the pipe
                char fullName[128];
                helper::GetSystemPipeName(nullptr, fullName);

                // open the fifo
                if (0 != mkfifo(fullName, 0777))
                {
                    TRACE_ERROR("Failed to create pipe '{}': {}", fullName, errno);
                    return nullptr;
                }

                // open the pipe for writing
                auto handle = open(fullName, O_RDWR | O_NONBLOCK);
                if (handle == -1)
                {
                    TRACE_ERROR("Unable to open pipe '{}' for writing: {}", fullName, errno)
                    return nullptr;
                }

                // create
                auto ret  = MemNew(POSIXPipeWriter);
                ret->m_handle = handle;
                strcpy_s(ret->m_name, fullName);

                // return
                TRACE_INFO("Created writing end of pipe '{}'", ret->name());
                return ret;
            }

            POSIXPipeWriter* POSIXPipeWriter::Open(const char* pipeName)
            {
                char fullName[128];
                helper::GetSystemPipeName(pipeName, fullName);

                // open the pipe for writing
                auto handle = open(fullName, O_WRONLY | O_NONBLOCK);
                if (handle == -1)
                {
                    TRACE_ERROR("Unable to open pipe '{}' for writing: {}", fullName, errno)
                    return nullptr;
                }

                // create
                auto ret  = MemNew( POSIXPipeWriter);
                ret->m_handle = handle;
                strcpy_s(ret->m_name, fullName);

                // return
                TRACE_INFO("Opened writing end of pipe '{}'", ret->name());
                return ret;
            }

            void POSIXPipeWriter::close()
            {
                if (m_handle != -1)
                {
                    TRACE_INFO("Closed writing end of pipe '{}'", name());
                    ::close(m_handle);
                    m_handle = -1;
                }
            }

            const char* POSIXPipeWriter::name() const
            {
                return helper::GetEnginePipeName(m_name);
            }

            bool POSIXPipeWriter::isOpened() const
            {
                return (m_handle != -1);
            }

            uint32_t POSIXPipeWriter::write(const void* data, uint32_t size)
            {
                if (!size)
                    return 0;

                if (m_handle == -1)
                    return 0;

                auto ret = ::write(m_handle, data, size);
                if (ret == -1)
                {
                    TRACE_ERROR("Writing end of pipe '{}' lost", name());
                    return 0;
                }

                return ret;
            }

            //--

            POSIXPipeReader::POSIXPipeReader()
                : m_handle(-1)
                , m_readThread(0)
                , m_requestExit(false)
                , m_callback(nullptr)
            {}

            POSIXPipeReader::~POSIXPipeReader()
            {
                close();

                ASSERT_EX(m_handle == -1, "Pipe not closed properly");
                ASSERT_EX(m_readThread == 0, "Pipe not closed properly");
            }

            POSIXPipeReader* POSIXPipeReader::Create(IOutputCallback* callback)
            {
                // get the system name for the pipe
                char fullName[128];
                helper::GetSystemPipeName(nullptr, fullName);

                // open the fifo
                if (0 != mkfifo(fullName, 0666))
                {
                    TRACE_ERROR("Failed to create pipe '{}': {}", fullName, errno);
                    return nullptr;
                }

                // open the pipe for writing
                return OpenNative(fullName, callback);
            }

            POSIXPipeReader* POSIXPipeReader::Open(const char* pipeName, IOutputCallback* callback)
            {
                if (!pipeName || !*pipeName)
                {
                    TRACE_ERROR("Invalid pipe name specified");
                    return nullptr;
                }

                char fullName[128];
                helper::GetSystemPipeName(pipeName, fullName);
                return OpenNative(fullName, callback);
            }

            POSIXPipeReader* POSIXPipeReader::OpenNative(const char* fullName, IOutputCallback* callback)
            {
                // open the pipe for writing
                auto handle = open(fullName, O_RDONLY | O_NONBLOCK);
                if (handle == -1)
                {
                    TRACE_ERROR("Unable to open pipe '{}' for reading: {}", fullName, errno)
                    return nullptr;
                }

                // create wrapper
                auto ret  = MemNew(POSIXPipeReader);
                ret->m_handle = handle;
                ret->m_requestExit = false;
                strcpy_s(ret->m_name, fullName);

                // start reading thread
                if (callback != nullptr)
                {
                    ret->m_callback = callback;

                    // create thread
                    pthread_attr_t attrs;
                    pthread_attr_init(&attrs);
                    pthread_attr_setstacksize(&attrs, 4096);
                    pthread_create(&ret->m_readThread, &attrs, &POSIXPipeReader::ThreadFunc, ret);
                    pthread_attr_destroy(&attrs);

                    // failed to create thread ?
                    if (0 == ret->m_readThread)
                    {
                        TRACE_ERROR("Unable to create reading thread for pipe '{}': {}", fullName, errno)
                        delete ret;
                        return nullptr;
                    }

                    // name the thread
                    pthread_setname_np(ret->m_readThread, "PipeReaderThread");
                }

                TRACE_INFO("Opened reading end of pipe '{}'", ret->name());
                return ret;
            }

            void POSIXPipeReader::close()
            {
                if (m_handle != -1)
                {
                    // request exit
                    m_requestExit = true;

                    // close the handle
                    TRACE_INFO("Closed reading end of pipe '{}'", name());
                    ::close(m_handle);
                    m_handle = -1;

                    // wait for thread
                    if (m_readThread != 0)
                    {
                        pthread_join(m_readThread, nullptr);
                        m_readThread = 0;
                    }
                }
            }

            const char* POSIXPipeReader::name() const
            {
                return helper::GetEnginePipeName(m_name);
            }

            bool POSIXPipeReader::isOpened() const
            {
                return (m_handle != -1);
            }

            uint32_t POSIXPipeReader::read(void* data, uint32_t size)
            {
                // we have a thread to read
                if (m_callback)
                    return 0;

                // lost
                if (m_handle == -1)
                    return 0;

                // read
                auto ret = ::read(m_handle, data, size);
                if (ret == -1)
                {
                    if (errno == EAGAIN)
                        return 0;

                    TRACE_ERROR("Unable to read content from pipe '{}': {}", name(), errno)
                    ::close(m_handle);
                    m_handle = -1;
                    return 0;
                }

                // return number of bytes read
                return ret;
            }

            void* POSIXPipeReader::ThreadFunc(void* param)
            {
                auto pipe  = (POSIXPipeReader*)param;

                std::vector<uint8_t> asyncBuffer;
                asyncBuffer.resize(512 * 1024);

                while (!pipe->m_requestExit)
                {
                    // wait for data
                    auto ret = ::read(pipe->m_handle, asyncBuffer.data(), asyncBuffer.size());
                    if (ret == -1)
                    {
                        if (errno == EAGAIN)
                            continue;

                        TRACE_WARNING("Pipe '{}' abruptly closed: {}", pipe->name(), errno);
                        break;
                    }

                    // send to callback
                    if (ret > 0)
                        pipe->m_callback->processData(asyncBuffer.data(), ret);
                }

                return nullptr;
            }

            //--

        } // prv
    } // process
} // base
