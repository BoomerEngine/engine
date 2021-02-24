/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: raw #]
***/

#include "build.h"
#include "block.h"
#include "blockAllocator.h"

BEGIN_BOOMER_NAMESPACE(base::socket)

//--

void Block::release()
{
    m_allocator->releaseBlock(this);
}

void Block::shrink(uint32_t skipFront, uint32_t newSize /*= INDEX_MAX*/)
{
    ASSERT(skipFront <= m_currentSize);
    m_currentSize -= skipFront;
    m_ptr += skipFront;

    if (newSize != INDEX_MAX)
    {
        ASSERT(newSize <= m_currentSize);
        m_currentSize = newSize;
    }
}

//--

END_BOOMER_NAMESPACE(base::socket)
