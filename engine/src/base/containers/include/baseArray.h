/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers\dynamic #]
***/

#pragma once

namespace base
{

	/// raw wrapper for basic array operations, used mostly by the RTTI accessors
    class BaseArray
    {
    public:
        BaseArray() = default;
        ~BaseArray();

        //! get internal data buffer
        INLINE void *data();

        //! get internal data buffer (read-only)
        INLINE const void *data() const;

        //! returns true if the array is empty
        INLINE bool empty() const;

        //! returns true if the array is empty
        INLINE bool full() const;

        //! returns number of elements in the array
        INLINE uint32_t size() const;

        //! returns maximum number of elements array can hold without resizing
        INLINE uint32_t capacity() const;

        //! adjust size - pro mode
        INLINE void adjustSize(uint32_t newSize);

        //--

        //! get pointer to n-th element (read-only)
        INLINE const void *elementPtr(uint32_t index, uint32_t elementSize) const;

        //! get pointer to n-th element
        INLINE void *elementPtr(uint32_t index, uint32_t elementSize);

        //--

        //! change size of the array, basically only changes the member + some validation, returns old size
        //! NOTE: caller takes full responsibility
		BASE_CONTAINERS_API uint32_t changeSize(uint32_t newSize);

        //! change the capacity of the array, basically just reallocates the data buffer, returns old capacity
		BASE_CONTAINERS_API uint32_t changeCapacity(mem::PoolID id, uint32_t newCapacity, uint32_t elementSize, uint32_t elementAlignment, const char* debugTypeName);

        //--

		//! is the data buffer owned by the array ?
        INLINE bool isLocal() const;

		//! can the array be resized AT ALL ?
        INLINE bool canResize() const;

		//! make the data buffer owned by the array
		BASE_CONTAINERS_API void makeLocal(mem::PoolID id, uint32_t minimumCapacity, uint32_t elementSize, uint32_t elementAlignment, const char* debugTypeName);

        //--

        //! calculate next capacity for current capacity and element size
		BASE_CONTAINERS_API static uint32_t CalcNextCapacity(uint32_t currentCapacity, uint32_t elementSize);

    protected:
		BaseArray(void* externalBuffer, uint32_t externalCapacity); // set to predefined buffer

		BASE_CONTAINERS_API void checkIndex(uint32_t index) const;
		BASE_CONTAINERS_API void checkIndexRange(uint32_t index, uint32_t count) const;
        BASE_CONTAINERS_API void suckFrom(BaseArray& arr);

    private:
        static const uint32_t FLAG_EXTERNAL = 0x80000000;
        static const uint32_t FLAG_FIXED_CAPACITY = 0x40000000; // can't ever resize
        static const uint32_t FLAG_MASK = 0x3FFFFFFF;

        void* m_data = nullptr;
        uint32_t m_size = 0;
        uint32_t m_capacityAndFlags = 0;
    };

} // base

#include "baseArray.inl"