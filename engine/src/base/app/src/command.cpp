/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "command.h"
#include "commandline.h"

#include "base/replication/include/replicationRttiExtensions.h"
#include "base/net/include/messageConnection.h"
#include "base/net/include/messagePool.h"

namespace base
{
    namespace app
    {
        //--

        RTTI_BEGIN_TYPE_CLASS(CommandCancelMessage);
            RTTI_PROPERTY(reson).metadata<replication::SetupMetadata>("maxLength:100");
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(CommandLastStatusQueryMessage);
            RTTI_PROPERTY(lastReceivedTimestamp).metadata<replication::SetupMetadata>();
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(CommandLastStatusResponseMessage);
            RTTI_PROPERTY(timeStamp).metadata<replication::SetupMetadata>();
            RTTI_PROPERTY(progress).metadata<replication::SetupMetadata>();
            RTTI_PROPERTY(total).metadata<replication::SetupMetadata>();
            RTTI_PROPERTY(status).metadata<replication::SetupMetadata>("maxLength:200");
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(CommandHelloMessage);
            RTTI_PROPERTY(startupTimestamp).metadata<replication::SetupMetadata>();
            RTTI_PROPERTY(localTimestamp).metadata<replication::SetupMetadata>();
            RTTI_PROPERTY(connectionKey).metadata<replication::SetupMetadata>("maxLength:32");
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(CommandHelloResponseMessage);
            RTTI_PROPERTY(timestampSentBack).metadata<replication::SetupMetadata>();
        RTTI_END_TYPE();        

        //--

        RTTI_BEGIN_TYPE_CLASS(CommandNameMetadata);
        RTTI_END_TYPE();

        CommandNameMetadata::CommandNameMetadata()
        {}

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICommand);
            RTTI_FUNCTION_SIMPLE(handleCancelMessage);
            RTTI_FUNCTION_SIMPLE(handleLastStatusQueryMessage);
            RTTI_FUNCTION_SIMPLE(handleHelloResponseMessage);
        RTTI_END_TYPE();

        //--

        ICommand::ICommand()
        {}

        ICommand::~ICommand()
        {}

        void ICommand::queryStatus(CommandLastStatusResponseMessage& outStatus) const
        {
            auto lock = CreateLock(m_lastStatusLock);
            outStatus = m_lastStatus;
        }

        void ICommand::requestCancel()
        {
            if (0 == m_cancelRequest.exchange(1))
            {
                TRACE_WARNING("Command: Command '{}' canceled locally", *this);
            }
        }

        bool ICommand::checkCancelation() const
        {
            return m_cancelRequest.load() != 0;
        }

        void ICommand::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
        {
            auto lock = CreateLock(m_lastStatusLock);
            m_lastStatus.timeStamp = NativeTimePoint::Now().rawValue();
            m_lastStatus.progress = currentCount;
            m_lastStatus.total = totalCount;
            m_lastStatus.status = StringBuf(text);
        }

        void ICommand::processMessage(net::Message* message, net::MessageConnection* connection)
        {
            TRACE_INFO("Command: Received network message '{}'", message->type());
            message->dispatch(this, connection);
        }

        //--

        void ICommand::handleCancelMessage(const CommandCancelMessage& msg)
        {
            if (0 == m_cancelRequest.exchange(1))
            {
                TRACE_WARNING("Command: Command '{}' cancel by remote request", *this);
            }
        }

        void ICommand::handleLastStatusQueryMessage(const CommandLastStatusQueryMessage& msg, const base::net::MessageConnectionPtr& connection)
        {
            CommandLastStatusResponseMessage status;
            queryStatus(status);
            connection->send(status);
        }

        void ICommand::handleHelloResponseMessage(const CommandHelloResponseMessage& msg, const net::MessageConnectionPtr& connection)
        {
            auto diff = NativeTimePoint(msg.timestampSentBack).timeTillNow().toSeconds();
            TRACE_INFO("Command: Ping to message server: {}", TimeInterval(diff));
        }

        //--

    } // app
} // base