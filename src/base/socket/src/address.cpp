/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: protocol #]
***/

#include "build.h"
#include "address.h"

#if defined(PLATFORM_WINDOWS)
#include <Ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace base
{
    namespace socket
    {
        static const uint8_t ZERO_IPV4[4] = { 0, 0, 0, 0 };
        
        Address Address::Any4(uint16_t port)
        {
            uint8_t rawAddress[4] = { 0, 0, 0, 0 };
            return Address(socket::AddressType::AddressIPv4, port, rawAddress);
        }

        Address Address::Any6(uint16_t port)
        {
            uint8_t rawAddress[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            return Address(socket::AddressType::AddressIPv6, port, rawAddress);
        }

        Address Address::Local4(uint16_t port)
        {
            uint8_t rawAddress[4] = { 127, 0, 0, 1 };
            return Address(socket::AddressType::AddressIPv4, port, rawAddress);
        }

        Address Address::Local6(uint16_t port)
        {
            uint8_t rawAddress[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
            return Address(socket::AddressType::AddressIPv6, port, rawAddress);
        }

        Address::Address()
            : Address(AddressType::AddressIPv4, 0, ZERO_IPV4)
        {
        }

        Address::Address(const IPAddress4& address)
            : Address(AddressType::AddressIPv4, address.port, address.address)
        {
        }
        
        Address::Address(const IPAddress6& address)
            : Address(AddressType::AddressIPv6, address.port, address.address)
        {
        }

        Address::Address(StringView txt, uint16_t portOverride /*= 0*/, AddressType type /*= AddressType::AddressIPv4*/)
            : m_type(type)
            , m_port(0)
        {
            memset(m_address, 0, sizeof(m_address));

            if (!Parse(txt, *this, portOverride, type))
            {
                TRACE_ERROR("Invalid network address '{}'", txt);
            }
        }

        bool Address::Parse(StringView txt, Address& outAddress, uint16_t portOverride /*= 0*/, AddressType type /*= AddressType::AddressIPv4*/)
        {
            uint32_t port = portOverride;
            auto mainPart = txt;

            if (mainPart.beginsWith("IP4:"))
                mainPart = mainPart.afterFirst("IP4:");
            else if (mainPart.beginsWith("IP6:"))
                mainPart = mainPart.afterFirst("IP6:");

            if (auto portPart = mainPart.afterLast(":"))
            {
                if (MatchResult::OK != portPart.match(port))
                    return false;
                mainPart = mainPart.beforeLast(":");
            }

            TempString tempString;
            tempString << mainPart;

            uint8_t address[16];

            auto domain = (type == AddressType::AddressIPv4) ? AF_INET : AF_INET6;
            if (inet_pton(domain, tempString.c_str(), address) <= 0)
                return false;

            outAddress = Address(type, port, address);
            return true;
        }

        Address::Address(AddressType type)
            : m_type(type)
            , m_port(0)
        {
            auto size = (m_type == AddressType::AddressIPv4) ? 4 : 16;
            memzero(m_address, size);
        }

        Address::Address(AddressType type, uint16_t port, const uint8_t* address)
            : m_type(type)
            , m_port(port)
        {
            auto size = (m_type == AddressType::AddressIPv4) ? 4 : 16;
            memset(m_address, 0, sizeof(m_address));
            memcpy(m_address, address, size);
        }

        void Address::set(AddressType type, uint16_t port, const uint8_t* address)
        {
            auto size = (m_type == AddressType::AddressIPv4) ? 4 : 16;

            m_type = type;
            m_port = port;
            memset(m_address, 0, sizeof(m_address));
            memcpy(m_address, address, size);
        }

        bool Address::operator==(const Address& rhs) const
        {
            return m_type == rhs.m_type && m_port == rhs.m_port && 0 == memcmp(m_address, rhs.m_address, length());
        }
        
        bool Address::operator!=(const Address& rhs) const
        {
            return !operator==(rhs);
        }

        uint32_t Address::CalcHash(const Address& addr)
        {
            CRC32 crc;

            if (addr.m_type == AddressType::AddressIPv4)
            {
                crc << addr.m_port;
                crc << *(const uint32_t*)(&addr.m_address[0]);
            }
            else
            {
                crc << addr.m_port;
                crc << *(const uint32_t *) (&addr.m_address[0]);
                crc << *(const uint32_t *) (&addr.m_address[4]);
                crc << *(const uint32_t *) (&addr.m_address[8]);
                crc << *(const uint32_t *) (&addr.m_address[12]);
            }

            return crc;
        }

        void Address::print(base::IFormatStream& f) const
        {
            char buf[128];
            memset(buf, 0, sizeof(buf));
            switch (m_type)
            {
            case AddressType::AddressIPv4:
#if defined(PLATFORM_WINDOWS)
                InetNtopA(AF_INET, (PVOID)m_address, buf, ARRAY_COUNT(buf));
#else
                inet_ntop(AF_INET, address, buf, ARRAY_COUNT(buf));
#endif
                f << (char*)buf;
                break;
            case AddressType::AddressIPv6:
#if defined(PLATFORM_WINDOWS)
                InetNtopA(AF_INET6, (PVOID)m_address, buf, ARRAY_COUNT(buf));
#else
                inet_ntop(AF_INET6, address, buf, ARRAY_COUNT(buf));
#endif
                f << (char*)buf;
                break;
            }

            if (m_port != 0)
                f << ":" << m_port;
        }

        void Address::printDebug(base::IFormatStream& f) const
        {
            char buf[128];
            switch (m_type)
            {
            case AddressType::AddressIPv4:
#if defined(PLATFORM_WINDOWS)
                InetNtopA(AF_INET, (PVOID)m_address, buf, ARRAY_COUNT(buf));
#else
                inet_ntop(AF_INET, address, buf, ARRAY_COUNT(buf));
#endif
                f.append("IP4:").append(buf).appendf(":{}", m_port);;
                break;
            case AddressType::AddressIPv6:
#if defined(PLATFORM_WINDOWS)
                InetNtopA(AF_INET6, (PVOID)m_address, buf, ARRAY_COUNT(buf));
#else
                inet_ntop(AF_INET6, address, buf, ARRAY_COUNT(buf));
#endif
                f.append("IP6:").append(buf).appendf(":{}", m_port);
                break;
            default:
                f.append("Unknown");
                break;
            }
        }
    } // socket
} // base