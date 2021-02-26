/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\process\posix #]
* [#platform: posix #]
***/

#include "build.h"
#include "pipe.h"
#include "processPOSIX.h"

#include <pthread.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <locale>
#include <codecvt>
#include <cassert>
#include <base/process/include/process.h>

extern char **environ;

namespace base
{
    namespace process
    {
        namespace prv
        {
            //--

            POSIXProcess::POSIXProcess()
                : m_pid(0)
            {}

            POSIXProcess::~POSIXProcess()
            {
                terminate();

                ASSERT_EX(m_pid == 0, "Process not closed correctly");
            }

            bool POSIXProcess::wait(uint32_t timeoutMS)
            {
                return false;
            }

            void POSIXProcess::terminate()
            {
                m_pid = 0;
            }

            ProcessID POSIXProcess::id() const
            {
                return m_pid;
            }

            bool POSIXProcess::isRunning() const
            {
                if (m_pid == 0)
                    return false;

                int status = 0;
                if (0 == waitpid(m_pid, &status, WNOHANG))
                    return true;

                return !WIFEXITED(status);
            }

            bool POSIXProcess::exitCode(int& outExitCode) const
            {
                if (m_pid == 0)
                    return false;

                int status = 0;
                if (0 == waitpid(m_pid, &status, WNOHANG))
                    return false;

                if (!WIFEXITED(status))
                    return false;

                if (WIFSIGNALED(status))
                {
                    outExitCode = -200;
                    return true;
                }

                outExitCode = WEXITSTATUS(status);
                return true;
            }

            POSIXProcess* POSIXProcess::Create(const ProcessSetup& setup)
            {
                // prepare arguments
                std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
                auto processPath = converter.to_bytes(setup.m_processPath.c_str());

                // convert arguments
                std::vector<std::string> arguments;
                for (auto& arg : setup.m_arguments)
                {
                    auto argText = converter.to_bytes(arg.c_str());
                    arguments.push_back(argText);
                }

                // add endpoint name
                if (setup.m_messageEndpointName != nullptr)
                {
                    std::string arg = " -endpointName=\"";
                    arg += setup.m_messageEndpointName;
                    arg += "\" ";
                }

                // prepare the argv table
                std::vector<const char*> argv;
                argv.push_back(processPath.c_str());
                for (auto& arg : arguments)
                    argv.push_back(arg.c_str());
                argv.push_back(nullptr);

                // leave a trace in the log
                TRACE_INFO("Starting process: '{}'", processPath.c_str());
                for (auto& arg : arguments)
                    TRACE_INFO("Process argument: '{}'", arg.c_str());

                // spawn process
                pid_t pid;
                auto ret = posix_spawn(&pid, processPath.c_str(), NULL, NULL, (char**)argv.data(), environ);
                if (ret != 0)
                {
                    TRACE_ERROR("Failed to spawn process '{}'", setup.m_processPath);
                    return nullptr;
                }

                // leave a trace
                TRACE_INFO("Process started with PID {}", pid);

                // create wrapper
                auto wrapper = MemNew(POSIXProcess);
                wrapper->m_pid = pid;
                return wrapper;
            }

        } // prv
    } // process
} // base
