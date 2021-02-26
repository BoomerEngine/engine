/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#include "build.h"
#include "messageKnowledgeBase.h"

BEGIN_BOOMER_NAMESPACE_EX(net)

//---

MessageKnowledgeBase::MessageKnowledgeBase()
    : m_mem(POOL_NET)
    , m_nextStringId(1)
    , m_nextPathId(1)
{
    m_stringMap.reserve(1024);
    m_stringRevMap.reserve(1024);
    m_pathMap.reserve(1024);
    m_pathRevMap.reserve(1024);
}

MessageKnowledgeBase::~MessageKnowledgeBase()
{
}

bool MessageKnowledgeBase::validStringId(const replication::DataMappedID id) const
{
    if (!id)
        return true;

    return m_stringRevMap.contains(id);
}

bool MessageKnowledgeBase::validPathId(const replication::DataMappedID id) const
{
    if (!id)
        return true;

    return m_pathRevMap.contains(id);
}

replication::DataMappedID MessageKnowledgeBase::mapString(StringView txt, bool& outNew)
{
    if (!txt)
        return 0;

    // find existing string
    replication::DataMappedID id = 0;
    if (m_stringMap.find(txt, id))
    {
        outNew = false;
        return id;
    }

    // allocate under a new ID
    id = m_nextStringId++;
    rememberString(id, txt);

    outNew = true;
    return id;
}

replication::DataMappedID MessageKnowledgeBase::mapPathPart(const replication::DataMappedID text, const replication::DataMappedID parent, bool& outNew)
{
    if (!text)
        return 0;

    PathPart part;
    part.m_text = text;
    part.m_parent = parent;

    // find existing string
    replication::DataMappedID id = 0;
    if (m_pathMap.find(part.key(), id))
    {
        outNew = false;
        return id;
    }

    // add new one
    id = m_nextPathId++;
    rememberPathPart(id, text, parent);

    // we need to send the update
    outNew = true;
    return id;
}

//---

bool MessageKnowledgeBase::rememberString(const replication::DataMappedID id, StringView txt)
{
    StringView existingString;
    if (m_stringRevMap.find(id, existingString))
    {
        if (0 != existingString.cmp(txt))
        {
            TRACE_ERROR("KnowledgeCollision: ID {} already registered as '{}', trying to remember as '{}'", id, existingString, txt);
            return false;
        }

        return true;
    }

    // copy string
    auto txtCopy  = StringView(m_mem.strcpy(txt.data(), txt.length()));

    // create new entry
    m_stringRevMap[id] = txtCopy;
    m_stringMap[txtCopy] = id;
    TRACE_INFO("New string '{}' added to KnowledgeDB at index {}", txt, id);
    return true;
}

bool MessageKnowledgeBase::rememberPathPart(const replication::DataMappedID id, const replication::DataMappedID text, const replication::DataMappedID parent)
{
    // find existing string
    PathPart part;
    if (m_pathRevMap.find(id, part))
    {
        if (part.m_text != text || part.m_parent != parent)
        {
            TRACE_ERROR("KnowledgeCollision: ID {} is already registered as {}/{}, trying to remember as {}/{}", id, part.m_text, part.m_parent, text, parent);
            return false;
        }

        return true;
    }

    part.m_text = text;
    part.m_parent = parent;

    m_pathMap[part.key()] = id;
    m_pathRevMap[id] = part;
    TRACE_INFO("New path {} on {} added to KnowledgeDB at index {}", part.m_text, part.m_parent, id);

    return true;
}

//---

bool MessageKnowledgeBase::resolveString(const replication::DataMappedID& id, IFormatStream& f) const
{
    // empty string
    if (!id)
        return true;

    // get the text
    StringView txt;
    if (!m_stringRevMap.find(id, txt))
        return false;

    f << txt;
    return true;
}

bool MessageKnowledgeBase::resolvePath(const replication::DataMappedID& id, const char* separator, IFormatStream& f) const
{
    // empty path
    if (!id)
        return true;

    // get path entry
    auto entry  = m_pathRevMap.find(id);
    if (!entry)
        return false;

    // resolve parent
    if (entry->m_parent != 0)
    {
        if (!resolvePath(entry->m_parent, separator, f))
            return false;
        f << separator;
    }

    // add text
    if (!resolveString(entry->m_text, f))
        return false;

    // valid path entry
    return true;
}

//---

END_BOOMER_NAMESPACE_EX(net)
