/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\xml\loader #]
***/

#include "build.h"
#include "resource.h"
#include "resourceXMLRuntimeLoaderObjectRegistry.h"
#include "resourceXMLRuntimeLoaderReferenceResolver.h"
#include "resourceXMLRuntimeLoaderTextReader.h"

#include "base/xml/include/xmlDocument.h"

namespace base
{
    namespace res
    {
        namespace xml
        {
            namespace prv
            {

                TextReader::TextReader(const base::xml::IDocument& doc, base::xml::NodeID objectNodeID, const LoaderObjectRegistry& objectRegistry, LoaderReferenceResolver& referenceResolver)
                    : m_objectRegistry(&objectRegistry)
                    , m_referenceResolver(&referenceResolver)
                    , m_doc(&doc)
                {
                    Node node(objectNodeID, NodeType::Object);
                    node.nextPropertyId = m_doc->nodeFirstChild(objectNodeID, "property");
                    m_nodeStack.pushBack(node);
                }

                TextReader::~TextReader()
                {
                    ASSERT_EX(m_nodeStack.size() == 1, "Node stack corruption");
                }

                //---
                
                void TextReader::pushElement(const base::xml::NodeID itemId, NodeType type)
                {
                    m_nodeStack.pushBack(Node(itemId, type));
                    m_nodeStack.back().nextPropertyId = m_doc->nodeFirstChild(itemId, "property");
                    m_nodeStack.back().nextId = m_doc->nodeFirstChild(itemId, "element");
                }

                bool TextReader::hasErrors() const
                {
                    return false;
                }

                bool TextReader::beginArrayElement()
                {
                    // the array elements can only be visited in arrays :)
                    auto& topNode = this->topNode();

                    // out of elements
                    if (topNode.nextId == 0)
                        return false;

                    // get next ID for the array element
                    auto itemId = topNode.nextId;
                    topNode.nextId = m_doc->nodeSibling(itemId, "element");

                    // push new context
                    pushElement(itemId, NodeType::ArrayElement);
                    return true;
                }

                void TextReader::endArrayElement()
                {
                    ASSERT_EX(topNode().type == NodeType::ArrayElement, "Node stack corruption");
                    m_nodeStack.popBack();
                }

                bool TextReader::beginProperty(StringView<char>& outPropertyName)
                {
                    // the property elements can only be visited in objects
                    auto& topNode = this->topNode();
                
                    // out of elements
                    if (topNode.nextPropertyId == 0)
                        return false;

                    // get next ID for the array element
                    auto itemId = topNode.nextPropertyId;
                    topNode.nextPropertyId = m_doc->nodeSibling(topNode.nextPropertyId, "property");

                    // get the name of the property
                    outPropertyName = m_doc->nodeAttributeOfDefault(itemId, "name");

                    // push new context
                    pushElement(itemId, NodeType::Property);
                    return true;
                }

                void TextReader::endProperty()
                {
                    ASSERT_EX(topNode().type == NodeType::Property, "Node stack corruption");
                    m_nodeStack.popBack();
                }

                bool TextReader::readValue(StringView<char>& outValue)
                {
                    // the property elements can only be visited in objects
                    auto& topNode = this->topNode();
                    if (topNode.type != NodeType::Property && topNode.type != NodeType::ArrayElement)
                    {
                        FATAL_ERROR("readValue called outside property");
                        return false;
                    }

                    // return the inner value
                    outValue = m_doc->nodeValue(topNode.id);
                    return true;
                }

