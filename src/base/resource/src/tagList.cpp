/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tags #]
***/

#include "build.h"
#include "tagList.h"

#include "base/xml/include/xmlUtils.h"
#include "base/xml/include/xmlDocument.h"
#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"
#include "base/containers/include/inplaceArray.h"
#include "base/object/include/streamOpcodeWriter.h"
#include "base/object/include/streamOpcodeReader.h"

namespace base
{
    RTTI_BEGIN_CUSTOM_TYPE(TagList);
        RTTI_TYPE_TRAIT().zeroInitializationValid();
        RTTI_BIND_NATIVE_BINARY_SERIALIZATION(TagList);
        //RTTI_BIND_NATIVE_PRINT(TagList);
    RTTI_END_TYPE();

    TagList GEmptyTagList;

    const TagList& TagList::EMPTY()
    {
        return GEmptyTagList;
    }

    TagList TagList::Merge(const TagList& a, const TagList& b)
    {
        if (a.empty())
            return b;
        else if (b.empty())
            return a;

        TagList ret(a);

        // TODO: since both lists are sorted arrays this can be optimized

        bool tagsAdded = false;
        for (auto& tag : b.m_tags)
        {
            if (!ret.m_tags.contains(tag))
            {
                ret.m_tags.pushBack(tag);
                tagsAdded = true;
            }
        }
        
        if (tagsAdded)
            ret.refreshHash();

        return ret;
    }

    TagList TagList::Difference(const TagList& a, const TagList& b)
    {
        // if the set to subtract is empty than we end up with the A set
        if (b.empty())
            return a;

        // if the input set is empty than the result will also be
        if (a.empty())
            return EMPTY();

        // subtracting a set from itself yields empty set
        if (a.m_hash == b.m_hash)
            return EMPTY();

        TagList ret(a);

        // TODO: since both lists are sorted arrays this can be optimized

        bool tagsRemoved = false;
        for (auto& tag : b.m_tags)
            tagsRemoved = ret.m_tags.remove(tag);

        if (tagsRemoved)
            ret.refreshHash();

        return ret;
    }

    TagList TagList::Intersect(const TagList& a, const TagList& b)
    {
        // if any of the set is empty the result is empty
        if (a.empty() || b.empty())
            return EMPTY();

        // if the sets are the same the result if the same 
        if (a.m_hash == b.m_hash)
            return a;

		InplaceArray<Tag, 10> temp;

        for (auto& tag : a.m_tags)
            if (b.contains(tag))
				temp.pushBack(tag);

        return TagList(temp);
    }

    bool TagList::containsAny(const TagList& other) const
    {
        if (other.empty())
            return true;

        for (auto& tag : other.m_tags)
            if (contains(tag))
                return true;

        return false;
    }

    bool TagList::containsAll(const TagList& other) const
    {
        if (other.empty())
            return true;

        for (auto& tag : other.m_tags)
            if (!contains(tag))
                return false;

        return true;
    }

    bool TagList::containsNone(const TagList& other) const
    {
        if (other.empty())
            return true;

        for (auto& tag : other.m_tags)
            if (contains(tag))
                return false;

        return true;
    }

    void TagList::writeBinary(stream::OpcodeWriter& stream) const
    {
        stream.beginArray(m_tags.size());
        for (auto& tag : m_tags)
            stream.writeStringID(tag);
        stream.endArray();
    }

    void TagList::readBinary(stream::OpcodeReader& stream)
    {
        uint32_t count = 0;
        stream.enterArray(count);

        m_tags.reset();
        m_tags.reserve(count);

        for (uint32_t i=0; i<count; ++i)
        {
            if (auto tag = stream.readStringID())
    			m_tags.pushBack(tag);
        }

        refreshHash();
    }

    void TagList::refreshHash()
    {
        m_hash = 0;

        base::CRC64 crc;
        calcHash(crc);

        m_hash = crc.crc();
    }

    void TagList::calcHash(CRC64& crc) const
    {
        for (auto& tag : m_tags)
            crc << tag;
    }

    bool TagList::operator==(const TagList& other) const
    {
        return m_hash == other.m_hash;
    }

    bool TagList::operator!=(const TagList& other) const
    {
        return m_hash != other.m_hash;
    }

} // base
