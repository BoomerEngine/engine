/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "basePointerRange.h"

namespace base
{

    //--

	/// Typed pointer range
    template< typename T >
    class PointerRange
    {
    public:
        ALWAYS_INLINE PointerRange();
        ALWAYS_INLINE PointerRange(T* ptr, uint64_t length);
        ALWAYS_INLINE PointerRange(T* start, T* end);
        ALWAYS_INLINE PointerRange(const T* ptr, uint64_t length);
        ALWAYS_INLINE PointerRange(const T* start, const T* end);
        ALWAYS_INLINE PointerRange(const PointerRange& other);
        ALWAYS_INLINE PointerRange(PointerRange&& other);
        ALWAYS_INLINE PointerRange& operator=(const PointerRange& other);
        ALWAYS_INLINE PointerRange& operator=(PointerRange&& other);
        ALWAYS_INLINE ~PointerRange();

        //--

        //! get internal data buffer
        ALWAYS_INLINE void* data();

        //! get internal data buffer (read-only)
        ALWAYS_INLINE const void* data() const;

        //! get TYPED data buffer
        ALWAYS_INLINE T* typedData();

        //! get internal data buffer (read-only)
        ALWAYS_INLINE const T* typedData() const;

        //! returns number of bytes in the pointer range
        ALWAYS_INLINE uint64_t dataSize() const;

        //! returns number of ELEMENTS in the pointer range
        ALWAYS_INLINE uint64_t size() const;

        //! last valid index for this range, -1 for empty range
        ALWAYS_INLINE Index lastValidIndex() const;

        //! returns true if the range is empty
        ALWAYS_INLINE bool empty() const;

        //! get the base (untyped) pointer range
        ALWAYS_INLINE BasePointerRange bytes();

        //! get the base (untyped) pointer range
        ALWAYS_INLINE const BasePointerRange bytes() const;

        //! boolean-check, true if we are not an empty range
        ALWAYS_INLINE operator bool() const;

        //--

        //! reset the range to empty range
        ALWAYS_INLINE void reset();

        //! check if this memory range contains other memory range
        ALWAYS_INLINE bool containsRange(BasePointerRange other) const;

        //! check if this memory range contains other memory range
        ALWAYS_INLINE bool containsRange(PointerRange<T> other) const;

        //! check if this memory range contains given pointer
        ALWAYS_INLINE bool containsPointer(const void* ptr) const;

        //--

        //! get indexed element, no checks
        ALWAYS_INLINE T& operator[](Index index);

        //! get indexed element, no checks
        ALWAYS_INLINE const T& operator[](Index index) const;

        //--

        //! call destructor on all elements in the range
        void destroyElements();

        //! default-construct all elements
        void constructElements();

        //! copy-construct all elements from a template
        void constructElementsFrom(const T& elementTemplate);

        //! reverse the elements in the range
        void reverseElements();

        //--

        //! check if the pointer range contains given element value
        template< typename FK >
        bool contains(const FK& key) const;

        //! find first occurrence of element in the range, returns true if index was found
        //! NOTE: valid initial value for the index is -1
        template< typename FK >
        bool findFirst(const FK& key, Index& outFoundIndex) const;

        //! find last occurrence of element in the range, returns true if index was found
        //! NOTE: valid initial values for the index is size()
        template< typename FK >
        bool findLast(const FK& key, Index& outFoundIndex) const;

        //--

        //! replace all existing occurrences of given element with element template, returns number of elements replaced
        template< typename FK >
        Count replaceAll(const FK& item, const T& itemTemplate);

        //! replace FIRST occurrence of given element with element template, returns true if element was replaced, element can be std::moved
        template< typename FK >
        bool replaceFirst(const FK& item, T&& itemTemplate);

        //! replace LAST occurrence of given element with element template, returns true if element was replaced, element can be std::moved
        template< typename FK >
        bool replaceLast(const FK& item, T&& itemTemplate);


        //! replace FIRST occurrence of given element with element template, returns true if element was replaced, element can be std::moved
        template< typename FK >
        bool replaceFirst(const FK& item, const T& itemTemplate);

        //! replace LAST occurrence of given element with element template, returns true if element was replaced, element can be std::moved
        template< typename FK >
        bool replaceLast(const FK& item, const T& itemTemplate);

        //--

        //! sort (inplace) element in the range
        void sort();

        //! sort (inplace) element in the range
        template< typename Pred >
        void sort(const Pred& pred);

        //! find lower_bound insertion point
        //! NOTE: data must be sorted
        template< typename FK >
        Index lowerBound(const FK& key) const;

        //! find upper_bound insertion point
        //! NOTE: data must be sorted
        template< typename FK >
        Index upperBound(const FK& key) const;

        //--

        //! Get iterator to start of the array
        ALWAYS_INLINE ArrayIterator<T> begin();

        //! Get iterator to end of the array
        ALWAYS_INLINE ArrayIterator<T> end();

        //! Get read only iterator to start of the array
        ALWAYS_INLINE ConstArrayIterator<T> begin() const;

        //! Get read only iterator to end of the array
        ALWAYS_INLINE ConstArrayIterator<T> end() const;
        
        //--

        //! "cast to" other type, we must be mega sure about it
        template< typename K >
        ALWAYS_INLINE PointerRange<K> cast() const;

        //--

    protected:
        T* m_start = nullptr;
        T* m_end = nullptr;
    };

    //--

} // base

#include "pointerRange.inl"