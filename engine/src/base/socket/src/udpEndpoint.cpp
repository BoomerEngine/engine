/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: udp #]
***/

#include "build.h"
#include "block.h"
#include "blockAllocator.h"
#include "selector.h"

#include "udpEndpoint.h"
#include "udpPacket.h"
#include "udpSocket.h"

#include "base/system/include/thread.h"
#include "base/containers/include/inplaceArray.h"

namespace base
{
    namespace socket
    {
        namespace udp
        {
            //--

            IEndpointHandler::~IEndpointHandler()
            {}

            //--

            void ConnectionStats::print(IFormatStream& f) const
            {
                auto aliveTime  = startTime.timeTillNow().toSeconds();

                if (numPacketsSent)
                    f.appendf("  Packets sent: {}\n", numPacketsSent);
                if (numPacketsReceived)
                    f.appendf("  Packets recv: {}\n", numPacketsReceived);
                if (numDataPacketsSent)
                    f.appendf("  Data packet sent: {} ({}) in {} frags\n", numDataPacketsSent, MemSize(totalDataSent), numFragmentsSent);
                if (numDataPacketsReceived)
                    f.appendf("  Data packet recv: {} ({}) in {} frags\n", numDataPacketsReceived, MemSize(totalDataReceived), numFragmentsReceived);
                if (totalDataSent)
                    f.appendf("  Data rate sent: {}/s\n",  MemSize((double)totalDataSent / aliveTime));
                if (totalDataReceived)
                    f.appendf("  Data rate recv: {}/s\n",  MemSize((double)totalDataReceived / aliveTime));

                if (numDataPacketsReceived)
                {
                    f.appendf("  Lost packets: {} ({}%)\n", numLostPackets, Prec(numLostPackets / (float) numDataPacketsReceived * 100.0f, 2));
                    f.appendf("  OOB packets: {} ({}%)\n", numOutOfBoundPackets, Prec(numOutOfBoundPackets / (float) numDataPacketsReceived * 100.0f, 2));
                }
            }

            //--

            Endpoint::Endpoint(IEndpointHandler* handler, const EndpointConfig& config /*= EndpointConfig()*/, BlockAllocator* blockAllocator /*= nullptr*/)
                : m_handler(handler)
                , m_blockAllocator(nullptr)
                , m_nextConnectionID(1)
                , m_externalBlockAllocator(false)
                , m_config(config)
            {
                if (blockAllocator)
                {
                    m_blockAllocator = blockAllocator;
                    m_externalBlockAllocator = true;
                }
                else
                {
                    m_blockAllocator = MemNew(BlockAllocator);
                    m_externalBlockAllocator = false;
                }
            }

            Endpoint::~Endpoint()
            {
                if (m_externalBlockAllocator)
                    MemDelete(m_blockAllocator);

                m_externalBlockAllocator = false;
                m_blockAllocator = nullptr;
            }

            bool Endpoint::init(const Address& listenAddress)
            {
                // open socket
                if (!m_socket.open(listenAddress))
                {
                    TRACE_ERROR("UDP Endpoint error: Failed to create socket at address '{}'", listenAddress);
                    return false;
                }

                // try to set socket in non-blocking mode
                if (!m_socket.blocking(false))
                {
                    TRACE_ERROR("UDP Endpoint error: failed to set socket into non blocking mode");
                    m_socket.close();
                    return false;
                }

                // Try to disable IP-level packet fragmentation
                if (!m_socket.allowFragmentation(false))
                {
                    TRACE_ERROR("UDP Endpoint error: failed to disable fragmentation on socket");
                    m_socket.close();
                    return false;
                }

                // Try to set socket buffer limits
                /*if (!rawSocket->bufferSize(Constants::MAX_DATAGRAM_SIZE, Constants::MAX_DATAGRAM_SIZE))
                {
                    TRACE_ERROR("UDP Endpoint error: failed to check buffer sizes on socket");
                    rawSocket.close();
                    return false;
                }*/

                // Create thread processing the data
                ThreadSetup setup;
                setup.m_function = [this]() { threadFunc(); };
                setup.m_priority = ThreadPriority::AboveNormal;
                setup.m_name = "UDOSocketThread";

                // set address
                m_address = listenAddress;

                // Start selector thread
                m_thread.init(setup);
                return true;
            }

