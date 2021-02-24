/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#include "build.h"
#include "messageKnowledgeBase.h"
#include "messageKnowledgeSync.h"

#include "base/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE(base::net)

//--

IKnowledgeUpdaterSink::~IKnowledgeUpdaterSink()
{}

//---

KnowledgeUpdater::KnowledgeUpdater(MessageKnowledgeBase& base, IKnowledgeUpdaterSink* sink)
    : m_knowledge(base)
    , m_updateSink(sink)
{}

replication::DataMappedID KnowledgeUpdater::mapString(StringView txt)
{
    bool addedNewEntry = false;
    auto id  = m_knowledge.mapString(txt, addedNewEntry);

    if (addedNewEntry)
        m_updateSink->reportNewString(id, txt);

    return id;
}

replication::DataMappedID KnowledgeUpdater::mapPath(StringView path, const char* pathSeparators)
{
    // split path into parts, according to split chars
    InplaceArray<StringView, 10> pathParts;
    path.slice(pathSeparators, false, pathParts);

    // map each part
    replication::DataMappedID pathID = 0;
    for (auto& part : pathParts)
    {
        auto textID  = mapString(part);

        // add part to knowledge base
        bool addedNewEntry = false;
        auto id = m_knowledge.mapPathPart(textID, pathID, addedNewEntry);

        // if that's the first time we see it than send it to remote KB
        if (addedNewEntry)
            m_updateSink->reportNewPath(id, textID, pathID);

        pathID = id;
    }

    return pathID;
}

replication::DataMappedID KnowledgeUpdater::mapObject(const IObject* obj)
{
    return 0;
}

//---

KnowledgeResolver::KnowledgeResolver(const MessageKnowledgeBase& base)
    : m_knowledge(base)
{}

bool KnowledgeResolver::resolveString(const replication::DataMappedID id, IFormatStream& f)
{
    return m_knowledge.resolveString(id, f);
}

bool KnowledgeResolver::resolvePath(const replication::DataMappedID id, const char* pathSeparator, IFormatStream& f)
{
    return m_knowledge.resolvePath(id, pathSeparator, f);
}

bool KnowledgeResolver::resolveObject(const replication::DataMappedID id, ObjectPtr& outObject)
{
    outObject = nullptr;
    return true;
}

//---

END_BOOMER_NAMESPACE(base::net)