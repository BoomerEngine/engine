/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\posix #]
* [# platform: posix #]
***/

#include "build.h"

#include "launcherPlatformCommon.h"
#include "core/app/include/commandline.h"
#include "core/io/include/absolutePathBuilder.h"
#include "core/io/include/absolutePath.h"

#include "posixPlatform.h"
#include "posixOutput.h"

#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include <ucontext.h>
#include <execinfo.h>

namespace base
{
    namespace platform
    {
        namespace posix
        {

            Platform::Platform()
                : m_hasLog(false)
                , m_hasErrors(false)
                , m_output(nullptr)
            {}

            Platform::~Platform()
            {
                if (m_hasLog && m_output)
                    log::ILogStream::DetachGlobalSink(m_output.get());

                if (m_hasErrors)
                    log::IErrorHandler::BindListener(nullptr);
            }

            ExitCode Platform::start(const CommandLine& cmdline)
            {
                // register output handlers
                m_hasLog = !cmdline.hasParam("silent") && !cmdline.hasParam("nolog");
                m_hasErrors = !cmdline.hasParam("silent") && !cmdline.hasParam("noasserts");
                bool console = cmdline.hasParam("console");

                // enable/disable the long-form log line header
                bool useLongLogLineHeader = !console;
                if (cmdline.hasParam("logheader"))
                    useLongLogLineHeader = true;
                else if (cmdline.hasParam("nologheader"))
                    useLongLogLineHeader = false;
                log::ILogStream::EnableLineHeader(useLongLogLineHeader);

                // create the output handler
                m_output = CreateUniquePtr<GenericOutput>();

                // register the output handler
                if (m_hasLog)
                    log::ILogStream::AttachGlobalSink(m_output.get());

                // register the error handler
                if (m_hasErrors)
                    log::IErrorHandler::BindListener(m_output.get());

                // enter the protected region
                return protectedStart(cmdline);             
            }

            ExitCode Platform::protectedStart(const CommandLine& cmdline)
            {
                installSignalHandlers();

                return CommonPlatform::start(cmdline);
            }

            void Platform::cleanup()
            {
                // unregister the stuff
                if (m_hasLog && m_output)
                    log::ILogStream::DetachGlobalSink(m_output.get());

                if (m_hasErrors)
                    log::IErrorHandler::BindListener(nullptr);

                // delete the handler
                m_output.reset();

                // pass to base class
                CommonPlatform::cleanup();
            }

            bool Platform::update()
            {
                return CommonPlatform::update();
            }

            //--

            typedef struct _sig_ucontext {
                unsigned long     uc_flags;
                struct ucontext   *uc_link;
                stack_t           uc_stack;
                struct sigcontext uc_mcontext;
                sigset_t          uc_sigmask;
            } sig_ucontext_t;

            void Platform::SignalHandler(int sig_num, void* info, void * ucontext)
            {
                // print error message
                auto uc  = (sig_ucontext_t *)ucontext;
                auto caller_address  = (void *) uc->uc_mcontext.rip;
                fprintf(stderr, "Error: signal %d (%s), address is %p from %p\n", sig_num, strsignal(sig_num), ((siginfo_t*)info)->si_addr, (void*)caller_address);

                // print callstack
                void* array[50];
                int size = backtrace(array, 50);
                array[1] = caller_address; // overwrite sigaction with caller's address

                backtrace_symbols_fd(array, size, 2);

                // exit the application
                exit(-1);
            }

            void Platform::PipeHandler(int sig_num)
            {
                fprintf(stderr, "Caught SIGPIPE\n");
            }

            typedef void (*TSigFunction) (int, siginfo_t *, void *);

            void Platform::installSignalHandlers()
            {
                signal(SIGPIPE, &PipeHandler);

                struct sigaction sigact;
                sigact.sa_sigaction = (TSigFunction) &SignalHandler;
                sigact.sa_flags = SA_RESTART | SA_SIGINFO;

                sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL);
                sigaction(SIGBUS, &sigact, (struct sigaction *)NULL);
                sigaction(SIGABRT, &sigact, (struct sigaction *)NULL);
            }

        } // posix

        bool Platform::HasDebuggerAttached()
        {
            return false;
        }

    } // platform
} // base
