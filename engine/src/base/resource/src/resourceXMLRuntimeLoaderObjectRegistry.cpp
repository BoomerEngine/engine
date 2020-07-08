/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\xml\loader #]
***/

#include "build.h"
#include "resourceXMLRuntimeLoaderObjectRegistry.h"

namespace base
{
    namespace res
    {
        namespace xml
        {
            namespace prv
            {

                void LoaderObjectRegistry::addObject(const ObjectPtr& object, StringView<char> id, base::xml::NodeID nodeID, bool isRoot)
                {
                    auto index = m_objects.size();

                    Object info;
                    info.id = id;
                    info.classType = object->cls();
                    info.dataNodeID = nodeID;
                    info.root = isRoot;
                    info.object = object;
                    m_objects.pushBack(info);

                    if (!info.id.empty())
                        m_objectIdMap.set(info.id, index);

                    m_objectNodeMap.set(info.dataNodeID, index);
                }

                void LoaderObjectRegistry::allObjects(Array< ObjectPtr >& outObjects) const
                {
                    outObjects.reserve(m_objects.size());

                    for (auto& it : m_objects)
                        outObjects.pushBack(it.object);
                }

                void LoaderObjectRegistry::rootObjects(Array< ObjectPtr >& outObjects) const
                {
                    outObjects.reserve(m_objects.size());

                    for (auto& it : m_objects)
                        if (it.root)
                            outObjects.pushBack(it.object);
                }

                bool LoaderObjectRegistry::resolveByObjectID(StringView<char> id, ObjectPtr& outObject) const
                {
                    uint32_t index = 0;
                    if (m_objectIdMap.find(id, index))
                    {
                        outObject = m_objects[index].object;
                        return true;
                    }

                    return false;
                }

                bool LoaderObjectRegistry::resolveByNodeID(const base::xml::NodeID id, ObjectPtr& outObject) const
                {
                    uint32_t index = 0;
                    if (m_objectNodeMap.find(id, index))
                    {
                        outObject = m_objects[index].object;
                        return true;
                    }

                    return false;
                }

            } // prv
        } // xml
    } // res
} // base