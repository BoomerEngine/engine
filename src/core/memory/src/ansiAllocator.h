/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\ansi #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(mem)

/// ANSI pass-through allocator
class AnsiAllocator
{
public:
    static const uint32_t DEFAULT_ALIGNMNET = 8;

    // allocate memory block
    static void* allocate(PoolTag id, size_t size, size_t alignmemnt, const char* typeName);

    // deallocate memory block
    static void deallocate(void* mem);

    // resize allocated memory block
    static void* reallocate(PoolTag id, void* mem, size_t newSize, size_t alignmemnt, const char* typeName);

    // print memory leaks
    static void printLeaks();

    // validate heap status
    static void validateHeap(void* freedPtr);
};

END_BOOMER_NAMESPACE_EX(mem)
