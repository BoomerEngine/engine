/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\text #]
***/

#include "build.h"
#include "resourceGeneralTextLoader.h"
#include "resourceLoader.h"
#include "resourceXMLLoader.h"

#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/inplaceArray.h"
#include "base/containers/include/hashMap.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/resource/include/resource.h"
#include "simpleTextFile.h"

#include <stdarg.h>

namespace base
{
    namespace res
    {
        namespace text
        {

            ///---
            
            mem::PoolID POOL_TEXT_LOADER("Engine.TextLoader");

            ///--

            namespace prv
            {

                // helper class to contain all the loading
                class LoadingWorker : public base::NoCopy
                {
                public:
                    LoadingWorker(const stream::LoadingContext& context, stream::LoadingResult& result)
                        : m_context(context)
                        , m_result(result)
                    {
                    }

                    bool createSingleObject(const text::ParameterNode* node, ClassType classType, const ObjectPtr& parent, ObjectPtr& outCreatedObject)
                    {
                        if (classType->isAbstract())
                        {
                            TRACE_ERROR("Class '{}' become abstract and the instance of serialized object cannot be created", classType->name());
                            return false;
                        }
                        else
                        {
                            outCreatedObject = classType->create<IObject>();
                            if (!outCreatedObject)
                            {
                                TRACE_ERROR("Object node of class '{}' cannot be created", classType->name());
                                return false;
                            }
                        }

                        // parent the object
                        outCreatedObject->parent(parent);

                        // created
                        return true;
                    }

                    bool createSelectiveObject(ClassType selectiveLoadingClass, const text::ParameterNode* node, Array< ObjectPtr >& outRootObjects)
                    {
                        // is this an object ? objects have the "class" property
                        ObjectPtr createdObject;
                        if (auto className = node->keyValue("class"))
                        {
                            // do this only for true objects, not resource references
                            if (!node->keyValue("path") && !node->keyValue("absPath"))
                            {
                                // get the class
                                auto objectClass = RTTI::GetInstance().findClass(base::StringID(className));
                                if (objectClass == selectiveLoadingClass)
                                {
                                    if (!createSingleObject(node, objectClass, nullptr, createdObject))
                                        return false;
                                }
                            }
                        }

                        // bind to services
                        if (createdObject)
                        {
                            m_objectsAtNodes[node] = createdObject;
                            outRootObjects.pushBack(createdObject);
                            return true;
                        }

                        // scan children
                        for (auto child = node->children(); child; ++child)
                            if (createSelectiveObject(selectiveLoadingClass, child.get(), outRootObjects))
                                return true;

                        return false;
                    }

                    void createObjects(const text::ParameterNode* node, const ObjectPtr& parent, const ObjectPtr& externalParent, Array< ObjectPtr >& outRootObjects)
                    {
                        bool canCreateChildren = true;

                        // is this an object ? objects have the "class" property
                        ObjectPtr createdObject;
                        if (auto className = node->keyValue("class"))
                        {
                            // do this only for true objects, not resource references
                            if (!node->keyValue("path") && !node->keyValue("absPath"))
                            {
                                // get the class
                                auto objectClass = RTTI::GetInstance().findClass(base::StringID(className));
                                if (!objectClass)
                                {
                                    auto objectName = node->keyValue("name");
                                    TRACE_ERROR("Missing class '{}' referenced by object '{}'", className, objectName);
                                    canCreateChildren = false;
                                }
                                // create the object
                                else if (!createSingleObject(node, objectClass, parent, createdObject))
                                {
                                    // we failed to create object, do not continue to children
                                    canCreateChildren = false;
                                }
                                // we didn't fail
                                else
                                {
                                    // add to export list
                                    if (createdObject && parent == externalParent)
                                        outRootObjects.pushBack(createdObject);

                                    // map
                                    m_objectsAtNodes[node] = createdObject;

                                    // do we have a reference ID ?
                                    auto objectID = node->keyValueRead<int>("id", -1);
                                    if (objectID != -1)
                                        m_objectsById[objectID] = createdObject;
                                }
                            }
                        }

                        // scan children
                        if (canCreateChildren)
                            for (auto child = node->children(); child; ++child)
                                createObjects(child.get(), createdObject ? createdObject : parent, externalParent, outRootObjects);
                    }