            void Endpoint::close()
            {
                m_socket.close();
                m_thread.close();

                {
                    auto lock  = CreateLock(m_pendingConnectionsLock);
                    m_pendingConnections.clearPtr();
                }

                {
                    auto lock  = CreateLock(m_activeConnectionsLock);
                    for (auto con  : m_activeConnections)
                    {
                        if (con->connected)
                        {
                            TRACE_INFO("Dropping UDP connection '{}'", con->address);
                            con->m_stats.print(TRACE_STREAM_INFO());
                        }
                    }

                    m_activeConnections.clearPtr();
                    m_activeConnectionsIDMap.clear();
                    m_activeConnectionsAddressMap.clear();
                }
            }

            ConnectionID Endpoint::connect(const Address& address, uint32_t overrideTimeout /*= INDEX_MAX*/)
            {
                // allocate ID
                auto id  = ++m_nextConnectionID;
                TRACE_INFO("Connecting to '{}' (ID {})", address, id);

                // create connection object
                auto connection = MemNew(Connection);
                connection->address = address;
                connection->id = id;
                connection->mtuSize = m_config.maxMtu;
                connection->nextSequenceNumber = 1;
                connection->timeoutPoint = NativeTimePoint::Now() + (m_config.connectionTimeoutMs / 1000.0);
                connection->nextPingPoint = NativeTimePoint::Now() + (m_config.timeoutProbeIntervalMs / 1000.0);
                connection->connected = false; // we are not yet confirmed
                connection->isConnectionInitializer = true;
                connection->m_stats = ConnectionStats();
                connection->m_stats.startTime.resetToNow();

                // add to connection maps - this allows to receive data
                {
                    auto lock  = CreateLock(m_activeConnectionsLock);
                    m_activeConnections.pushBack(connection);
                    m_activeConnectionsIDMap[id] = connection;
                    m_activeConnectionsAddressMap[address] = connection;
                }

                // create a pending connection entry
                auto pendingConnection  = MemNew(PendingConnection);
                pendingConnection->retriesLeft = m_config.maxConnectionRetries;
                pendingConnection->connectionTimeout = (overrideTimeout != INDEX_MAX) ? overrideTimeout : m_config.timeoutProbeIntervalMs;
                pendingConnection->connection = connection;

                // add to list of pending connections for tracking
                {
                    auto lock  = CreateLock(m_pendingConnectionsLock);
                    m_pendingConnections.pushBack(pendingConnection);
                }

                // send the initial connection request packet
                sendConnectionRequest(pendingConnection);
                return id;
            }

            void Endpoint::processPendingConnectionsTimeouts()
            {
                auto lock = CreateLock(m_pendingConnectionsLock);

                // anything timed out ?
                for (int i=m_pendingConnections.lastValidIndex(); i >= 0; --i)
                {
                    auto pending  = m_pendingConnections[i];

                    // handle timout
                    if (pending->timeoutPoint.reached())
                    {
                        // do we have any more tries ?
                        if (pending->retriesLeft > 0)
                        {
                            TRACE_INFO("Retrying connection to '{}'", pending->connection->address);
                            sendConnectionRequest(pending);
                            pending->retriesLeft -= 1;
                        }
                        // out of luck
                        else
                        {
                            TRACE_INFO("Not able to connect to '{}'", pending->connection->address);
                            m_handler->handleConnectionClosed(this, pending->connection->address, pending->connection->id);
                            pending->connection = nullptr;
                        }
                    }

                    // handle removal
                    if (!pending->connection)
                    {
                        m_pendingConnections.eraseUnordered(i);
                        MemDelete(pending);
                    }
                }
            }

