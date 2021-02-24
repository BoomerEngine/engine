/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#include "build.h"
#include "messageReplicator.h"
#include "messageKnowledgeBase.h"
#include "messageKnowledgeSync.h"
#include "messagePool.h"

#include "base/socket/include/blockBuilder.h"
#include "base/replication/include/replicationBitReader.h"
#include "base/replication/include/replicationBitWriter.h"
#include "base/replication/include/replicationDataModel.h"
#include "base/replication/include/replicationDataModelRepository.h"

BEGIN_BOOMER_NAMESPACE(base::net)

//--

IMessageReplicatorDataSink::~IMessageReplicatorDataSink()
{}

//--

IMessageReplicatorDispatcher::~IMessageReplicatorDispatcher()
{}

//--

enum class ReplicatorMessageType : uint8_t
{
    UpdateString = 0xAB,
    UpdatePath = 0xCC,
    CallFunction = 0x42,
};

struct StringUpdateHeader
{
    ReplicatorMessageType m_type = ReplicatorMessageType::UpdateString;
    replication::DataMappedID m_id;
    // text data follows up to the size of the message
};

struct PathUpdateHeader
{
    ReplicatorMessageType m_type = ReplicatorMessageType::UpdatePath;;
    replication::DataMappedID m_id;
    replication::DataMappedID m_textId;
    replication::DataMappedID m_parentId;
};

struct CallHeader
{
    ReplicatorMessageType m_type = ReplicatorMessageType::CallFunction;;
    replication::DataMappedID m_messageTypeId;
    // serialized data follows
};

//--

MessageStats::MessageStats()
{}

MessageStats::MessageStats(const MessageStats& other) = default;
MessageStats& MessageStats::operator=(const MessageStats& other) = default;

void MessageStats::reset()
{
    *this = MessageStats();
}

void MessageStats::countSentMessage(Type type, uint32_t dataSize)
{
    ASSERT(type != nullptr);
    auto& entry = m_typeStats[type];
    entry.m_type = type;
    entry.m_numBytesSent = dataSize;
    entry.m_numMessagesSent += 1;
    m_numTotalMessagesSent += 1;
    m_numTotalBytesSent += dataSize;
}

void MessageStats::countReceivedMessage(Type type, uint32_t dataSize)
{
    ASSERT(type != nullptr);
    auto& entry = m_typeStats[type];
    entry.m_type = type;
    entry.m_numBytesReceived = dataSize;
    entry.m_numMessagesReceived += 1;
    m_numTotalMessagesReceived += 1;
    m_numTotalBytesReceived += dataSize;
}

void MessageStats::print(IFormatStream& f) const
{
    if (m_numTotalMessagesSent > 0)
    {
        f.appendf("  Messages SENT {} ({}):\n", m_numTotalMessagesSent, MemSize(m_numTotalBytesSent));
        for (auto &entry : m_typeStats.values())
            if (entry.m_numMessagesSent > 0)
                f.appendf("    {}: {} ({})\n", entry.m_type->name(), entry.m_numMessagesSent, MemSize(entry.m_numBytesSent));
    }

    if (m_numTotalMessagesReceived > 0)
    {
        f.appendf("  Messages RECV {} ({}):\n", m_numTotalMessagesReceived, MemSize(m_numTotalBytesReceived));
        for (auto &entry : m_typeStats.values())
            if (entry.m_numMessagesReceived > 0)
                f.appendf("    {}: {} ({})\n", entry.m_type->name(), entry.m_numMessagesReceived, MemSize(entry.m_numBytesReceived));
    }
}

//--

// sends knowledge update as messages
class KnowledgeUpdateDataSinkByMessage : public IKnowledgeUpdaterSink
{
public:
    KnowledgeUpdateDataSinkByMessage(IMessageReplicatorDataSink* sink)
        : m_sink(sink)
    {}

    virtual void reportNewString(const replication::DataMappedID id, StringView txt) override final
    {
        socket::BlockBuilder msg;

        StringUpdateHeader header;
        header.m_id = id;
        msg.write(&header, sizeof(header));
        msg.write(txt.data(), txt.length());

        m_sink->sendMessage(msg.data(), msg.size());
    }

    virtual void reportNewPath(const replication::DataMappedID id, const replication::DataMappedID textId, const replication::DataMappedID parentPathId) override final
    {
        PathUpdateHeader header;
        header.m_id = id;
        header.m_textId = textId;
        header.m_parentId = parentPathId;

        m_sink->sendMessage(&header, sizeof(header));
    }

private:
    IMessageReplicatorDataSink* m_sink;
};

//--

MessageReplicator::MessageReplicator( const replication::DataModelRepositoryPtr& sharedModelRepository)
    : m_models(sharedModelRepository)
{
    // create local knowledge base to map data between remote and local systems
    // NOTE: we have separate stores bo avoid races between what was defined and what was not defined
    // TODO: move it outside ?
    m_incomingKnowledge.create();
    m_outgoingKnowledge.create();
}

MessageReplicator::~MessageReplicator()
{
    TRACE_INFO("Connection message stats:\n{}", m_stats);
}

