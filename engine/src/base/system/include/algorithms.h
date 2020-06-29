/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

namespace base
{

    //-----------------------------------------------------------------------------

    /// Class that makes the copy constructor and assignment operator private
    class NoCopy
    {
    public:
        INLINE NoCopy() {};
        INLINE ~NoCopy() {};

    private:
        INLINE NoCopy& operator=( const NoCopy& ) { return *this; }
        INLINE NoCopy( const NoCopy& ) {}
    };

    //-----------------------------------------------------------------------------

    // align value to given any value
    // NOTE: values does not have to be a power of two
    template<typename T>
    INLINE T Align(T offset, T alignment)
    {
        if (alignment > 1)
        {
            T mask = (alignment - 1);
            return ((offset + mask) / alignment) * alignment;
        }
        else
        {
            return offset;
        }
    }

    // align pointer
    template<typename T>
    INLINE T* AlignPtr(T* ptr, size_t alignment)
    {
        return (T*)(void*)Align<size_t>((size_t)ptr, alignment);
    }

    // compute number of groups of given size
    INLINE uint32_t GroupCount(uint32_t count, uint32_t groupSize)
    {
        return (count + (groupSize-1)) / groupSize;
    }

    // get difference (in bytes) between pointers
    template<typename T>
    INLINE ptrdiff_t PtrDirr(T* a, T* b)
    {
        return (char*)a - (char*)b;
    }

    //! Offset a pointer by given amount of bytes
    template <typename T>
    static INLINE T* OffsetPtr(T* ptr, ptrdiff_t offset)
    {
        return (T*)((char*)ptr + offset);
    }

    //! Fash 32-bit hash
    static INLINE uint32_t FastHash32(const void* data, uint32_t size)
    {
        auto ptr  = (const uint8_t*)data;
        auto end  = ptr + size;
        uint32_t hval = UINT32_C(0x811c9dc5); // FNV-1a
        while (ptr< end)
        {
            hval ^= (uint32_t)*ptr++;
            hval *= UINT32_C(0x01000193);
        }
        return hval;
    }

} // base

//-----------------------------------------------------------------------------

template< typename Dest, typename Src >
static INLINE Dest range_cast(Src x)
{
    return (Dest)x;
}

//-----------------------------------------------------------------------------