                bool TextReader::readValue(ObjectPtr& outValue)
                {
                    // the property elements can only be visited in objects
                    auto& topNode = this->topNode();

                    // null reference
                    if (m_doc->nodeValue(topNode.id) == "null")
                    {
                        outValue = nullptr;
                        return true;
                    }

                    // we expect the "ref" or "object" node
                    auto refNodeId = m_doc->nodeFirstChild(topNode.id);
                    if (refNodeId == 0)
                    {
                        TRACE_ERROR("expected ref or object in value at {}", m_doc->nodeLocationInfo(topNode.id));
                        return false;
                    }

                    // resource reference ?
                    if (!m_doc->nodeAttributeOfDefault(refNodeId, "path").empty())
                    {
                        outValue = nullptr;
                        return false;
                    }

                    // resolve by node ID (direct objects)
                    if (m_objectRegistry->resolveByNodeID(refNodeId, outValue))
                        return true;

                    // get the ID of the referenced object
                    auto refId = m_doc->nodeAttributeOfDefault(refNodeId, "id");
                    if (refId.empty())
                    {
                        TRACE_ERROR("expected ID for the ref node '{}' at {}",
                            m_doc->nodeName(refNodeId), m_doc->nodeLocationInfo(refNodeId));
                        return false;
                    }

                    // resolve the id
                    if (m_objectRegistry->resolveByObjectID(refId, outValue))
                        return true;

                    // unresolved object
                    TRACE_WARNING("Unresolved object '{}' in '{}'",
                        refId, m_doc->nodeLocationInfo(refNodeId));

                    outValue = nullptr;
                    return true;
                }

                bool TextReader::readValue(Buffer& outData)
                {
                    return false;
                }

                bool TextReader::readValue(stream::ResourceLoadingPolicy policy, StringBuf& outPath, ClassType& outClass, ObjectPtr& outObject)
                {
                    // the property elements can only be visited in objects
                    auto& topNode = this->topNode();
                    if (topNode.type != NodeType::Property && topNode.type != NodeType::ArrayElement)
                    {
                        FATAL_ERROR("readValue called outside property");
                        return false;
                    }

                    // null reference
                    auto value = m_doc->nodeValue(topNode.id);
                    if (value == "null")
                    {
                        outPath = "";
                        outClass = nullptr;
                        outObject = nullptr;
                        return true;
                    }

                    // TODO!!
                    /*
                    // we expect the "ref" or "object" node
                    auto refNodeId = m_doc->nodeFirstChild(topNode.id, "ref");
                    if (refNodeId == 0)
                    {
                        TRACE_ERROR("expected ref in value at {}", m_doc->nodeLocationInfo(topNode.id));
                        return false;
                    }

                    // get the path of the reference
                    auto path = m_doc->nodeAttribute(refNodeId, "path");
                    if (path.empty())
                    {
                        TRACE_ERROR("no path in ref in value at {}", m_doc->nodeLocationInfo(topNode.id));
                        return false;
                    }

                    // resolve the path
                    if (outObjectPtr != nullptr)
                    {
                        // get the class of the resource
                        ClassType resourceClass = nullptr;
                        auto className = m_doc->nodeAttribute(refNodeId, "class");
                        if (!className.empty())
                        {
                            resourceClass = RTTI::GetInstance().findClass(StringID(className));
                            if (!resourceClass)
                            {
                                TRACE_ERROR("invalid resource class '{}' specified for resource reference {} at {}",
                                    className, path, m_doc->nodeLocationInfo(topNode.id));
                                return false;
                            }

                            outResourceClass = resourceClass;
                        }
                        else
                        {
                            auto extension = path.afterFirst(".");
                            resourceClass = IResource::FindResourceClassByExtension(extension);

                            if (!resourceClass)
                            {
                                TRACE_ERROR("no resource class specified for resource reference {} at {}", path, m_doc->nodeLocationInfo(topNode.id));
                                return false;
                            }

                            outResourceClass = resourceClass;
                        }

                        // resolve the object
                        *outObjectPtr = m_referenceResolver->resolveSyncReference(resourceClass, outPath);
                    }

                    // the path is always outputted
                    outPath = res::ResourcePath(StringBuf(path));*/
                    return true;
                }

            } // prv
        } // xml
    } // res
} // base

