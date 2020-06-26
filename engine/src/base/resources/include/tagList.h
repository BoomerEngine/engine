/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tags #]
***/

#pragma once

#include "base/containers/include/sortedArray.h"
#include "base/containers/include/inplaceArray.h"

namespace base
{
    /// a tag typedef, usually it's just indexed string
    typedef StringID Tag;

    /// low-level container for tags
    typedef SortedArray<Tag, InplaceArray<Tag, 8>> TagContainer;

    /// collection of tags
    class BASE_RESOURCES_API TagList
    {
    public:
        INLINE TagList();
		INLINE TagList(const Array<Tag>& tags);
		INLINE TagList(const Tag* tags, uint32_t numTags);
		INLINE TagList(const TagList& other) = default;
        INLINE TagList(TagList&& other) = default;
        INLINE TagList& operator=(const TagList& other) = default;
        INLINE TagList& operator=(TagList&& other) = default;
        INLINE ~TagList() = default;

        /// clear tag list
        INLINE void clear();

        /// is the tag list empty ?
        INLINE bool empty() const;

        /// do we contain a tag ?
        /// NOTE: empty tag is always contained
        INLINE bool contains(const Tag& tag) const;

        /// add single tag to list, returns true if tag was added, false if it already there
        /// NOTE: adding emtpy tag always fails with false
        INLINE bool add(const Tag& tag);

        /// remove single tag to list, returns true if tag was added, false if it already there
        /// NOTE: removing emtpy tag always fails with false
        INLINE bool remove(const Tag& tag);

        ///--

        /// get number of tags in the set
        INLINE uint32_t size() const;

        /// get n-th tag
        INLINE const Tag operator[](uint32_t index) const;

        /// get hash of the whole set
        INLINE uint64_t hash() const;

        ///--

        /// return empty set
        static const TagList& EMPTY();

        /// get a tag list that is a sum of two tag lists
        static TagList Merge(const TagList& a, const TagList& b);

        /// calculate the difference between set a and b - ie. the tags in A that are NOT in B
        /// NOTE: this is not symetric operation, i.e Difference(A,B) != Difference(B,A);
        static TagList Difference(const TagList& a, const TagList& b);

        /// calculate the set that is intersection of the two sets - ie. tags that are in both sets
        /// NOTE: this is symmetric operation (contrary to the Difference)
        static TagList Intersect(const TagList& a, const TagList& b);

        ///--

        /// test if ANY tag from set B is in this set
        /// NOTE: returns true if the "other" set is empty
        bool containsAny(const TagList& other) const;

        /// test if ALL tags from set B are in this set
        /// NOTE: returns true if the "other" set is empty
        bool containsAll(const TagList& other) const;

        /// test if NO tags from set B are in this set
        /// NOTE: returns true if the "other" set is empty
        bool containsNone(const TagList& other) const;

        //--

        // Custom type implementation requirements
        bool writeBinary(const rtti::TypeSerializationContext& typeContext, stream::IBinaryWriter& stream) const;
        bool writeText(const rtti::TypeSerializationContext& typeContext, stream::ITextWriter& stream) const;
        bool readBinary(const rtti::TypeSerializationContext& typeContext, stream::IBinaryReader& stream);
        bool readText(const rtti::TypeSerializationContext& typeContext, stream::ITextReader& stream);

        // Calculate hash of the data
        void calcHash(CRC64& crc) const;

        //--

        // Compare tag lists (note: checks the hash only)
        INLINE bool operator==(const TagList& other) const;
        INLINE bool operator!=(const TagList& other) const;

    private:
        uint64_t m_hash; // hash of the tags
        TagContainer m_tags; // tags in the set

        void refreshHash();
    };

} // base

#include "tagList.inl"