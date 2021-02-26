/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: raw #]
***/

#pragma once

#include "udpPacket.h"

BEGIN_BOOMER_NAMESPACE_EX(socket)

namespace Constants
{

    static const uint32_t MAX_PACKETS = 256;
    static const uint32_t MAX_CONNECTIONS = 32;
            
    // RFC 791
    static const uint32_t MIN_MTU = 576;
    static const uint32_t MIN_MTU_IPV6 = 1280;
    static const uint32_t MAX_MTU = 1500;

    static const uint16_t DEFAULT_MTU = 1400;

    static const uint32_t IP_HEADER_OVERHEAD = 28;
    static const uint32_t IP_HEADER_OVERHEAD_IPV6 = 48;

    static const uint32_t UDP_HEADER_OVERHEAD = 8;

    static const uint32_t MIN_DATAGRAM_SIZE = 4;
    static const uint32_t MAX_DATAGRAM_SIZE = MAX_MTU - IP_HEADER_OVERHEAD - UDP_HEADER_OVERHEAD;
    static const uint32_t MAX_DATAGRAM_SIZE_IPV6 = MAX_MTU - IP_HEADER_OVERHEAD_IPV6 - UDP_HEADER_OVERHEAD;
            
    static const uint32_t DEFAULT_CONNECTION_TIMEOUT_MS = 5000;
    static const uint32_t DEFAULT_PING_INTERVAL_MS = 1000;
    static const uint32_t DEFAULT_SEND_TIMEOUT_MS = 50;
    static const uint32_t DEFAULT_SELECTOR_TIMEOUT_MS = 5;

    static const uint8_t DEFAULT_RETRIES = 3;
    static const uint8_t DEFAULT_WINDOW_SIZE = 128;

    static const uint32_t INFINITY_MS = 0xffffffff;

} // constants

END_BOOMER_NAMESPACE_EX(socket)