            void Endpoint::collectConnectionsForPing(Array<Connection*>& outArray)
            {
                outArray.reset();

                auto lock = CreateLock(m_activeConnectionsLock);

                for (auto con  : m_activeConnections)
                    if (con->connected && con->nextPingPoint.reached())
                        outArray.pushBack(con);
            }

            void Endpoint::collectConnectionsTimeouted(Array<Connection*>& outArray)
            {
                outArray.reset();

                auto lock = CreateLock(m_activeConnectionsLock);

                for (auto con  : m_activeConnections)
                    if (con->connected && con->timeoutPoint.reached())
                        outArray.pushBack(con);
            }

            void Endpoint::processGeneralConnectionsTimeouts(Array<Connection*>& tempArray)
            {
                // collect connections that require ping to be sent
                collectConnectionsForPing(tempArray);

                // send pings
                for (auto con  : tempArray)
                    sendPing(con);

                // collect connections that have timed o
                collectConnectionsTimeouted(tempArray);

                // close them
                for (auto con  : tempArray)
                    sendTimeoutDisconnect(con);
            }

            void Endpoint::sendPing(Connection* connection)
            {
                TRACE_INFO("Sending ping to '{}'", connection->address);

                // send a simple connection packet
                PacketHeader pingHeader;
                pingHeader.type = (uint8_t) PacketType::TimeoutProbe;
                pingHeader.checksum = 0;

                // count packets
                connection->m_stats.numPacketsSent += 1;

                // send message directly over the raw socket
                // NOTE: we don't care in UDP what happens, if data failed to send we will get the "timeout"
                auto ret = m_socket.send(&pingHeader, sizeof(pingHeader), connection->address);
                if (ret == sizeof(pingHeader))
                {
                    // compute next timeout point
                    connection->nextPingPoint = NativeTimePoint::Now() + ((double)m_config.timeoutProbeIntervalMs / 1000.0);
                }
                else
                {
                    TRACE_INFO("Failed to send ping to '{}'", connection->address);
                }
            }

            void Endpoint::sendTimeoutDisconnect(Connection* connection)
            {
                TRACE_INFO("Sending disconnect due to timeout to '{}'", connection->address);

                // send a simple connection packet
                PacketHeader pingHeader;
                pingHeader.type = (uint8_t) PacketType::Disconnect;
                pingHeader.checksum = 0;

                // count packets
                connection->m_stats.numPacketsSent += 1;

                // send message directly over the raw socket
                // NOTE: we don't care in UDP what happens, if data failed to send we will get the "timeout"
                 m_socket.send(&pingHeader, sizeof(pingHeader), connection->address);

                 // close connection on our end so we can consider it dropped
                 closeConnection(connection);
            }

            void Endpoint::sendConnectionRequest(PendingConnection *request)
            {
                // compute next timeout point
                request->timeoutPoint = NativeTimePoint::Now() + ((double) request->connectionTimeout / 1000.0);

                // send a simple connection packet
                PacketHeader connectionHeader;
                connectionHeader.type = (uint8_t) PacketType::Connect;
                connectionHeader.checksum = 0;

                // count packets
                request->connection->m_stats.numPacketsSent += 1;

                // send message directly over the raw socket
                // NOTE: we don't care in UDP what happens, if data failed to send we will get the "timeout"
                m_socket.send(&connectionHeader, sizeof(connectionHeader), request->connection->address);
            }

