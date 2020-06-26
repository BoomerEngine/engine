/***
* Boomer Engine v4
* Written by Łukasz "Krawiec" Krawczyk
*
* [#filter: raw #]
***/

#pragma once

#include "address.h"

namespace base
{
    namespace socket
    {
        //--

        // a helper for allocating network block
        class BASE_SOCKET_API BlockAllocator : public NoCopy
        {
        public:
            BlockAllocator();
            ~BlockAllocator();

            // allocate block with specific usable size
            Block* alloc(uint32_t size);

            /// build a single block from parts
            Block* build(std::initializer_list<BlockPart> blocks);

            /// build a single block from parts
            Block* build(const Array<BlockPart>& blocks);

        private:
            void releaseBlock(Block* block);

            std::atomic<uint32_t> m_numBlocks;
            std::atomic<uint32_t> m_numBytes;
            std::atomic<uint32_t> m_maxBlocks;
            std::atomic<uint32_t> m_maxBytes;

            friend class Block;
        };

        //--

    } // socket
} // base
