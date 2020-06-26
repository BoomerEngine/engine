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

namespace base
{
    namespace socket
    {
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
                MemFree(m_startPos);

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

            auto newBuffer  = (uint8_t*) MemAlloc(POOL_NET, capacity, 1);
            memcpy(newBuffer, m_startPos, size);

            if (m_startPos != m_internalBuffer)
                MemFree(m_startPos);

            m_startPos = newBuffer;
            m_endPos = newBuffer + capacity;
            m_curPos = newBuffer + size;
        }

        //--

    } // socket
} // base