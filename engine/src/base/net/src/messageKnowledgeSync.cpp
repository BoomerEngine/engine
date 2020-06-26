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
#include "messageObjectRepository.h"

#include "base/containers/include/inplaceArray.h"

namespace base
{
    namespace net
    {

        //--

        IKnowledgeUpdaterSink::~IKnowledgeUpdaterSink()
        {}

        //---

        KnowledgeUpdater::KnowledgeUpdater(MessageKnowledgeBase& base, IKnowledgeUpdaterSink* sink, MessageObjectRepository& objectRepository)
            : m_knowledge(base)
            , m_updateSink(sink)
            , m_objectRepository(objectRepository)
        {}

        replication::DataMappedID KnowledgeUpdater::mapString(StringView<char> txt)
        {
            bool addedNewEntry = false;
            auto id  = m_knowledge.mapString(txt, addedNewEntry);

            if (addedNewEntry)
                m_updateSink->reportNewString(id, txt);

            return id;
        }

        replication::DataMappedID KnowledgeUpdater::mapPath(StringView<char> path, const char* pathSeparators)
        {
            // split path into parts, according to split chars
            InplaceArray<StringView<char>, 10> pathParts;
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
            return m_objectRepository.findObjectId(obj);
        }

        //---

        KnowledgeResolver::KnowledgeResolver(const MessageKnowledgeBase& base, const MessageObjectRepository& objectRepository)
            : m_knowledge(base)
            , m_objectRepository(objectRepository)
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
            if (!id)
            {
                outObject = nullptr;
                return true;
            }

            auto obj = m_objectRepository.resolveObject(id);
            if (!obj)
                return false;

            outObject = std::move(obj);
            return true;
        }

        //---

    } // msg
} // base