            bool Endpoint::rawReceive()
            {
                // preallocate raw data, it's not a packet yet but reserve enough memory in case it's becoming one
                auto block = m_blockAllocator->alloc(Constants::MAX_DATAGRAM_SIZE + sizeof(Packet) + alignof(Packet));
                ASSERT(block != nullptr);

                // receive data
                Address sourceAddress;
                auto bytesReceived = m_socket.receive(block->data(), Constants::MAX_DATAGRAM_SIZE, &sourceAddress);
                if (bytesReceived > 0)
                {
                    // shrink the block to match the actually received data;
                    block->shrink(0, bytesReceived);

                    // get the memory preallocated for the data structure and build it there
                    void* packetMemory = OffsetPtr(block->data(), bytesReceived);
                    auto packet  = new (packetMemory) Packet(block, block->data(), sourceAddress);

                    // validate
                    auto expectedPacketSize = packet->calcTotalSize();
                    if (expectedPacketSize != bytesReceived)
                    {
                        TRACE_ERROR("UDP endpoint: unexpected packet size '{}' when proper would be '{}'", bytesReceived, expectedPacketSize);
                        block->release();
                        return false;
                    }
                    else
                    {
                        // ok, do something with data
                        processReceivedPacket(packet);
                        return true;
                    }
                }
				else if (bytesReceived == 0)
				{
					 // data dropped, eventually we will get a timeout
					return true;
				}
                else // fatal error
                {
                    TRACE_ERROR("UDP endpoint: fatal error on reading from socket");
                    block->release();
                    return false;
                }
            }

            void Endpoint::threadFunc()
            {
                ASSERT_EX(m_socket, "Accessing invalid socket, race with close()");
                auto socket = m_socket.systemSocket();

                Selector selector;
                InplaceArray<Connection*, 100> tempConnections;

                // loop while we don't have any error
                bool keepRunning = true;
                while (keepRunning)
                {
                    // wait for something
                    switch (selector.wait(SelectorOp::Read, &socket, 1, m_config.timeoutProbeIntervalMs))
                    {
                        // has data
                        case SelectorEvent::Ready:
                        {
                            for (auto& result : selector)
                            {
                                if (result.error)
                                {
                                    TRACE_ERROR("UDP Endpoint: socket got closed, exiting thread");
                                    keepRunning = false;
                                }
                                else
                                {
                                    if (!rawReceive())
                                        keepRunning = false;
                                }
                            }
                            break;
                        }

                        // we are busy, probe waiting connections for timeouts and return
                        case SelectorEvent::Busy:
                        {
                            processPendingConnectionsTimeouts();
                            processGeneralConnectionsTimeouts(tempConnections);
                            break;
                        }

                        // we have some error
                        case SelectorEvent::Error:
                        {
                            TRACE_ERROR("UDP endpoint: unrecoverable error in thread");
                            keepRunning = false;
                            break;
                        }
                    }
                }

                TRACE_ERROR("UDP endpoint: finished thread for '{}'", m_address);
            }

            static uint32_t CalculateFragmentPayloadSize(uint32_t mtu, const Address& address)
            {
                bool ipv6 = address.type() == AddressType::AddressIPv6;
                uint32_t ipHeaderOverhead = ipv6 ? Constants::IP_HEADER_OVERHEAD_IPV6 : Constants::IP_HEADER_OVERHEAD;
                uint32_t udpHeaderOverhead = Constants::UDP_HEADER_OVERHEAD;
                uint32_t headerOverhead = sizeof(PacketHeader);
                return mtu - ipHeaderOverhead - udpHeaderOverhead - headerOverhead;
            }

            struct BlockReader : public NoCopy
            {
            public:
                BlockReader(const Array<BlockPart> &inputBlocks)
                    : blocks(inputBlocks)
                {
                    for (auto& block : blocks)
                        totalSize += block.size;

                    if (!blocks.empty())
                    {
                        curPos = (const uint8_t*)blocks[0].dataPtr;
                        endPos = curPos + blocks[0].size;
                    }
                }

                INLINE uint32_t size() const
                {
                    return totalSize;
                }

                INLINE uint32_t pos() const
                {
                    return offset;
                }

