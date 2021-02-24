/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "arrayIterator.h"

BEGIN_BOOMER_NAMESPACE(base)

//--

template< typename T >
class PointerRange;

//--

/// Most fundamental concept of a range of memory addresses composed of [start,end> kind of range
/// NOTE: the pointer range is intended to be passed by VALUE as on good compiler it is passed in 2 registers directly
class BasePointerRange
{
public:
    ALWAYS_INLINE BasePointerRange() = default;
    ALWAYS_INLINE ~BasePointerRange() = default;
    ALWAYS_INLINE BasePointerRange(const BasePointerRange& other) = default;
    ALWAYS_INLINE BasePointerRange& operator=(const BasePointerRange& other) = default;
    ALWAYS_INLINE BasePointerRange(BasePointerRange&& other);
    ALWAYS_INLINE BasePointerRange& operator=(BasePointerRange&& other);
    ALWAYS_INLINE BasePointerRange(void* data, uint64_t length);
    ALWAYS_INLINE BasePointerRange(void* data, void* dataEnd);
    ALWAYS_INLINE BasePointerRange(const void* data, uint64_t length);
    ALWAYS_INLINE BasePointerRange(const void* data, const void* dataEnd);

    //--

    //! get internal data buffer
    ALWAYS_INLINE uint8_t* data();

    //! get internal data buffer (read-only)
    ALWAYS_INLINE const uint8_t* data() const;

    //! returns number of bytes in the pointer range
    ALWAYS_INLINE uint64_t dataSize() const;

    //! returns true if the array is empty
    ALWAYS_INLINE bool empty() const;

    //--

    //! reset the range to empty range
    ALWAYS_INLINE void reset();

    //! check if this memory range contains other memory range
    ALWAYS_INLINE bool containsRange(BasePointerRange other) const;

    //! check if this memory range contains given pointer
    ALWAYS_INLINE bool containsPointer(const void* ptr) const;

    //--

    //! zero the memory range
    void zeroBytes();

    //! clear memory range to given value, slower than clearToZero
    void fillBytes(uint8_t value);

    //! compare byte-to-byte raw content of memory range with other memory range
    int compareBytes(BasePointerRange other) const;

    //! byte-reverse the content
    void reverseBytes();

    //! do an endian swap in group of 2-bytes
    void byteswap16();

    //! do an endian swap in group of 4-bytes
    void byteswap32();

    //! do an endian swap in group of 8-bytes
    void byteswap64();

    //--

    //! Get iterator to start of the array
    ALWAYS_INLINE ArrayIterator<uint8_t> begin();

    //! Get iterator to end of the array
    ALWAYS_INLINE ArrayIterator<uint8_t> end();

    //! Get read only iterator to start of the array
    ALWAYS_INLINE ConstArrayIterator<uint8_t> begin() const;

    //! Get read only iterator to end of the array
    ALWAYS_INLINE ConstArrayIterator<uint8_t> end() const;

    //--

    //! compute CRC32 value
    uint32_t crc32() const;

    //! compute CRC64 value
    uint64_t crc64() const;

    //--

    //! "cast" to pointer range of differen type
    // NOTE: this asserts if the alignment or memory size does not match, this is serious stuff and must be done right
    template< typename T >
    ALWAYS_INLINE PointerRange<T> cast() const;

protected:
    uint8_t* m_start = nullptr;
    uint8_t* m_end = nullptr;

    void checkAlignment(uint32_t alignemnt) const;
};

END_BOOMER_NAMESPACE(base)

//--

#include "basePointerRange.inl"