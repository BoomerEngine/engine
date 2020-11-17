/***
* Boomer Engine v4
* Written by Tomasz "RexDex" Jonarski
*
* [#filter: http #]
***/

#include "build.h"
#include "requestServer.h"
#include "requestServerHeaders.h"

namespace base
{
    namespace http
    {

        //---

        RequestResult::RequestResult()
            : code(0)
        {}

        RequestResult::~RequestResult()
        {}

        //---

        Connection::Connection(StringView address, StringView protocol)
            : m_address(address)
            , m_protocol(protocol)
        {}

        Connection::~Connection()
        {}

        RequestResult Connection::wait(StringView url, const RequestArgs& params, Method method, uint32_t timeOut) CAN_YIELD
        {
            RequestResult res;

            auto wait = Fibers::GetInstance().createCounter("HTTPOutgoingRequest");
            send(url, params, [wait, &res](const RequestResult& result)
                {
                    res = result;
                    Fibers::GetInstance().signalCounter(wait);
                }, method, timeOut);

            Fibers::GetInstance().waitForCounterAndRelease(wait);
            return res;
        }

        //---

        IncomingRequest::IncomingRequest(const RefPtr<RequestHeader>& header)
            : m_header(header)
        {
            if (header->method == "GET")
            {
                auto extraArguments  = header->url.view().afterFirst("?");
                TRACE_INFO("URL GET arguments: '{}'", extraArguments);
                if (!RequestArgs::Parse(extraArguments, m_args))
                {
                    TRACE_WARNING("HttpRequest: Failed to parse URL args from GET '{}'", extraArguments);
                }
                else
                {
                    TRACE_INFO("Parsed args: '{}'", m_args);
                }
            }
            else if (header->method == "POST" && header->contentType == "application/x-www-form-urlencoded")
            {
                if (header->contentBuffer)
                {
                    auto extraArguments  = StringView((char*)header->contentBuffer.data(), header->contentBuffer.size());
                    TRACE_INFO("URL POST arguments: '{}'", extraArguments);
                    if (!RequestArgs::Parse(extraArguments, m_args))
                    {
                        TRACE_WARNING("HttpRequest: Failed to parse URL args from POST '{}'", extraArguments);
                    }
                    else
                    {
                        TRACE_INFO("Parsed args: '{}'", m_args);
                    }
                }
            }
        }

        IncomingRequest::~IncomingRequest()
        {}

        void IncomingRequest::finishText(uint32_t code, StringView txt)
        {
            finish(code, txt.toBuffer(), "text/html; charset=UTF-8");
        }

        void IncomingRequest::finishXML(uint32_t code, StringView txt)
        {
            finish(code, txt.toBuffer(), "text/xml, application/xml");
        }

        //---

        RequestServer::RequestServer()
            : m_server(this)
        {}

        RequestServer::~RequestServer()
        {}

        bool RequestServer::init(uint16_t specificPort /*= 0*/)
        {
            auto localAddress  = socket::Address::Any4(specificPort);
            if (!m_server.init(localAddress))
            {
                TRACE_ERROR("Failed to start TCP server");
                return false;
            }

            m_address = m_server.address();
            TRACE_INFO("Local HTTP server started on '{}'", m_address);

            return true;
        }

        static StringView SanitizeURL(StringView path)
        {
            auto sanitizedPath  = path;
            if (sanitizedPath.beginsWith("/"))
                sanitizedPath = sanitizedPath.subString(1);
            if (sanitizedPath.endsWith("/"))
                sanitizedPath = sanitizedPath.leftPart(sanitizedPath.length() - 1);
            return sanitizedPath;
        }

        void RequestServer::registerHandler(StringView path, const TRequestHandlerFunc& handler)
        {
            if (!path.empty() && handler)
            {
                auto sanitizedPath  = SanitizeURL(path);
                TRACE_INFO("Registering URL request handler at '{}'", sanitizedPath);

                auto lock  = CreateLock(m_handlerMapLock);
                m_handlerMap[StringBuf(sanitizedPath)] = MemNew(TRequestHandlerFunc, handler);
            }
        }

        void RequestServer::unregisterHandler(StringView path)
        {
            if (!path.empty())
            {
                auto sanitizedPath  = SanitizeURL(path);

                auto lock  = CreateLock(m_handlerMapLock);
                m_handlerMap.remove(StringBuf(sanitizedPath));
            }
        }

        //---

