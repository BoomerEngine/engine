/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: protocol #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(socket)

enum class AddressType : uint8_t
{
    AddressIPv4,
    AddressIPv6
};

struct IPAddress4
{
    uint8_t address[4];
    uint16_t port;
};

struct IPAddress6
{
    uint8_t address[16];
    uint16_t port;
};

class CORE_SOCKET_API Address
{
public:
    static Address Any4(uint16_t port);
    static Address Any6(uint16_t port);

    static Address Local4(uint16_t port);
    static Address Local6(uint16_t port);

    Address();
    Address(const IPAddress4& address);
    Address(const IPAddress6& address);
    Address(AddressType type);
    Address(AddressType type, uint16_t port, const uint8_t* address);
    void set(AddressType type, uint16_t port, const uint8_t* address);

    Address(StringView txt, uint16_t portOverride = 0, AddressType type = AddressType::AddressIPv4);

    INLINE AddressType type() const { return m_type; }

    INLINE uint16_t port() const { return m_port; }

    INLINE const uint8_t* address() const { return m_address; }

    INLINE size_t length() const { return m_type == AddressType::AddressIPv4 ? 4 : 16; }

    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;

    ///---

    /// parse from string, safer form
    static bool Parse(StringView txt, Address& outAddress, uint16_t portOverride = 0, AddressType type = AddressType::AddressIPv4);

    ///---

    // get a string representation (for debug and error printing)
    void printDebug(IFormatStream& f) const;

    // print in native format
    void print(IFormatStream& f) const;

    ///--

    /// hashing for hashmaps
    static uint32_t CalcHash(const Address& addr);

private:
    AddressType m_type;
    uint16_t m_port;
    uint8_t m_address[16]; // worst case
};

END_BOOMER_NAMESPACE_EX(socket)
