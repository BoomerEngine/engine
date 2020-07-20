/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\process #]
***/

#pragma once

#include "base/io/include/absolutePath.h"

namespace base
{
    namespace process
    {

        //-----------------------------------------------------------------------------

        /// Callback for processing process data
        /// Used for both pipe and stdout
        class BASE_PROCESS_API IOutputCallback
        {
        public:
            virtual ~IOutputCallback();

            virtual void processData(const void* data, uint32_t dataSize) = 0;
        };

        //-----------------------------------------------------------------------------

        /// SetupMetadata for running the process
        struct BASE_PROCESS_API ProcessSetup
        {
            // path to process to run (copied out of the pointer)
            // NOTE: if no process specified we will run ourselves
            io::AbsolutePath m_processPath;

            // optional arguments to pass to process
            Array<StringBuf> m_arguments;

            // asynchronous callback for the process communication via stdout
            // callback gets notified whenever we receive any data from the process via it's stdout
            // NOTE: if not specified than no std output will be observed
            IOutputCallback* m_stdOutCallback = nullptr;

            // show the window of the process or not
            bool m_showWindow = false;

            //--

            ProcessSetup();
        };

        //-----------------------------------------------------------------------------

        /// This is the abstraction of a running external process
        /// NOTE: this interface is fully asynchronous as most of the usages are like that
        class BASE_PROCESS_API IProcess : public base::NoCopy
        {
        public:
            virtual ~IProcess();

            //! Wait for the process to finish in given time (ms)
            //! returns false if process was not finished in given time
            virtual bool wait(uint32_t timeoutMS) = 0;

            //! Terminate the process, without waiting
            virtual void terminate() = 0;

            //! Get internal (system) ID of the thread
            virtual ProcessID id() const = 0;

            //! Are we running ?
            /// NOTE: false if the process has finished
            virtual bool isRunning() const = 0;

            //! Get the exit code of the process
            //! NOTE: returns false if process has not yet completed
            virtual bool exitCode(int& outExitCode) const = 0;

            //---

            //! Spawn a process
            static IProcess* Create(const ProcessSetup& setup);
        };

        //-----------------------------------------------------------------------------

    } // process
} // base