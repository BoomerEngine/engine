/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "indexIterator.h"

namespace base
{

    //--

    class ReversedIndexRange;

    //--

    // range of VALID indices
    class BASE_CONTAINERS_API IndexRange
    {
    public:
        ALWAYS_INLINE IndexRange() = default;
        ALWAYS_INLINE IndexRange(const IndexRange& other) = default;
        ALWAYS_INLINE IndexRange& operator=(const IndexRange& other) = default;
        ALWAYS_INLINE IndexRange(IndexRange&& other);
        ALWAYS_INLINE IndexRange& operator=(IndexRange&& other);
        ALWAYS_INLINE IndexRange(Index first, Count count);
        ALWAYS_INLINE ~IndexRange() = default;

        //--

        /// get number of elements in the range
        ALWAYS_INLINE Count size() const;

        /// is the range empty ?
        ALWAYS_INLINE bool empty() const;

        /// boolean check - true if the range is not empty
        ALWAYS_INLINE operator bool() const;

        //--

        /// get first index in the range
        ALWAYS_INLINE Index first() const;

        /// get last index in the range
        ALWAYS_INLINE Index last() const;

        /// does the index range contain given index
        ALWAYS_INLINE bool contains(Index index) const;

        /// does the index range contain given index range
        ALWAYS_INLINE bool contains(IndexRange range) const;

        //--

        ///! get range that goes into opposite direction
        //IndexRange reversed() const;

        //--

        ///! split range at given location, location must be within the range, the split element is NOT included
        ///! NOTE: splitting range that has one element will yieald two empty ranges
        void splitExclude(Index at, IndexRange& outLeft, Index& outRight) const;

        ///! split range at given location, location must be within the range, the split element will be part of the LEFT range
        void splitLeft(Index at, IndexRange& outLeft, Index& outRight) const;

        ///! split range at given location, location must be within the range, the split element will be part of the RIGHT range
        void splitRight(Index at, IndexRange& outLeft, Index& outRight) const;

        //--

        //! Get index iterator to the start of the range
        ALWAYS_INLINE IndexIterator begin() const;

        //! Get read only iterator to end of the array
        ALWAYS_INLINE IndexIterator end() const;

        //--

        //! debug print, prints it like [{} - {}] or "empty" if range is empty
        void print(IFormatStream& f) const;

        //--

        //! get this index range but in reverse
        ALWAYS_INLINE ReversedIndexRange reversed() const;

        //--

    private:
        Index m_first = 0;
        Count m_count = 0;

#ifdef BUILD_RELEASE
        ALWAYS_INLINE void checkNotEmpty() const {};
#else
        void checkNotEmpty() const;
#endif
    };

    //--

    // range of VALID indices - reversed for back to front iteration
    class BASE_CONTAINERS_API ReversedIndexRange
    {
    public:
        ALWAYS_INLINE ReversedIndexRange() = default;
        ALWAYS_INLINE ReversedIndexRange(const ReversedIndexRange& other) = default;
        ALWAYS_INLINE ReversedIndexRange& operator=(const ReversedIndexRange& other) = default;
        ALWAYS_INLINE ReversedIndexRange(ReversedIndexRange&& other);
        ALWAYS_INLINE ReversedIndexRange& operator=(ReversedIndexRange&& other);
        ALWAYS_INLINE ReversedIndexRange(Index last, Count count);
        ALWAYS_INLINE ~ReversedIndexRange() = default;

        //--

        /// get number of elements in the range
        ALWAYS_INLINE Count size() const;

        /// is the range empty ?
        ALWAYS_INLINE bool empty() const;

        /// boolean check - true if the range is not empty
        ALWAYS_INLINE operator bool() const;

        //--

        /// get first index in the range
        ALWAYS_INLINE Index first() const;

        /// get last index in the range
        ALWAYS_INLINE Index last() const;

        /// does the index range contain given index
        ALWAYS_INLINE bool contains(Index index) const;

        /// does the index range contain given index range
        ALWAYS_INLINE bool contains(ReversedIndexRange range) const;

        //--

        //! Get index iterator to the start of the range
        ALWAYS_INLINE ReversedIndexIterator begin() const;

        //! Get read only iterator to end of the array
        ALWAYS_INLINE ReversedIndexIterator end() const;

        //--

        //! debug print, prints it like [{} - {}] or "empty" if range is empty
        void print(IFormatStream& f) const;

        //--

        //! get this index range but in reverse
        ALWAYS_INLINE IndexRange reversed() const;

    private:
        Index m_last = 0;
        Count m_count = 0;

#ifdef BUILD_RELEASE
        ALWAYS_INLINE void checkNotEmpty() const {};
#else
        void checkNotEmpty() const;
#endif
    };

} // base

#include "indexRange.inl"