/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: raw #]
***/

#include "build.h"
#include "block.h"
#include "blockBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(socket)

//--

BlockBuilder::BlockBuilder()
{
    m_startPos = m_internalBuffer;
    m_curPos = m_internalBuffer;
    m_endPos = m_internalBuffer + MAX_INTERNAL_SIZE;
}

BlockBuilder::~BlockBuilder()
{
    clear();
}

void BlockBuilder::clear()
{
    if (m_startPos != m_internalBuffer)
        GlobalPool<POOL_NET, uint8_t>::Free(m_startPos);

    m_startPos = m_internalBuffer;
    m_curPos = m_internalBuffer;
    m_endPos = m_internalBuffer + MAX_INTERNAL_SIZE;
}

void BlockBuilder::grow(uint32_t additionalSizeNeeded)
{
    uint32_t capacity = (m_endPos - m_startPos);
    uint32_t size = (m_curPos - m_startPos);
    if (size + additionalSizeNeeded <= capacity)
        return;

    while (size + additionalSizeNeeded > capacity)
        capacity *= 2;

    auto newBuffer = GlobalPool<POOL_NET, uint8_t>::Alloc(capacity, 1);
    memcpy(newBuffer, m_startPos, size);

    if (m_startPos != m_internalBuffer)
GlobalPool<POOL_NET, uint8_t>::Free(m_startPos);

    m_startPos = newBuffer;
    m_endPos = newBuffer + capacity;
    m_curPos = newBuffer + size;
}

//--

END_BOOMER_NAMESPACE_EX(socket)
