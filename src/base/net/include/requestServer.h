/***
* Boomer Engine v4
* Written by Tomasz "RexDex" Jonarski
*
* [#filter: http #]
***/

#pragma once

#include "requestArguments.h"
#include "requestServerHeaders.h"

#include "base/socket/include/address.h"
#include "base/socket/include/tcpServer.h"

BEGIN_BOOMER_NAMESPACE(base::http)

//----

/// a simple "http" outgoing connection
class BASE_NET_API Connection : public IReferencable
{
public:
    Connection(StringView address, StringView protocol = "http");
    virtual ~Connection();

    // address we are connected to
    INLINE const StringBuf& address() const { return m_address; }

    // connection protocol
    INLINE const StringBuf& protocol() const { return m_protocol; }

    //--

    /// process a request and call a callback function once it's completed
    /// NOTE: this function is safe to call from multiple threads/fibers
    virtual void send(StringView url, const RequestArgs& params, const TRequestResponseFunc& service, Method method = Method::POST, uint32_t timeOut = INDEX_MAX) = 0;

    //--

    /// helper function: create a request and wait for it on current fiber
    RequestResult wait(StringView url, const RequestArgs& params, Method method = Method::POST, uint32_t timeOut = INDEX_MAX) CAN_YIELD;

protected:
    StringBuf m_address;
    StringBuf m_protocol;
};

//----

/// incoming request
class BASE_NET_API IncomingRequest : public IReferencable
{
public:
    IncomingRequest(const RefPtr<RequestHeader>& header);
    virtual ~IncomingRequest();

    //--

    INLINE const RequestHeader& header() const { return *m_header; }

    INLINE const RequestArgs& args() const { return m_args; } // parsed for POST/GET request

    //--

    /// finish request with a response, use 200 for OK, 500 for server error
    /// NOTE: this can be called only once
    /// NOTE: the request may die on it's own first with "TimeOut" so be quick!
    virtual void finish(uint32_t code, const Buffer& data, StringView contentType) = 0;

    /// finish with text response
    void finishText(uint32_t code, StringView txt = "");

    /// finish with text response
    void finishXML(uint32_t code, StringView txt = "");

private:
    RefPtr<RequestHeader> m_header;
    RequestArgs m_args;
};

//----

struct ActiveConnectionState;

/// a simple HTTP server for exchanging data between different apps or hosting local "debug" pages
class BASE_NET_API RequestServer : public IReferencable, public socket::tcp::IServerHandler
{
public:
    RequestServer();
    ~RequestServer();

    // server address
    INLINE const socket::Address& address() const { return m_address; }

    //--

    /// initialize HTTP server, can be initialized on specific port if request
    bool init(uint16_t specificPort = 0);

    //--

    /// register a request handler at a specific "path" in local server
    void registerHandler(StringView path, const TRequestHandlerFunc& handler);

    /// unregister previously registered handler
    void unregisterHandler(StringView path);

    //--

protected:
    socket::Address m_address;
    socket::tcp::Server m_server;

    //--

    HashMap<StringBuf, TRequestHandlerFunc*> m_handlerMap;
    SpinLock m_handlerMapLock;

    //--

    HashMap<socket::ConnectionID, RefPtr<ActiveConnectionState>> m_connections;
    SpinLock m_connectionsLock;

    virtual void handleConnectionAccepted(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection) override final;
    virtual void handleConnectionClosed(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection)  override final;
    virtual void handleConnectionData(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection, const void* data, uint32_t dataSize) override final;
    virtual void handleServerClose(socket::tcp::Server* server) override final;

    void serviceRequest(socket::tcp::Server* server, socket::ConnectionID connection, const RefPtr<RequestHeader>& header);
    void serviceMissingHandler(socket::tcp::Server* server, socket::ConnectionID connection, const RefPtr<RequestHeader>& header);
};

//----

END_BOOMER_NAMESPACE(base::http)