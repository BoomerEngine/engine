/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tags #]
***/

#pragma once

namespace base
{
    INLINE TagList::TagList()
        : m_hash(0)
    {}

	INLINE TagList::TagList(const Array<Tag>& tags)
		: m_tags(tags)
    {
		refreshHash();	    
    }
	
	INLINE TagList::TagList(const Tag* tags, uint32_t numTags)
		: m_tags(tags, numTags)
    {
		refreshHash();	    
    }

    INLINE void TagList::clear()
    {
        m_tags.clear();
    }

    INLINE bool TagList::empty() const
    {
        return m_tags.empty();
    }

    INLINE bool TagList::contains(const Tag& tag) const
    {
        return m_tags.contains(tag);
    }

    INLINE bool TagList::add(const Tag& tag)
    {
        if (m_tags.contains(tag))
            return false;
        m_tags.pushBack(tag);
        return true;
    }

    INLINE bool TagList::remove(const Tag& tag)
    {
        return m_tags.remove(tag);
    }

    INLINE uint32_t TagList::size() const
    {
        return m_tags.size();
    }

    INLINE const Tag TagList::operator[](uint32_t index) const
    {
        return m_tags[index];
    }

    INLINE uint64_t TagList::hash() const
    {
        return m_hash;
    }

} // scene