                    void translatePath(bool absolutePath, StringView<char> resourcePath, StringBuf& outPath)
                    {
                        if (!m_context.m_baseReferncePath.empty() && !absolutePath)
                        {
                            outPath = TempString("{}{}", m_context.m_baseReferncePath, resourcePath);
                        }
                        else
                        {
                            outPath = StringBuf(resourcePath);
                        }
                    }

                    void loadResource(StringView<char> resourcePath, SpecificClassType<IResource> resourceClass, ResourceHandle& outLoadedResource)
                    {
                        ResourceKey key(ResourcePath(resourcePath), resourceClass);
                        if (!m_loadedResources.find(key, outLoadedResource))
                        {
                            if (m_context.m_resourceLoader != nullptr)
                            {
                                outLoadedResource = m_context.m_resourceLoader->loadResource(key);
                                if (!outLoadedResource)
                                {
                                    TRACE_WARNING("Required resource '{}' could not be loaded", key);
                                }
                            }
                            else
                            {
                                TRACE_WARNING("Required resource '{}' could not be loaded because there's no loader", key);
                            }
                            m_loadedResources[key] = outLoadedResource;
                        }
                    }

                    void postLoad()
                    {
                        PC_SCOPE_LVL2(PostLoad);

                        for (auto i=m_objectsAtNodes.values().lastValidIndex(); i>=0; --i)
                            m_objectsAtNodes.values()[i]->onPostLoad();
                    }

                    INLINE const HashMap<const text::ParameterNode*, ObjectPtr>& createdObjets() const
                    {
                        return m_objectsAtNodes;
                    }

                    INLINE bool findObjectByID(uint32_t index, ObjectPtr& outObject) const
                    {
                        return m_objectsById.find(index, outObject);
                    }

                    INLINE bool findObjectAtNode(const text::ParameterNode* node, ObjectPtr& outObject) const
                    {
                        return m_objectsAtNodes.find(node, outObject);
                    }

                    INLINE const stream::LoadingContext& context() const
                    {
                        return m_context;
                    }

                private:
                    const stream::LoadingContext& m_context;
                    stream::LoadingResult& m_result;

                    // objects at given nodes
                    HashMap<const text::ParameterNode*, ObjectPtr> m_objectsAtNodes;

                    // object by ID
                    HashMap<uint32_t, ObjectPtr> m_objectsById;

                    // all loaded child resources so far
                    HashMap<ResourceKey, ResourceHandle> m_loadedResources;
                };

                ///--

                class LoadingStream : public stream::ITextReader
                {
                public:
                    LoadingStream(LoadingWorker& worker, const text::ParameterNode* objectNode)
                        : m_worker(worker)
                        , m_root(objectNode)
                        , m_hasError(false)
                    {
                        m_nodeStack.emplaceBack(objectNode, NodeType::Object);
                    }

                    virtual bool hasErrors() const override final
                    {
                        return m_hasError;
                    }

                    virtual bool beginArrayElement() override final
                    {
                        // the property elements can only be visited in objects
                        auto& topNode = this->topNode();
                        if (topNode.type != NodeType::Property && topNode.type != NodeType::ArrayElement)
                        {
                            reportError("array elements can only be placed inside a property or other array elements ");
                            return false;
                        }

                        // look for a valid child
                        if (topNode.childrenIterator)
                        {
                            // get next ID for the array element
                            auto childNode = topNode.childrenIterator.get();
                            ++topNode.childrenIterator;

                            // we have found a valid named property node
                            m_nodeStack.emplaceBack(childNode, NodeType::ArrayElement);
                            return true;
                        }

                        // no property found
                        return false;

                    }

                    virtual void endArrayElement() override final
                    {
                        ASSERT_EX(topNode().type == NodeType::ArrayElement, "Node stack corruption");
                        m_nodeStack.popBack();
                    }

                    virtual bool beginProperty(StringView<char>& outPropertyName) override final
                    {
                        // the property elements can only be visited in objects
                        auto& topNode = this->topNode();

                        // look for a valid child
                        while (topNode.childrenIterator)
                        {
                            // get next ID for the array element
                            auto childNode = topNode.childrenIterator.get();
                            ++topNode.childrenIterator;

                            // get the name of the property, should be the first token in the node
                            if (!childNode->token(0))
                                continue;

                            // get property name
                            auto propName = StringView<char>(childNode->token(0)->value);
                            if (propName == "class" || propName == "id" || propName == "ref")
                                continue;

                            // we have found a valid named property node
                            m_nodeStack.emplaceBack(childNode, NodeType::Property);
                            outPropertyName = std::move(propName);
                            return true;
                        }

                        // no property found
                        return false;
                    }