                INLINE void read(void* outData, uint32_t size)
                {
                    auto writePtr  = (uint8_t*) outData;

                    if (curPos == endPos)
                    {
                        blockIndex += 1;
                        auto& block = blocks[blockIndex];
                        curPos = (const uint8_t*)block.dataPtr;
                        endPos = curPos + block.size;
                    }

                    uint32_t left = size;
                    while (left > 0)
                    {
                        auto maxCopy = endPos - curPos;
                        auto toCopy = std::min<uint32_t>(left, maxCopy);
                        memcpy(writePtr, curPos, toCopy);

                        curPos += toCopy;
                        writePtr += toCopy;
                        offset += toCopy;

                        left -= toCopy;
                    }
                }

            private:
                uint32_t offset = 0;
                uint32_t totalSize = 0;
                uint32_t blockIndex = 0;
                const uint8_t* curPos = nullptr;
                const uint8_t* endPos = nullptr;

                const Array<BlockPart>& blocks;
            };

            void Endpoint::rawSend(const void* data, int size, const Address& destinationAddress)
            {
                for (;;)
                {
                    auto ret = m_socket.send(data, size, destinationAddress);
                    if (ret == 0)
                    {
                        TRACE_WARNING("UDP Endpoint: sending buffer full");
                        Sleep(1);
                        continue;
                    }
                    else if (ret < 0)
                    {
                        TRACE_WARNING("UDP Endpoint: sending error");
                        break;
                    }
                    else
                    {
                        TRACE_INFO("UDP Endpoint: sent '{}' bytes to '{}'", size, destinationAddress);
                        break;
                    }
                }
            }

            Endpoint::Connection* Endpoint::findConnection(ConnectionID id)
            {
                auto lock = CreateLock(m_activeConnectionsLock);

                Connection* con = nullptr;
                if (m_activeConnectionsIDMap.find(id, con))
                    if (con->connected)
                        return con;

                return nullptr;
            }

            bool Endpoint::send(ConnectionID id, const BlockPart& block)
            {
                InplaceArray<BlockPart, 1> blocks;
                blocks.pushBack(block);
                return send(id, blocks);
            }

            bool Endpoint::send(ConnectionID id, const Array<BlockPart>& blocks)
            {
                uint8_t mtuBuffer[Constants::MAX_MTU];

                // get target address for connection
                auto connection  = findConnection(id);
                if (!connection)
                    return false;

                // get the maximum size of data in single packet
                auto maxPayload = CalculateFragmentPayloadSize(connection->mtuSize, connection->address);
                auto maxDataPacketSize = maxPayload - sizeof(DataPacketHeader);

                // allocate sequence index for this packet
                auto sequenceIndex = ++connection->nextSequenceNumber;

                // send data in batches
                BlockReader reader(blocks);
                uint16_t fragmentIndex = 0;
                while (reader.pos() < reader.size())
                {
                    auto left = reader.size() - reader.pos();
                    auto writeSize = std::min<uint32_t>(left, maxDataPacketSize);

                    // prepare header
                    auto header = (PacketHeader *) (mtuBuffer + 0);
                    header->type = (uint8_t) PacketType::Data;
                    header->checksum = 0; // computed later

                    // data header
                    auto dataHeader = (DataPacketHeader *) (mtuBuffer + sizeof(PacketHeader));
                    dataHeader->totalSize = reader.size();
                    dataHeader->id = 0; // ?
                    dataHeader->sequenceNumber = sequenceIndex;
                    dataHeader->fragmentIndex = fragmentIndex++;
                    dataHeader->dataSize = writeSize;

                    // extract data to copy
                    auto payload = (DataPacketHeader *) (mtuBuffer + sizeof(PacketHeader) + sizeof(DataPacketHeader));
                    reader.read(payload, writeSize);

                    // send via low level socket
                    auto totalSize = writeSize + sizeof(PacketHeader) + sizeof(DataPacketHeader);
                    ASSERT(totalSize <= Constants::MAX_DATAGRAM_SIZE);
                    rawSend(mtuBuffer, totalSize, connection->address);

                    // stats
                    connection->m_stats.numPacketsSent += 1;
                    connection->m_stats.numFragmentsSent += 1;
                }

                // stats
                connection->m_stats.numDataPacketsSent += 1;
                connection->m_stats.totalDataSent += reader.size();
                return true;
            }

