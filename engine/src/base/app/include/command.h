/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/system/include/timing.h"
#include "base/io/include/absolutePath.h"
#include "localServiceContainer.h"

namespace base
{
    namespace app
    {
        //--

        // cancel message that can be send to command to indicate that we wish to cancel it 
        // NOTE: this is used only in the commands run in background processes
        struct BASE_APP_API CommandCancelMessage
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(CommandCancelMessage);

            StringBuf reson;
        };

        // simple command hello message
        struct BASE_APP_API CommandHelloMessage
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(CommandHelloMessage);

            uint64_t startupTimestamp = 0;
            uint64_t localTimestamp = 0;
            StringBuf connectionKey;
        };

        // response to hello message sent by server
        struct BASE_APP_API CommandHelloResponseMessage
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(CommandHelloResponseMessage);

            uint64_t timestampSentBack;
        };

        // a query for last command status/progress
        struct BASE_APP_API CommandLastStatusQueryMessage
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(CommandLastStatusQueryMessage);

            uint64_t lastReceivedTimestamp;
        };

        // a response for the request to get status/progress
        struct BASE_APP_API CommandLastStatusResponseMessage
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(CommandLastStatusResponseMessage);

            uint64_t timeStamp = 0; // timestamp of the message, to be sent back in the next query
            uint64_t progress = 0;
            uint64_t total = 0;
            StringBuf status;
        };

        //--

        // the application "command" that can be run by the BCC (Boomer Content Compiler)
        // the typical role of a Command is to do some processing and exit, there's no notion of a loop
        // NOTE: command is ALWAYS run on a separate fiber, never on main thread
        class BASE_APP_API ICommand : public IObject, public IProgressTracker
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ICommand, IObject);

        public:
            ICommand();
            virtual ~ICommand();

            //--

            /// query command status
            void queryStatus(CommandLastStatusResponseMessage& outStatus) const;

            /// request command to be canceled
            void requestCancel();

            //--

            /// called after all local services were initialized and is expected to do the work
            virtual bool run(base::net::MessageConnectionPtr connection, const base::app::CommandLine& commandline) = 0;

            //--

            // do we want to cancel this command ?
            virtual bool checkCancelation() const override;

            /// post status update, will replace the previous status update
            /// this status will be sent next time somebody asks us about it
            /// NOTE: on windows the last status is also displayed on the Console Window title bar
            virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text);

            //--

            /// process (dispatch) a messages received from the spawner of this command
            /// NOTE: this function is called from "outside", ie. it's asynchronous to the command execution, care must be taken not to break things processing messages
            virtual void processMessage(net::Message* message, net::MessageConnection* connection);

        private:
            void handleCancelMessage(const CommandCancelMessage& msg);
            void handleLastStatusQueryMessage(const CommandLastStatusQueryMessage& msg, const base::net::MessageConnectionPtr& connection);
            void handleHelloResponseMessage(const CommandHelloResponseMessage& msg, const net::MessageConnectionPtr& connection);

            SpinLock m_lastStatusLock;
            CommandLastStatusResponseMessage m_lastStatus;

            std::atomic<uint32_t> m_cancelRequest = 0;
        };

        //--

        // metadata with name of the command 
        class BASE_APP_API CommandNameMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CommandNameMetadata, rtti::IMetadata);

        public:
            CommandNameMetadata();

            INLINE base::StringView<char> name() const { return m_name; }

            INLINE void name(base::StringView<char> name) { m_name = name; }

        private:
            base::StringView<char> m_name;
        };

        //--

    } // app
} // base