                    virtual void endProperty() override final
                    {
                        ASSERT_EX(topNode().type == NodeType::Property, "Node stack corruption");
                        m_nodeStack.popBack();
                    }

                    virtual bool readValue(StringView<char>& outValue) override
                    {
                        // the property elements can only be visited in objects
                        auto& topNode = this->topNode();
                        if (topNode.type != NodeType::Property && topNode.type != NodeType::ArrayElement)
                        {
                            reportError("value can only be placed inside a property or array element");
                            return false;
                        }

                        // get the token with the value
                        auto valueToken  = topNode.node->lastToken();
                        if (!valueToken)
                        {
                            reportError("element has no value assign to it");
                            return false;
                        }

                        // additional validation for properties
                        if (topNode.type == NodeType::Property && valueToken == topNode.node->token(0))
                        {
                            reportError("property has no value specified");
                            return false;
                        }

                        // return the inner value
                        outValue = valueToken->value;
                        return true;
                    }

                    virtual bool readValue(Buffer& outData) override
                    {
                        // TODO
                        return false;
                    }

                    virtual bool readValue(ObjectPtr& outValue) override
                    {
                        auto& topNode = this->topNode();
                        if (topNode.type != NodeType::Property && topNode.type != NodeType::ArrayElement)
                        {
                            reportError("object can only be placed inside a property or array element");
                            return false;
                        }

                        // we may be an inlined object defined right at this node, use it if that's the case
                        if (m_worker.findObjectAtNode(topNode.node, outValue))
                            return true;

                        // if we have the ref attribute we are a reference to existing object already created
                        auto objectRefId = topNode.node->keyValueRead<int>("ref", -1);
                        if (objectRefId >= 0)
                        {
                            if (m_worker.findObjectByID((uint32_t)objectRefId, outValue))
                                return true;

                            reportError(base::TempString("unresolved object ID {}", objectRefId));
                            return false;
                        }

                        // are we a null ?
                        auto lastToken  = topNode.node->lastToken();
                        if (lastToken && (0 == lastToken->value.caseCmp("null")))
                        {
                            outValue.reset();
                            return true;
                        }

                        // invalid object value
                        //reportError("invalid object value");
                        outValue.reset();
                        return true;
                    }

                    virtual bool readValue(stream::ResourceLoadingPolicy policy, StringBuf& outPath, ClassType& outClass, ObjectPtr& outObject) override
                    {
                        auto& topNode = this->topNode();
                        if (topNode.type != NodeType::Property && topNode.type != NodeType::ArrayElement)
                        {
                            reportError("object can only be placed inside a property or array element");
                            return false;
                        }

                        // get the basic information, one of this shit should be set
                        auto pathValue = topNode.node->keyValue("path");
                        auto absPathValue = topNode.node->keyValue("absPath");
                        auto classValue = topNode.node->keyValue("class");
                        if (!pathValue && !classValue && !absPathValue)
                        {
                            reportError("no path nor class specified for resource");
                            return false;
                        }

                        // inlined resource
                        if ((pathValue == nullptr && absPathValue == nullptr) && classValue != nullptr)
                            return readValue(outObject);

                        // empty path, null resource
                        if (!pathValue && !absPathValue)
                        {
                            outObject = nullptr;
                            outClass = nullptr;
                            outPath = "";
                            return true;
                        }

                        // determine the class of the resource to load
                        SpecificClassType<IResource> resourceClass;
                        if (!classValue.empty())
                        {
                            // find the resource class by the specified name
                            resourceClass = RTTI::GetInstance().findClass(StringID(classValue)).cast<IResource>();
                            if (!resourceClass)
                            {
                                reportError(base::TempString("unknown resource class '{}', resource '{}' will not be loaded", classValue, pathValue));
                                return false;
                            }
                        }
                        else
                        {
                            // use the resource extension to determine the class
                            auto pathExt = pathValue.afterFirst(".").beforeFirstOrFull(":");
                            resourceClass = IResource::FindResourceClassByExtension(pathExt);
                            if (!resourceClass)
                            {
                                reportError(base::TempString("no resource class for extension '{}', resource '{}' will not be loaded", pathExt, pathValue));
                                return false;
                            }
                        }

                        // should we load it ?
                        bool load = false;
                        if (auto loadedValue = topNode.node->keyValue("loaded"))
                            loadedValue.match(load);

                        // translate load path
                        if (pathValue)
                            m_worker.translatePath(false, pathValue, outPath);
                        else if (absPathValue)
                            m_worker.translatePath(true, absPathValue, outPath);

                        // resolve the object
                        if (load && !outPath.empty())
                            m_worker.loadResource(outPath, resourceClass, (ResourceHandle&)outObject);
                        return true;
                    }