            void Endpoint::processConnectionRequest(Connection* connection, Packet* packet)
            {
                // report about incoming connection
                if (!connection->connected)
                {
                    if (connection->isConnectionInitializer)
                    {
                        TRACE_WARNING("UDP Endpoint: Received unexpected connection request from '{}' ", connection->address);
                    }
                    else
                    {
                        TRACE_INFO("UDP Endpoint: Received connection request from '{}'", connection->address);

                        // reset connection state
                        connection->m_stats = ConnectionStats();
                        connection->m_stats.startTime.resetToNow();

                        // report to the handler
                        connection->connected = true;
                        m_handler->handleConnectionRequest(this, connection->address, connection->id);

                        // TODO: add option to silently block connections ?

                        // send a simple connection packet
                        PacketHeader ackHeader;
                        ackHeader.type = (uint8_t) PacketType::Acknowledge;
                        ackHeader.checksum = 0;

                        // stats
                        connection->m_stats.numPacketsSent += 1;

                        // send message directly over the raw socket
                        // NOTE: we don't care in UDP what happens, if data failed to send we will get the "timeout"
                        m_socket.send(&ackHeader, sizeof(ackHeader), connection->address);
                    }
                }

                packet->release();
            }

            void Endpoint::processConnectionAck(Connection* connection, Packet* packet)
            {
                // report about incoming connection
                if (!connection->connected)
                {
                    if (!connection->isConnectionInitializer)
                    {
                        TRACE_WARNING("UDP Endpoint: Received unexpected connection ack from '{}' ", connection->address);
                    }
                    else
                    {
                        m_handler->handleConnectionSucceeded(this, connection->address, connection->id);

                        TRACE_INFO("UDP Endpoint: Received connection ack from '{}'", connection->address);
                        connection->connected = true;
                    }

                    {
                        auto lock = CreateLock(m_pendingConnectionsLock);
                        for (uint32_t i=0; i<m_pendingConnections.size(); ++i)
                        {
                            if (m_pendingConnections[i]->connection == connection)
                            {
                                MemDelete(m_pendingConnections[i]);
                                m_pendingConnections.eraseUnordered(i);
                            }
                        }
                    }
                }

                packet->release();
            }

            void Endpoint::cleanFragments(Connection* connection, bool lost)
            {
                if (!connection->receivedFragments.empty())
                {
                    TRACE_INFO("Releasing {} unprocessed fragments", connection->receivedFragments.size());

                    for (auto packet  : connection->receivedFragments)
                    {
                        packet->release();

                        if (lost)
                            connection->m_stats.numLostPackets += 1;
                    }

                    connection->receivedFragments.reset();
                    connection->receivedFragmentsTotalData = 0;
                }
            }

