/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "stringView.h"
#include "stringBuf.h"
#include "stringID.h"

namespace base
{

    //-----------------------------------------------------------------------------

    // CRC calculator for 32-bit CRC values, can be stored and updated as needed
    class BASE_CONTAINERS_API CRC32 : public base::NoCopy
    {
    public:
        static const uint32_t CRCTable[256];

        INLINE CRC32(uint32_t initValue = 0)
            : m_crc(~initValue)
        {};

        /// get CRC value computed so far
        INLINE uint32_t crc() const { return ~m_crc; }
        INLINE operator uint32_t() const { return ~m_crc; }

        /// append raw data of large size
        CRC32& append(const void* data, size_t size);

        /// wrappers for trivial types
        INLINE CRC32& operator<<(uint8_t data) { return appendStatic1(data); }
        INLINE CRC32& operator<<(uint16_t data) { return appendStatic2(data); }
        INLINE CRC32& operator<<(uint32_t data) { return appendStatic4(data); }
        INLINE CRC32& operator<<(uint64_t data) { return appendStatic8(data); }
        INLINE CRC32& operator<<(char data) { return appendStatic1(data); }
        INLINE CRC32& operator<<(short data) { return appendStatic2(data); }
        INLINE CRC32& operator<<(int data) { return appendStatic4(data); }
        INLINE CRC32& operator<<(int64_t data) { return appendStatic8(data); }
        INLINE CRC32& operator<<(float data) { return appendStatic4(reinterpret_cast<const uint32_t&>(data)); }
        INLINE CRC32& operator<<(double data) { return appendStatic8(reinterpret_cast<const uint64_t&>(data)); }
        INLINE CRC32& operator<<(bool data) { return appendStatic1(data); }

        // string types
        INLINE CRC32& operator<<(StringView data) { return append(data.data(), data.length()); }
        INLINE CRC32& operator<<(const StringBuf& data) { return append(data.c_str(), data.length()); }
        INLINE CRC32& operator<<(StringID data) { return append(data.view().data(), data.view().length()); }

    private:
        uint32_t m_crc;

        INLINE CRC32& appendStatic1(uint8_t data);
        CRC32& appendStatic2(uint16_t data);
        CRC32& appendStatic4(uint32_t data);
        CRC32& appendStatic8(uint64_t data);
    };

    //-----------------------------------------------------------------------------

    // CRC calculator
    class BASE_CONTAINERS_API CRC64 : public base::NoCopy
    {
    public:
        INLINE CRC64(uint64_t initValue = 0xCBF29CE484222325)
            : m_crc(initValue)
        {};

        INLINE CRC64(const CRC64& crc)
            : m_crc(crc.m_crc)
        {}

        /// get current CRC value
        INLINE uint64_t crc() const { return m_crc; }
        INLINE operator uint64_t() const { return m_crc; }

        /// append raw data of large size
        CRC64& append(const void* data, size_t size);

        /// wrappers for trivial types
        INLINE CRC64& operator<<(uint8_t data) { return appendStatic1(data);  }
        INLINE CRC64& operator<<(uint16_t data) { return appendStatic2(data); }
        INLINE CRC64& operator<<(uint32_t data) { return appendStatic4(data); }
        INLINE CRC64& operator<<(uint64_t data) { return appendStatic8(data); }
        INLINE CRC64& operator<<(char data) { return appendStatic1(data); }
        INLINE CRC64& operator<<(short data) { return appendStatic2(data); }
        INLINE CRC64& operator<<(int data) { return appendStatic4(data); }
        INLINE CRC64& operator<<(int64_t data) { return appendStatic8(data); }
        INLINE CRC64& operator<<(float data) { return appendStatic4(reinterpret_cast<const uint32_t&>(data)); }
        INLINE CRC64& operator<<(double data) { return appendStatic8(reinterpret_cast<const uint64_t&>(data)); }
        INLINE CRC64& operator<<(bool data) { return appendStatic1(data); }

        // string types
        INLINE CRC64& operator<<(StringView data) { return append(data.data(), data.length()); }
        INLINE CRC64& operator<<(const StringBuf& data) { return append(data.c_str(), data.length()); }
        INLINE CRC64& operator<<(StringID data) { return append(data.view().data(), data.view().length()); }

    private:
        uint64_t m_crc;

        INLINE CRC64& appendStatic1(uint8_t data);
        CRC64& appendStatic2(uint16_t data);
        CRC64& appendStatic4(uint32_t data);
        CRC64& appendStatic8(uint64_t data);

        static const uint64_t CRCTable[256];
    };

    //-----------------------------------------------------------------------------

} // base