        class RequestIncomingConnection : public IncomingRequest
        {
        public:
            RequestIncomingConnection(socket::tcp::Server* tcpServer, const RefPtr<RequestServer>& serverPtr, socket::ConnectionID connection, const RefPtr<RequestHeader>& header)
                : IncomingRequest(header)
                , m_owner(serverPtr)
                , m_tcpServer(tcpServer)
                , m_tcpConnection(connection)
            {}

            virtual void finish(uint32_t code, const Buffer& data, StringView contentType) override final
            {
                if (auto server  = m_owner.lock())
                {
                    StringBuilder response;
                    response << "HTTP/1.1 " << code;
                    
                    if (code >= 200 && code <= 299)
                        response << "OK";
                    else if (code >= 300 && code <= 399)
                        response << "Redirected";
                    else if (code >= 400 && code <= 499)
                        response << "Not Found";
                    else if (code >= 500 && code <= 599)
                        response << "Internal Server Error";
                    response << "\r\n";

                    response << "Server: BoomerEngine\r\n";
                    response << "Connection: Keep-Alive\r\n";
                    response << "Keep-Alive: timeout=60, max=999\r\n";

                    if (contentType)
                        response << "Content-Type: " << contentType << "\r\n";

                    response << "Content-Length: " << (data ? data.size() : 0) << "\r\n";
                    response << "\r\n";

                    m_tcpServer->send(m_tcpConnection, response.c_str(), response.length());

                    if (data)
                        m_tcpServer->send(m_tcpConnection, data.data(), data.size());

                    TRACE_INFO("HttpServer: Sent response {} to {}", code, header().host);
                }
            }

        private:
            socket::tcp::Server* m_tcpServer;
            socket::ConnectionID m_tcpConnection;

            RefWeakPtr<RequestServer> m_owner;
        };

        //---

        struct ActiveConnectionState : public IReferencable
        {
            socket::ConnectionID m_id = 0;
            socket::Address m_address;
            RequestHeaderParser m_headerParser;
        };

        void RequestServer::handleConnectionAccepted(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection)
        {
            auto ret  = RefNew<ActiveConnectionState>();
            ret->m_id = connection;
            ret->m_address = address;

            auto lock  = CreateLock(m_connectionsLock);
            m_connections[connection] = ret;
        }

        void RequestServer::handleConnectionClosed(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection)
        {
            auto lock  = CreateLock(m_connectionsLock);
            m_connections.remove(connection);
        }

        void RequestServer::serviceMissingHandler(socket::tcp::Server* server, socket::ConnectionID connection, const RefPtr<RequestHeader>& header)
        {
            TRACE_WARNING("No handler found to service URL '{}'", header->url);

            const char* response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
            server->send(connection, response, strlen(response));
        }

        void RequestServer::serviceRequest(socket::tcp::Server* server, socket::ConnectionID connection, const RefPtr<RequestHeader>& header)
        {
            TRequestHandlerFunc* handler;
            {
                auto url  = SanitizeURL(header->url.view().beforeFirstOrFull("?").beforeFirstOrFull("&"));

                auto lock  = CreateLock(m_handlerMapLock);
                for (;;)
                {
                    TRACE_INFO("Looking for handler for URL: '{}'", url);

                    if (m_handlerMap.find(StringBuf(url), handler))
                        break;

                    if (url.empty())
                        break;

                    url = url.beforeLast("/");
                }
                
            }

            if (handler)
            {
                auto incomingRequest  = RefNew<RequestIncomingConnection>(&m_server, AddRef(this), connection, header);
                (*handler)(incomingRequest);
            }
            else
            {
                serviceMissingHandler(server, connection, header);
            }
        }

        void RequestServer::handleConnectionData(socket::tcp::Server* server, const socket::Address& address, socket::ConnectionID connection, const void* data, uint32_t dataSize)
        {
            RefPtr<ActiveConnectionState> state;

            {
                auto lock = CreateLock(m_connectionsLock);
                m_connections.find(connection, state);
            }

            if (state)
            {
                InplaceArray<RefPtr<RequestHeader>, 10> headers;
                state->m_headerParser.push(data, dataSize, headers);

                if (state->m_headerParser.valid())
                {
                    for (auto& header : headers)
                        serviceRequest(server, connection, header);
                }
                else
                {
                    TRACE_ERROR("HTTP connection from '{}' has invalid content and will be closed", address);

                    const char* response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
                    server->send(connection, response, strlen(response));

                    server->disconnect(connection);
                }
            }
        }

        void RequestServer::handleServerClose(socket::tcp::Server* server)
        {
            auto lock = CreateLock(m_connectionsLock);
            m_connections.clear();
        }

        //---

    } // http
} // base