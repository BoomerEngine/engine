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
#include "base/object/include/streamTextWriter.h"
#include "base/object/include/streamTextReader.h"
#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"
#include "base/containers/include/inplaceArray.h"

namespace base
{
    RTTI_BEGIN_CUSTOM_TYPE(TagList);
        RTTI_TYPE_TRAIT().zeroInitializationValid();
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
            tagsAdded |= ret.m_tags.insert(tag);
        
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

    bool TagList::writeBinary(const rtti::TypeSerializationContext& typeContext, stream::IBinaryWriter& stream) const
    {
        for (auto& tag : m_tags)
            stream.writeName(tag);
        stream.writeName(Tag::EMPTY());
        return true;
    }

    bool TagList::writeText(const rtti::TypeSerializationContext& typeContext, stream::ITextWriter& stream) const
    {
        StringBuilder ret;

        for (auto& tag : m_tags)
        {
            if (!ret.empty())
                ret.append(",");
            ret.append(tag.c_str());
        }

        stream.writeValue(ret.toString());
        return true;
    }

    bool TagList::readBinary(const rtti::TypeSerializationContext& typeContext, stream::IBinaryReader& stream)
    {
		InplaceArray<Tag, 10> tags;
        while (1)
        {
            Tag tag = stream.readName();
            if (tag.empty())
                break;

			tags.pushBack(tag);
        }

		m_tags = std::move(TagContainer(tags));
        refreshHash();
    	
        return true;
    }

    bool TagList::readText(const rtti::TypeSerializationContext& typeContext, stream::ITextReader& stream)
    {
        clear();

        StringView<char> txt;
        if (!stream.readValue(txt))
        {
            TRACE_WARNING("Unable to load string data when string data was expected");
            return true; // let's try to continue
        }

        // split into tags
        InplaceArray<StringView<char>, 32> tags;
        txt.slice(";", false, tags);

        // process the data
        for (auto& tag : tags)
            add(tag);

        // loaded
        return true;
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
