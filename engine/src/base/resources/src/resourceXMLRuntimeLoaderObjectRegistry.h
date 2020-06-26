/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\xml #]
***/

#pragma once

#include "base/xml/include/public.h"
#include "base/containers/include/hashMap.h"
#include "base/containers/include/array.h"

namespace base
{
    namespace res
    {
        namespace xml
        {
            namespace prv
            {

                /// hold loaded objects
                class LoaderObjectRegistry
                {
                public:
                    void addObject(const ObjectPtr& object, StringView<char> id, base::xml::NodeID nodeID, bool isRoot);

                    void allObjects(Array< ObjectPtr >& outObjects) const;
                    void rootObjects(Array< ObjectPtr >& outObjects) const;

                    bool resolveByObjectID(StringView<char> id, ObjectPtr& outObject) const;
                    bool resolveByNodeID(const base::xml::NodeID id, ObjectPtr& outObject) const;

                    struct Object
                    {
                        StringView<char> id; // only if known
                        ClassType classType = nullptr; // resolved object class
                        base::xml::NodeID dataNodeID; // source node with the object definition
                        ObjectPtr object; // created object
                        bool root = false; // is this a root object
                    };

                    typedef Array< Object > TObjects;
                    INLINE const TObjects& objects() const
                    {
                        return m_objects;
                    }

                private:
                    TObjects m_objects;

                    typedef HashMap< StringView<char>, uint32_t > TObjectMap;
                    TObjectMap m_objectIdMap;

                    typedef HashMap< base::xml::NodeID, uint32_t > TObjectNodeMap;
                    TObjectNodeMap m_objectNodeMap;

                };

            } // prv
        } // xml
    } // res
} // base