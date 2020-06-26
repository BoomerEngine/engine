/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\ansi #]
***/

#pragma once

namespace base
{
    namespace mem
    {

        /// ANSI pass-through allocator
        class AnsiAllocator
        {
        public:
            AnsiAllocator();
            ~AnsiAllocator();

            static const uint32_t DEFAULT_ALIGNMNET = 8;

            // allocate memory block
            void* allocate(PoolID id, size_t size, size_t alignmemnt, const char* fileName, uint32_t fileLine, const char* typeName);

            // deallocate memory block
            void deallocate(void* mem);

            // resize allocated memory block
            void* reallocate(PoolID id, void* mem, size_t newSize, size_t alignmemnt, const char* fileName, uint32_t fileLine, const char* typeName);

            // print memory leaks
            void printLeaks();

            // validate heap status
            void validateHeap(void* freedPtr);
        };

    } // mem

} // base