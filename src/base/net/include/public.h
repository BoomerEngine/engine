/***
* Boomer Engine v4
* Written by Lukasz "Krawiec" Krawczyk
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_net_glue.inl"

#include "base/system/include/timing.h"

#define RESPONSE_FUNC const base::http::RequestResult& result
#define REQUEST_FUNC const base::http::IncomingRequestPtr& request

BEGIN_BOOMER_NAMESPACE(base::net)

//--

struct Message;
typedef RefPtr<Message> MessagePtr;

class MessageReplicator;
class MessageKnowledgeBase;
class MessageObjectExecutor;

/// stats for peer connection
struct BASE_NET_API MessageStats
{
public:
    MessageStats();
    MessageStats(const MessageStats& other);
    MessageStats& operator=(const MessageStats& other);

    void reset();

    void countSentMessage(Type type, uint32_t dataSize);
    void countReceivedMessage(Type type, uint32_t dataSize);

    void print(IFormatStream& f) const;

private:
    struct PerMessageStats
    {
        Type m_type = nullptr;
        uint32_t m_numMessagesSent = 0;
        uint32_t m_numBytesSent = 0;
        uint32_t m_numMessagesReceived = 0;
        uint32_t m_numBytesReceived = 0;
    };

    uint32_t m_numTotalMessagesSent = 0;
    uint32_t m_numTotalMessagesReceived = 0;
    uint32_t m_numTotalBytesSent = 0;
    uint32_t m_numTotalBytesReceived = 0;

    HashMap<Type, PerMessageStats> m_typeStats;
};

//--

class MessageReassembler;
class IMessageReassemblerInspector;

class MessageConnection;
typedef RefPtr<MessageConnection> MessageConnectionPtr;

class TcpMessageServer;
typedef RefPtr<TcpMessageServer> TcpMessageServerPtr;

class TcpMessageClient;
typedef RefPtr<TcpMessageClient> TcpMessageClientPtr;

END_BOOMER_NAMESPACE(base::net)

//--

BEGIN_BOOMER_NAMESPACE(base::curl)

class RequestQueue;

END_BOOMER_NAMESPACE(base::curl)

//--

BEGIN_BOOMER_NAMESPACE(base::http)

class RequestArgs;
class RequestService;

enum class Method : uint8_t
{
    POST,
    GET,
    DEL,
};

class Connection;
typedef RefPtr<Connection> ConnectionPtr;

struct BASE_NET_API RequestResult
{
    uint32_t code;
    Buffer data; // may be string, may be not
    NativeTimePoint scheduleTime;
    NativeTimePoint completionTime;

    //--

    RequestResult();
    ~RequestResult();
};

class RequestServer;

class IRequestHandler;
typedef RefPtr<IRequestHandler> RequestHandlerPtr;

class IncomingRequest;
typedef RefPtr<IncomingRequest> IncomingRequestPtr;

typedef std::function<void(RESPONSE_FUNC)> TRequestResponseFunc;
typedef std::function<void(REQUEST_FUNC)> TRequestHandlerFunc;

END_BOOMER_NAMESPACE(base::http)

//--