                private:
                    LoadingWorker& m_worker;
                    const text::ParameterNode* m_root;

                    enum class NodeType
                    {
                        Object,
                        ArrayElement,
                        Property,
                    };

                    struct Node
                    {
                        const text::ParameterNode* node = nullptr;
                        NodeType type;
                        text::ParameterNodeIterator childrenIterator;

                        INLINE Node(const text::ParameterNode* node, NodeType type)
                            : node(node)
                            , type(type)
                            , childrenIterator(node->children())
                        {}
                    };

                    InplaceArray<Node, 32> m_nodeStack;

                    INLINE Node& topNode()
                    {
                        return m_nodeStack.back();
                    }

                    bool m_hasError;

                    void reportError(const char* txt)
                    {
                        if (!m_hasError)
                        {
                            // report only once
                            m_hasError = true;

                            // get the context
                            auto &topNode = m_nodeStack.back();
                            TRACE_ERROR("{}({}): error: {}", topNode.node->fileName(), topNode.node->line(), txt);
                        }
                    }

                };

            } // prv
            ///--

            TextLoader::TextLoader()
            {}

            TextLoader::~TextLoader()
            {}

            bool TextLoader::extractLoadingDependencies(stream::IBinaryReader& file, bool includeAsync, Array<stream::LoadingDependency>& outDependencies)
            {
                // TODO!
                return false;
            }

            bool TextLoader::loadObjects(stream::IBinaryReader& file, const stream::LoadingContext& context, stream::LoadingResult& result)
            {
                PC_SCOPE_LVL1(LoadObjectsText);

                // remember initial file offset
                auto initalOffset = file.pos();

                // load the header to determine if this is indeed XML
                char header[6];
                memzero(&header, sizeof(header));
                file.read(&header, sizeof(header));

                // detected XML, send to XML loader
                if (0 == strncmp(header, "<?xml ", ARRAY_COUNT(header)))
                {
                    file.seek(initalOffset);
                    xml::XMLLoader xmlLoader;
                    return xmlLoader.loadObjects(file, context, result);
                }

                // create the storage
                // TODO: Fix abstraction
                Buffer buffer = Buffer::Create(POOL_TEXT_LOADER, file.size() + 1);
                auto bufferPtr = (char*) buffer.data();

                // load data
                memcpy(bufferPtr + 0, header, sizeof(header));
                file.read(bufferPtr + sizeof(header), file.size() - sizeof(header));
                if (file.isError())
                {
                    TRACE_ERROR("Reading text content form file failed");
                    return false;
                }

                // zero terminate the buffer - mostly for debugging, the reader does not require this
                bufferPtr[file.size()] = 0;

                // load as parameter file
                mem::LinearAllocator mem(POOL_TEXT_LOADER);
                text::ParameterFile params(mem);
                if (!ParseParameters(StringView<char>(bufferPtr, bufferPtr + file.size()), params, context.m_contextName))
                {
                    TRACE_ERROR("Parsing text content form file failed");
                    return false;
                }

                // get the root node, should be one
                if (params.nodes()->tokenValueStr(0) != "document")
                {
                    TRACE_ERROR("Text file does not contain document");
                    return false;
                }

                // start loading, create all objects first
                prv::LoadingWorker loader(context, result);
                if (context.m_selectiveLoadingClass)
                    loader.createSelectiveObject(context.m_selectiveLoadingClass, params.nodes().get(), result.m_loadedRootObjects);
                else
                    loader.createObjects(params.nodes().get(), context.m_parent, context.m_parent, result.m_loadedRootObjects);

                // load object content
                bool contentValid = true;
                loader.createdObjets().forEach([&loader, &context, &contentValid](const text::ParameterNode* node, const ObjectPtr& obj)
                {
                    prv::LoadingStream streamReader(loader, node);
                    contentValid &= obj->onReadText(streamReader);
                    contentValid &= !streamReader.hasErrors();
                });

                // cast post load on all objects
                loader.postLoad();

                // return final state
                return contentValid;
            }

            ///--

        } // text
    } // res
} // base