            void Endpoint::processDataPacket(Connection* connection, Packet* packet)
            {
                auto& header = packet->dataHeader();

                // stats
                connection->m_stats.totalDataReceived += packet->dataHeader().dataSize;
                connection->m_stats.numFragmentsReceived += 1;

                // detect out of bound
                if (header.sequenceNumber < connection->maxReceivedSequenceID)
                {
                    TRACE_INFO("Received out of bound packet {} < {} from '{}'", header.sequenceNumber, connection->maxReceivedSequenceID, connection->address);
                    connection->m_stats.numOutOfBoundPackets += 1;
                    packet->release();
                    return;
                }

                // new sequence
                if (header.sequenceNumber > connection->maxReceivedSequenceID)
                {
                    TRACE_INFO("Received new seqence {} > {} from '{}'", header.sequenceNumber, connection->maxReceivedSequenceID, connection->address);
                    connection->maxReceivedSequenceID = header.sequenceNumber;
                    cleanFragments(connection, true);
                }

                // single piece shit
                ASSERT(header.sequenceNumber == connection->maxReceivedSequenceID);
                if (header.dataSize == header.totalSize)
                {
                    TRACE_INFO("Single piece fragment {}, size {}", connection->maxReceivedSequenceID, header.totalSize);
                    m_handler->handleConnectionData(this, connection->address, connection->id, packet->mutateToBlock());

                    connection->m_stats.numDataPacketsReceived += 1;
                    connection->maxReceivedSequenceID += 1; // do not allow getting this again
                }
                // multi part
                else if (header.dataSize < header.totalSize)
                {
                    TRACE_INFO("Multi piece fragment {}, size {} of ({}), index {}", connection->maxReceivedSequenceID, header.dataSize, header.totalSize, header.fragmentIndex);

                    // prepare fragment list
                    if (connection->receivedFragments.empty())
                    {
                        // calculate number of fragments required
                        auto maxPayload = CalculateFragmentPayloadSize(connection->mtuSize, connection->address);
                        auto maxDataPacketSize = maxPayload - sizeof(DataPacketHeader);
                        auto maxFragmentCount = (header.totalSize / maxDataPacketSize) + 1;

                        // prepare list
                        connection->receivedFragments.resizeWith(maxFragmentCount, nullptr);
                        connection->receivedFragmentsTotalData = 0;
                        TRACE_INFO("Expecting '{}' fragments for '{}'", maxFragmentCount, header.sequenceNumber);
                    }

                    // out of bound fragment
                    if (header.fragmentIndex >= connection->receivedFragments.size())
                    {
                        TRACE_INFO("Out of bound fragment '{}' for seq {}", header.fragmentIndex, header.sequenceNumber);
                        connection->maxReceivedSequenceID += 1; // do not allow getting this again
                        packet->release();
                    }
                    else if (connection->receivedFragments[header.fragmentIndex] != nullptr)
                    {
                        TRACE_ERROR("Fragment {} in {} already received", header.fragmentIndex, header.sequenceNumber);
                        packet->release();
                    }
                    else
                    {
                        connection->receivedFragments[header.fragmentIndex] = packet;
                        connection->receivedFragmentsTotalData += header.dataSize;

                        // all fragments ?
                        if (connection->receivedFragmentsTotalData == header.totalSize)
                        {
                            // allocate final block
                            TRACE_INFO("Received all pices ({}) of seq {}, gatherd size {}", connection->receivedFragments.size(), header.sequenceNumber, connection->receivedFragmentsTotalData);
                            auto block = m_blockAllocator->alloc(header.totalSize);

                            // reassemble fragments
                            auto writePtr  = block->data();
                            for (uint32_t i=0; i<connection->receivedFragments.size(); ++i)
                            {
                                auto frag  = connection->receivedFragments[i];
                                ASSERT(frag->dataHeader().fragmentIndex == i);
                                memcpy(writePtr, frag->data(), frag->dataHeader().dataSize);
                                writePtr += frag->dataHeader().dataSize;
                            }
                            ASSERT(writePtr - block->data() == header.totalSize);

                            // release used fragments
                            connection->maxReceivedSequenceID += 1; // do not allow getting this again
                            cleanFragments(connection, false);

                            // stats
                            connection->m_stats.numDataPacketsReceived += 1;

                            // send for processing
                            m_handler->handleConnectionData(this, connection->address, connection->id, block);
                        }
                        else if (connection->receivedFragmentsTotalData > header.totalSize)
                        {
                            TRACE_ERROR("To many data received {} > {} in seq {}", connection->receivedFragmentsTotalData, header.totalSize, header.sequenceNumber);
                            connection->m_stats.numLostPackets += 1;
                            connection->maxReceivedSequenceID += 1; // do not allow getting this again
                            cleanFragments(connection, true);
                        }
                    }
                }
            }