void MessageReplicator::send(const void* data, Type dataType, IMessageReplicatorDataSink* dataSink)
{
    ASSERT(data != nullptr);
    ASSERT(dataType != nullptr);

    // generate data model for packing the message's data
    auto dataModel  = m_models->buildModelForType(dataType);
    if (!dataModel)
    {
        TRACE_ERROR("Sending message '{}' not possible because model was not built", dataType->name());
        return;
    }

    // protect the outgoing DB from access from multiple threads
    // NOTE: this could be relaxed to allow encoding and packing of messages from multiple threads but there's a risk of generating OOO update messages
    auto lock  = CreateLock(m_lock);

    // map type of the message we are sending, capture updates and transform them into messages
    KnowledgeUpdateDataSinkByMessage knowledgeUpdateToMessage(dataSink);
    KnowledgeUpdater knowledgeUpdater(*m_outgoingKnowledge, &knowledgeUpdateToMessage);
    auto messageTypeId  = knowledgeUpdater.mapTypeRef(dataType);
    ASSERT(messageTypeId != 0);

    // serialize the message to bit writer, this will update our outgoing knowledge that we will send directly
    // TODO: consider a proxy message sink class so we don't have to touch the TCP directly
    replication::BitWriter bitWriter;
    dataModel->encodeFromNativeData(data, knowledgeUpdater, bitWriter);

    // build and send packet
    {
        // make a packet
        socket::BlockBuilder msg;

        // write data header
        CallHeader header;
        header.m_messageTypeId = messageTypeId;
        msg.write(&header, sizeof(header));
        msg.write(bitWriter.data(), bitWriter.byteSize());

        // push to sink for actual sending
        dataSink->sendMessage(msg.data(), msg.size());

        // update internal stats
        auto lock  = CreateLock(m_statsLock);
        m_stats.countSentMessage(dataType, msg.size());
    }
}

bool MessageReplicator::reportDataError(StringView txt)
{
    // TODO: silent ?
    TRACE_ERROR("MessageReplicator: error: {}", txt);
    return false;
}

bool MessageReplicator::processMessageData(const void* data, uint32_t dataSize, IMessageReplicatorDispatcher* dispatcher)
{
    // empty shit ?
    if (!data || dataSize < 1)
        return reportDataError("Empty data packet");

    // lock access
    auto lock  = CreateLock(m_lock);

    // get message type
    auto dataPtr  = (const uint8_t*) data;
    auto messageType  = (ReplicatorMessageType)dataPtr[0];
    switch (messageType)
    {
        case ReplicatorMessageType::UpdateString:
        {
            auto header  = (StringUpdateHeader*)data;
            if (dataSize < sizeof(StringUpdateHeader))
                return reportDataError("String update packet with invalid header");

            auto stringLength  = dataSize - sizeof(StringUpdateHeader);
            if (stringLength == 0)
                return reportDataError("String update packet with no string");

            auto text  = StringView((const char*)data + sizeof(StringUpdateHeader), stringLength);
            if (!m_incomingKnowledge->rememberString(header->m_id, text))
                return reportDataError("String update packet data collision");

            break;
        }

        case ReplicatorMessageType::UpdatePath:
        {
            auto header  = (PathUpdateHeader*)data;
            if (dataSize < sizeof(PathUpdateHeader))
                return reportDataError("Path update packet with invalid header");

            if (!m_incomingKnowledge->validStringId(header->m_textId))
                return reportDataError("Path update packet uses invalid string ID");

            if (!m_incomingKnowledge->validPathId(header->m_parentId))
                return reportDataError("Path update packet uses invalid parent path ID");

            if (!m_incomingKnowledge->rememberPathPart(header->m_id, header->m_textId, header->m_parentId))
                return reportDataError("Path update packet data collision");

            break;
        }

        case ReplicatorMessageType::CallFunction:
        {
            auto header  = (CallHeader*)data;
            if (dataSize < sizeof(CallHeader))
                return reportDataError("Call packet with invalid header");

            if (!m_incomingKnowledge->validPathId(header->m_messageTypeId))
                return reportDataError("Call packet with invalid type");

            auto callDataSize  = dataSize - sizeof(CallHeader); // NOTE: zero data size is a valid thing - sometimes the message itself is enough data
            auto callData  = dataPtr + sizeof(CallHeader);
            if (!processCall(header, callData, callDataSize, dispatcher))
                return reportDataError("Invalid call");

            break;
        }

        default:
            return reportDataError("Invalid message");
    }

    return true;
}

bool MessageReplicator::processCall(const CallHeader* call, const void* data, uint32_t dataSize, IMessageReplicatorDispatcher* dispatcher)
{
    // resolve the name of the type to call
    BaseTempString<200> typeName;
    if (!m_incomingKnowledge->resolvePath(call->m_messageTypeId, "::", typeName))
        return reportDataError("Call type not registered first");

    // find the type
    auto dataType  = RTTI::GetInstance().findType(StringID::Find(typeName));
    if (!dataType)
        return reportDataError(TempString("Call type '{}' not found in local RTTI", typeName.c_str()));

    // find the data model
    auto dataModel  = m_models->buildModelForType(dataType);
    if (!dataModel)
        return reportDataError(TempString("Call type '{}' uses invalid data model", typeName.c_str()));

    // allocate message structure
    auto message  = Message::AllocateFromPool(dataType);
    if (!message)
        return true; // message ignored

    // deserialize data
    replication::BitReader bitReader(data, dataSize * 8);
    KnowledgeResolver resolver(*m_incomingKnowledge);
    if (!dataModel->decodeToNativeData(message->payload(), resolver, bitReader))
        return reportDataError(TempString("Call '{}' data decoding error", typeName.c_str()));

    // count stats
    {
        auto lock = CreateLock(m_statsLock);
        m_stats.countReceivedMessage(dataType, dataSize);
    }

    // dispatch message
    dispatcher->dispatchMessageForExecution(message);
    return true;
}


//--

END_BOOMER_NAMESPACE(base::net)