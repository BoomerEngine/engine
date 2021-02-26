/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: udp #]
***/

#pragma once

#include "address.h"
#include "constants.h"
#include "udpSocket.h"

#include "core/system/include/timing.h"
#include "core/system/include/thread.h"
#include "core/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE_EX(socket::udp)

//--

/// endpoint event handler,
/// NOTE: all events happen from NON-FIBER network threads
class CORE_SOCKET_API IEndpointHandler : public NoCopy
{
public:
    virtual ~IEndpointHandler();

    /// connection request was successful
    virtual void handleConnectionSucceeded(Endpoint* endpoint, const Address& address, ConnectionID connection) = 0;

    /// connection was closed (either by us, or remote peer or lack of pings)
    virtual void handleConnectionClosed(Endpoint* endpoint, const Address& address, ConnectionID connection) = 0;

    /// there's incoming connection from given address
    virtual void handleConnectionRequest(Endpoint* endpoint, const Address& address, ConnectionID connection) = 0;

    /// data was received by connection, returns the payload
    virtual void handleConnectionData(Endpoint* endpoint, const Address& address, ConnectionID connection, Block* dataPayload) = 0;

    /// our endpoint died (socket error)
    virtual void handleEndpointError(Endpoint* endpoint) = 0;
};

//--

/// connection stats
struct CORE_SOCKET_API ConnectionStats
{
    NativeTimePoint startTime;

    uint32_t numPacketsSent = 0;
    uint32_t numPacketsReceived = 0;

    uint32_t numDataPacketsSent = 0;
    uint32_t numDataPacketsReceived = 0;
    uint64_t totalDataSent = 0;
    uint64_t totalDataReceived = 0;

    uint32_t numFragmentsSent = 0;
    uint32_t numFragmentsReceived = 0;
    uint32_t numLostPackets = 0;
    uint32_t numOutOfBoundPackets = 0;

    void print(IFormatStream& f) const;
};

//--

// endpoint configuration params
struct EndpointConfig
{
    uint8_t maxRetransmissions = Constants::DEFAULT_RETRIES;
    uint8_t maxConnectionRetries = Constants::DEFAULT_RETRIES;
    uint32_t connectionTimeoutMs = Constants::DEFAULT_CONNECTION_TIMEOUT_MS;
    uint32_t sendTimeoutMs = Constants::DEFAULT_SEND_TIMEOUT_MS;
    uint32_t timeoutProbeIntervalMs = Constants::DEFAULT_SELECTOR_TIMEOUT_MS;
    uint32_t maxMtu = Constants::MAX_MTU;
};

//--

/// endpoint for the UDP communication
class CORE_SOCKET_API Endpoint : public NoCopy
{
public:
    Endpoint(IEndpointHandler* handler, const EndpointConfig& config = EndpointConfig(), BlockAllocator* blockAllocator = nullptr);
    ~Endpoint();

    //--

    // get the address we are listening on
    INLINE const Address& address() const { return m_address; }

    //--

    // start an UDP endpoint on given address (usually a 0.0.0.0 + some port)
    // connections can be established as soon as this function returns
    bool init(const Address& listenAddress);

    // close endpoint without destroying all data and other setup
    void close();

    //--

    // connect to a remote endpoint, if the remote endpoint responds within the time limit we get
    // this functions returns an unique ConnectionID number that will identify the connection to remote endpoint
    ConnectionID connect(const Address& address, uint32_t overrideTimeout = INDEX_MAX);

    //--

    // send data via connection, fails only if connection ID is no longer valid
    // NOTE: data may NOT arrive at all
    // NOTE: data may arrive OUT OF ORDER (but than it will be dropped)
    bool send(ConnectionID id, const Array<BlockPart>& blocks);
    bool send(ConnectionID id, const BlockPart& block);

    // disconnect
    bool disconnect(ConnectionID id);

    // get stats for given connection
    bool stat(ConnectionID id, ConnectionStats& outStats) const;

private:
    IEndpointHandler* m_handler;

    EndpointConfig m_config;

    RawSocket m_socket;
    Address m_address;

    BlockAllocator* m_blockAllocator;
    bool m_externalBlockAllocator;

    std::atomic<ConnectionID> m_nextConnectionID;

    Thread m_thread;

    //--

    struct Connection : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_NET)

    public:
        Address address;
        ConnectionID id = 0;
        uint32_t mtuSize = Constants::DEFAULT_MTU;
        std::atomic<uint32_t> nextSequenceNumber = 1;
        NativeTimePoint timeoutPoint;
        NativeTimePoint nextPingPoint;
        bool connected = false;
        bool isConnectionInitializer = false;

        uint32_t maxReceivedSequenceID = 0;
        Array<Packet*> receivedFragments;
        uint32_t receivedFragmentsTotalData = 0;

        ConnectionStats m_stats;
    };

    Array<Connection*> m_activeConnections;
    HashMap<ConnectionID, Connection*> m_activeConnectionsIDMap;
    HashMap<Address, Connection*> m_activeConnectionsAddressMap;
    SpinLock m_activeConnectionsLock;

    //--

    struct PendingConnection : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_NET)

    public:
        Connection* connection = nullptr;
        NativeTimePoint timeoutPoint;
        uint32_t connectionTimeout;
        uint8_t retriesLeft;
    };

    Array<PendingConnection*> m_pendingConnections;
    SpinLock m_pendingConnectionsLock;

    //--

    void sendConnectionRequest(PendingConnection* request);
    void sendPing(Connection* connection);
    void sendTimeoutDisconnect(Connection* connection);

    void processPendingConnectionsTimeouts();
    void processGeneralConnectionsTimeouts(Array<Connection*>& tempArray);
    void processReceivedPacket(Packet* packet);

    void collectConnectionsForPing(Array<Connection*>& outArray);
    void collectConnectionsTimeouted(Array<Connection*>& outArray);

    void processConnectionRequest(Connection* connection, Packet* packet);
    void processConnectionAck(Connection* connection, Packet* packet);
    void processDataPacket(Connection* connection, Packet* packet);
    void processDisconnect(Connection* connection, Packet* packet);
    void processPingPong(Connection* connection, Packet* packet);

    void cleanFragments(Connection* connection, bool lost);
    void closeConnection(Connection* connection);

    Connection* findConnection(ConnectionID id);

    void rawSend(const void* data, int size, const Address& destinationAddress);

    bool rawReceive();

    void threadFunc();
};

END_BOOMER_NAMESPACE_EX(socket::udp)