            void Endpoint::processDisconnect(Connection* connection, Packet* packet)
            {
                TRACE_INFO("UDP Endpoint: Received remote disconnect from '{}'", connection->address);
                closeConnection(connection);
                packet->release();
            }

            void Endpoint::processPingPong(Connection* connection, Packet* packet)
            {
                TRACE_INFO("UDP Endpoint: Received ping from '{}'", packet->address());
                packet->release();
            }

            void Endpoint::processReceivedPacket(Packet* packet)
            {
                // NOTE: we may get another connection request even though we already
                Connection* connection = nullptr;
                {
                    auto lock = CreateLock(m_activeConnectionsLock);
                    if (!m_activeConnectionsAddressMap.find(packet->address(), connection))
                    {
                        TRACE_INFO("UDP Endpoint: Received packet from unknown endpoint '{}'", packet->address());

                        // crate connection entry
                        connection = MemNew(Connection);
                        connection->timeoutPoint = NativeTimePoint::Now() + (m_config.connectionTimeoutMs / 1000.0);
                        connection->nextPingPoint = NativeTimePoint::Now() + (m_config.timeoutProbeIntervalMs / 1000.0);
                        connection->mtuSize = m_config.maxMtu;
                        connection->address = packet->address();
                        connection->id = ++m_nextConnectionID;
                        connection->connected = false; // we are not yet confirmed

                        m_activeConnections.pushBack(connection);
                        m_activeConnectionsIDMap[connection->id] = connection;
                        m_activeConnectionsAddressMap[connection->address] = connection;
                    }
                    else
                    {
                        // we just got a message, update out timeouts
                        connection->timeoutPoint = NativeTimePoint::Now() + (m_config.connectionTimeoutMs / 1000.0);
                        connection->nextPingPoint = NativeTimePoint::Now() + (m_config.timeoutProbeIntervalMs / 1000.0);
                    }
                }

                // stats
                connection->m_stats.numPacketsReceived += 1;

                // process the packet
                auto packetType = (PacketType) packet->header().type;
                switch (packetType)
                {
                    case PacketType::Connect:
                        processConnectionRequest(connection, packet);
                        break;

                    case PacketType::Acknowledge:
                        processConnectionAck(connection, packet);
                        break;

                    case PacketType::Data:
                        processDataPacket(connection, packet);
                        break;

                    case PacketType::Disconnect:
                        processDisconnect(connection, packet);
                        break;

                    case PacketType::TimeoutProbe:
                        processPingPong(connection, packet);
                        break;

                    default:
                        TRACE_INFO("Unknown packet type '{}' received from '{}'", packet->header().type, connection->address);
                        packet->release();
                }
            }

            void Endpoint::closeConnection(Connection* connection)
            {
                if (connection->connected)
                {
                    static base::SpinLock printLock;

                    printLock.acquire();
                    TRACE_INFO("Closing UDP connection '{}'", connection->address);
                    connection->m_stats.print(TRACE_STREAM_INFO());
                    printLock.release();

                    connection->connected = false;
                    m_handler->handleConnectionClosed(this, connection->address, connection->id);
                }
            }

            bool Endpoint::disconnect(ConnectionID id)
            {
                // get target address for connection
                auto connection  = findConnection(id);
                if (!connection)
                    return false;

                // send the disconnection header
                PacketHeader header;
                header.type = (uint8_t) PacketType::Disconnect;
                TRACE_INFO("UDP Endpoint: Sending disconnect to endpoint '{}'", connection->address);
                rawSend(&header, sizeof(header), connection->address);

                // put in disconnected state
                closeConnection(connection);
                return true;
            }

        } // udp
    } // socket
} // base