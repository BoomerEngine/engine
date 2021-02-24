/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#pragma once

#include "base/object/include/object.h"
#include "base/system/include/thread.h"
#include "base/system/include/spinLock.h"

BEGIN_BOOMER_NAMESPACE(base::net)

//--

struct CallHeader;

//--

/// message sink for channel, where's where the data is "sent"
class BASE_NET_API IMessageReplicatorDataSink : public NoCopy
{
public:
    virtual ~IMessageReplicatorDataSink();

    /// sink data, we assume that we are the only sender thus we may see fragmented calls instead of going "atomic"
    virtual void sendMessage(const void* data, uint32_t size) = 0;
};

//--

/// message dispatcher, routes message to execute to actual target
class BASE_NET_API IMessageReplicatorDispatcher : public NoCopy
{
public:
    virtual ~IMessageReplicatorDispatcher();

    /// dispatch given message for execution
    virtual void dispatchMessageForExecution(Message* message) = 0;
};

//--

/// message replicator, handles serialization/deserialization/dispatch of messages on top of abstract transport layer
/// usually this thing sits close to the actual data channel since the output of this class is message oriented
/// NOTE:
class BASE_NET_API MessageReplicator : public NoCopy
{
public:
    MessageReplicator(const replication::DataModelRepositoryPtr& sharedModelRepository);
    ~MessageReplicator();

    //--

    // send message to the other end, message should be a structure with proper replication setup for fields
    void send(const void* data, Type dataType, IMessageReplicatorDataSink* dataSink);

    //--

    // process received message (actual messages not byte stream) and pass decoded data to the dispatcher
    // NOTE: we are not error tolerant here and we expect that the message we got is well formatted
    // NOTE: not every call to this function will dispatch something since not every message is actual call
    bool processMessageData(const void* data, uint32_t dataSize, IMessageReplicatorDispatcher* dispatcher);

private:
    //--

    Mutex m_lock;

    UniquePtr<MessageKnowledgeBase> m_incomingKnowledge;
    UniquePtr<MessageKnowledgeBase> m_outgoingKnowledge;

    replication::DataModelRepositoryPtr m_models;

    bool m_corruption;

    SpinLock m_statsLock;
    MessageStats m_stats;

    //--

    bool reportDataError(StringView txt);
    bool processCall(const CallHeader* call, const void* data, uint32_t dataSize, IMessageReplicatorDispatcher* dispatcher);
};

//--

END_BOOMER_NAMESPACE(base::net)
