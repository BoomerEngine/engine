/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\xml\loader #]
***/

#include "build.h"

#include "base/xml/include/xmlUtils.h"
#include "base/xml/include/xmlDocument.h"

#include "resource.h"
#include "resourceLoader.h"

#include "resourceXMLRuntimeLoader.h"
#include "resourceXMLRuntimeLoaderObjectRegistry.h"
#include "resourceXMLRuntimeLoaderReferenceResolver.h"
#include "resourceXMLRuntimeLoaderTextReader.h"

namespace base
{
    namespace res
    {
        namespace xml
        {
            namespace prv
            {

                LoaderState::LoaderState()
                    : m_selectiveLoadingClass(nullptr)
                    , m_selectiveLoadingObjectCreated(false)
                {
                }

                LoaderState::~LoaderState()
                {
                }

                LoaderStatePtr LoaderState::LoadContent(const Buffer& xmlData, const stream::LoadingContext& context)
                {
                    PC_SCOPE_LVL1(LoadContent);

                    // parse document
                    auto doc = base::xml::LoadDocument(base::xml::ILoadingReporter::GetDefault(), xmlData);
                    if (!doc)
                    {
                        TRACE_ERROR("Failed to parse XML from buffer");
                        return nullptr;
                    }

                    // parent all objects to the specified parent object
                    auto rootObject = context.m_parent;

                    auto ret = CreateSharedPtr<LoaderState>();
                    ret->m_objectRegistry = CreateUniquePtr<LoaderObjectRegistry>();
                    ret->m_referenceResolver = CreateUniquePtr<LoaderReferenceResolver>(context.m_resourceLoader);
                    ret->m_selectiveLoadingClass = context.m_selectiveLoadingClass;
                    ret->m_resourceLoader = context.m_resourceLoader;

                    // create the object wrappers, scans the XML
                    // NOTE: for safety we don't allow to load XMLs with missing object classes         
                    if (!ret->createObjects(*doc, doc->root(), rootObject, rootObject))
                    {
                        TRACE_WARNING("Some objects were not created during deserialiation");
                    }

                    // load the object content
                    if (!ret->loadObjects(*doc))
                    {
                        TRACE_ERROR("Object loading failed");
                        return nullptr;
                    }

                    // return state for further processing
                    return ret;
                }

                bool LoaderState::createObjects(const base::xml::IDocument& doc, base::xml::NodeID id, const ObjectPtr& parentObject, const ObjectPtr& rootObject)
                {
                    PC_SCOPE_LVL1(CreateObjects);

                    bool isValid = true;

                    auto childNodeId = doc.nodeFirstChild(id);
                    while (childNodeId != 0)
                    {
                        auto nodeName = doc.nodeName(childNodeId);
                        if (nodeName == "object")
                        {
                            ObjectPtr createdObject;
                            if (createSingleObject(doc, childNodeId, parentObject, createdObject))
                            {
                                // create entry in the registry
                                if (createdObject)
                                {
                                    auto objectId = doc.nodeAttributeOfDefault(childNodeId, "id");
                                    bool isRoot = (parentObject == rootObject);
                                    m_objectRegistry->addObject(createdObject, objectId, childNodeId, isRoot);
                                }

                                // recurse down the XML but change the parent object so all found sub-objects will be parented to this object
                                isValid &= createObjects(doc, childNodeId, createdObject, rootObject);
                            }
                            else
                            {
                                TRACE_ERROR("Failed to create object from node at '{}'", doc.nodeLocationInfo(childNodeId));
                                isValid = false;
                            }
                        }
                        else
                        {
                            // this was not an object node continue exploring
                            isValid &= createObjects(doc, childNodeId, parentObject, rootObject);
                        }

                        childNodeId = doc.nodeSibling(childNodeId);
                    }

                    return isValid;
                }

                bool LoaderState::createSingleObject(const base::xml::IDocument& doc, base::xml::NodeID objectNodeID, const ObjectPtr& parentObject, ObjectPtr& outCreatedObject)
                {
                    // the "class" filed must be specified for valid objects
                    auto className = doc.nodeAttributeOfDefault(objectNodeID, "class");
                    if (className.empty())
                    {
                        TRACE_ERROR("Object node has no class name specified");
                        return false;
                    }

                    // find the RTTI class with given name
                    auto classType  = rtti::TypeSystem::GetInstance().findClass(StringID(className));
                    if (!classType)
                    {
                        TRACE_ERROR("Object node uses unknown class '{}'", className);
                        return false;
                    }

                    // handle selective loading case
                    if (m_selectiveLoadingClass)
                    {
                        // we don't want this object in selective loading
                        if (!classType->is(m_selectiveLoadingClass) || m_selectiveLoadingObjectCreated)
                            return true;
                        m_selectiveLoadingObjectCreated = true;
                    }

                    // create the object
                    ObjectPtr obj;
                    if (classType->isAbstract())
                    {
                        TRACE_ERROR("Class '{}' become abstract and the instance of serialized object cannot be created", className);
                        return false;
                    }
                    else
                    {
                        obj = classType->create<IObject>();
                        if (!obj)
                        {
                            TRACE_ERROR("Object node of class '{}' cannot be created", className);
                            return false;
                        }
                    }

                    // parent the object
                    obj->parent(parentObject);

                    // done
                    outCreatedObject = obj;
                    return true;
                }

                bool LoaderState::loadObjects(const base::xml::IDocument& doc)
                {
                    PC_SCOPE_LVL1(LoadObjects);

                    bool isValid = true;

                    for (auto& obj : m_objectRegistry->objects())
                    {
                        if (obj.object)
                        {
                            isValid &= loadSingleObject(doc, obj.dataNodeID, obj.object);
                        }
                    }

                    return isValid;
                }

                bool LoaderState::loadSingleObject(const base::xml::IDocument& doc, base::xml::NodeID objectNodeID, const ObjectPtr& objectPtr)
                {
                    prv::TextReader reader(doc, objectNodeID, *m_objectRegistry, *m_referenceResolver);

                    // call internal loading method
                    if (!objectPtr->onReadText(reader))
                    {
                        TRACE_ERROR("Internal loading failed for object '{}' at '{}'",
                            objectPtr->cls()->name().c_str(), doc.nodeLocationInfo(objectNodeID).c_str());
                        return false;
                    }

                    return true;
                }

                void LoaderState::postLoad()
                {
                    PC_SCOPE_LVL1(PostLoad);

                    auto& objects = m_objectRegistry->objects();
                    for (auto i=objects.lastValidIndex(); i>=0; --i)
                        objects[i].object->onPostLoad();
                }

                void LoaderState::extractResults(stream::LoadingResult& result) const
                {
                    m_objectRegistry->rootObjects(result.m_loadedRootObjects);
                }

            } // prv
        } // xml
    } // res
} // base

