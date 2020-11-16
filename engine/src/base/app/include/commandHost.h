/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/system/include/timing.h"
#include "localServiceContainer.h"

namespace base
{
    namespace app
    {
        //--

        /// wrapper that hosts a command that is executing in the background (it's own fiber)
        class BASE_APP_API CommandHost : public IReferencable
        {
        public:
            CommandHost();
            virtual ~CommandHost(); // will wait for the command to finish, there's no other way

            //--

            // is the command still running ?
            INLINE bool running() const { return m_command; }

            //--

            /// start command - by convention parameters are passed as a "commandline" object
            bool start(const CommandLine& cmdLine);

            /// request command to stop
            void cancel();

            /// update command state, dispatch messages (should be called periodically)
            /// NOTE: this function will return false once we are done
            bool update();

            //--

        private:
            // connection to message server (used in remote commands)
            base::net::TcpMessageClientPtr m_connection;

            // command itself
            base::app::CommandPtr m_command;

            // signal that is flagged once command finishes
            base::fibers::WaitCounter m_finishedSignal;
            std::atomic_bool m_finishedFlag = false;

            //--
        };

        //--

    } // app
} // base
