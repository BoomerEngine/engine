/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"
#include "base/containers/include/hashMap.h"

namespace base
{
    namespace net
    {
        //--

        /// shared "knowledge" between both sides of connection, each connection has it's own on each end
        /// NOTE: this class is NOT protected by internal locks, it's up to callers
        class MessageKnowledgeBase : public NoCopy
        {
        public:
            MessageKnowledgeBase();
            ~MessageKnowledgeBase();

            //--

            // map string to ID
            replication::DataMappedID mapString(StringView<char> txt, bool& outNew);

            // map path part
            replication::DataMappedID mapPathPart(const replication::DataMappedID text, const replication::DataMappedID parent, bool& outNew);

            //--

            // remember string
            bool rememberString(const replication::DataMappedID id, StringView<char> txt);

            // remember path part
            bool rememberPathPart(const replication::DataMappedID id, const replication::DataMappedID text, const replication::DataMappedID parent);

            //

            // check if given string ID is valid
            bool validStringId(const replication::DataMappedID id) const;

            // check if given path ID is valid
            bool validPathId(const replication::DataMappedID id) const;

            //

            // get a string from knowledge base
            bool resolveString(const replication::DataMappedID& id, IFormatStream& f) const;

            // get a path from knowledge base
            bool resolvePath(const replication::DataMappedID& id, const char* separator, IFormatStream& f) const;

        private:
            mem::LinearAllocator m_mem; // to hold string memory

            HashMap<StringView<char>, replication::DataMappedID> m_stringMap;
            HashMap<replication::DataMappedID, StringView<char>> m_stringRevMap;
            replication::DataMappedID m_nextStringId;

            struct PathPart
            {
                replication::DataMappedID m_text;
                replication::DataMappedID m_parent;

                uint64_t key() const
                {
                    uint64_t key = 0;
                    memcpy(&key, this, sizeof(PathPart));
                    return key;
                }
            };

            HashMap<uint64_t, replication::DataMappedID> m_pathMap;
            HashMap< replication::DataMappedID, PathPart> m_pathRevMap;
            replication::DataMappedID m_nextPathId;
        };

        //--

    } // msg
} // base