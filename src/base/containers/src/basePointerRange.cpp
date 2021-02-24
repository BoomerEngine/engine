/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "basePointerRange.h"
#include "pointerRange.h"

BEGIN_BOOMER_NAMESPACE(base)

//---

void BasePointerRange::zeroBytes()
{
    memzero(data(), dataSize());
}

void BasePointerRange::fillBytes(uint8_t value)
{
    memset(data(), value, dataSize());
}

int BasePointerRange::compareBytes(BasePointerRange other) const
{
    if (dataSize() < other.dataSize())
        return -1;
    else if (dataSize() > other.dataSize())
        return 1;

    return memcmp(data(), other.data(), dataSize());
}

void BasePointerRange::reverseBytes()
{
    auto* a = m_start;
    auto* b = m_end - 1;
    while (a < b)
        std::swap(*a++, *b--);
}

void BasePointerRange::byteswap16()
{
    auto* ptr = (uint16_t*)m_start;
    auto* endPtr = (uint16_t*)m_end - 1;
    while (ptr <= endPtr)
        *ptr = _byteswap_ushort(*ptr);
}

void BasePointerRange::byteswap32()
{
    auto* ptr = (uint32_t*)m_start;
    auto* endPtr = (uint32_t*)m_end - 1;
    while (ptr <= endPtr)
        *ptr = _byteswap_ulong(*ptr);
}

void BasePointerRange::byteswap64()
{
    auto* ptr = (uint64_t*)m_start;
    auto* endPtr = (uint64_t*)m_end - 1;
    while (ptr <= endPtr)
        *ptr = _byteswap_uint64(*ptr);
}

//---

END_BOOMER_NAMESPACE(base)