/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml\document #]
***/

#include "build.h"
#include "xmlDynamicDocument.h"

#include "base/containers/include/stringBuilder.h"

#include <stdarg.h>

namespace base
{
    namespace xml
    {
        DynamicDocument::DynamicDocument(StringView<char> rootNodeName)
            : m_pool(POOL_XML)
            , m_nodes(POOL_XML)
            , m_attributes(POOL_XML)
        {
            m_nodes.resize(INITIAL_NODE_RESERVE);
            m_nodes.emplace(); // empty 

            m_attributes.resize(INITIAL_NODE_RESERVE);
            m_attributes.emplace(); // empty

            m_nodes.emplace(); // root
            m_nodes.typedData()[1].name = mapString(rootNodeName);
        }

        bool DynamicDocument::isReadOnly() const
        {
            return false;
        }

        //---

        NodeID DynamicDocument::root() const
        {
            return 1;
        }

        StringView<char> DynamicDocument::mapString(StringView<char> sourceString)
        {
            if (sourceString.empty())
                return StringView<char>();

            auto crc  = sourceString.calcCRC64();

            StringView<char> mappedString;
            if (m_stringMap.find(crc, mappedString))
            {
                ASSERT(mappedString == sourceString);
                return mappedString;
            }

            auto ptr  = m_pool.strcpy(sourceString.data(), sourceString.length());
            auto view  = StringView<char>(ptr, ptr + sourceString.length());
            m_stringMap[crc] = view;
            return view;
        }

        NodeID DynamicDocument::nodeFirstChild(NodeID id, StringView<char> childName /*= nullptr*/) const
        {
            ASSERT_EX(id < m_nodes.capacity(), "Node ID is out of range");

            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid node ID");

            auto childId  = node.firstChildID;
            while (childId)
            { 
                auto& childNode = m_nodes.typedData()[childId];

                if (childName.empty() || childNode.name == childName)
                    return childId;

                childId = childNode.nextSiblingID;
            }

            return 0; // invalid node
        }

        NodeID DynamicDocument::nodeSibling(NodeID id, StringView<char> siblingName /*= nullptr*/) const
        {
            ASSERT_EX(id < m_nodes.capacity(), "Node ID is out of range");

            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid node ID");

            auto siblingId  = node.nextSiblingID;
            while (siblingId)
            {
                auto& siblingNode = m_nodes.typedData()[siblingId];

                if (siblingName.empty() || siblingNode.name == siblingName)
                    return siblingId;

                siblingId  = siblingNode.nextSiblingID;
            }

            return 0; // invalid node
        }

        NodeID DynamicDocument::nodeParent(NodeID id) const
        {
            ASSERT_EX(id < m_nodes.capacity(), "Node ID is out of range");

            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid node ID");

            return node.parentID;
        }

        StringBuf DynamicDocument::nodeLocationInfo(NodeID id) const
        {
            return StringBuf("unknown");
        }

        bool DynamicDocument::nodeLocationInfo(NodeID id, uint32_t& outLineNumber, uint32_t& outPosition) const
        {
            return false;
        }

        //---

        StringView<char> DynamicDocument::nodeValue(NodeID id) const
        {
            ASSERT_EX(id < m_nodes.capacity(), "Node ID is out of range");

            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid node ID");

            return node.value;
        }

        StringView<char> DynamicDocument::nodeName(NodeID id) const
        {
            ASSERT_EX(id < m_nodes.capacity(), "Node ID is out of range");

            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid node ID");

            return node.name;
        }

        StringView<char> DynamicDocument::nodeAttributeOfDefault(NodeID id, StringView<char> name, StringView<char> defaultVal /*= StringBuf::EMPTY()*/) const
        {
            ASSERT_EX(id < m_nodes.capacity(), "Node ID is out of range");

            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid node ID");

            auto attributeId  = node.firstAttrID;
            while (attributeId != 0)
            {
                auto& attr = m_attributes.typedData()[attributeId];
                if (attr.name == name)
                    return attr.value;

                attributeId = attr.nextAttrID;
            }

            return defaultVal;
        }

        AttributeID DynamicDocument::nodeFirstAttribute(NodeID id, StringView<char> name /*= nullptr*/) const
        {
            ASSERT_EX(id < m_nodes.capacity(), "Node ID is out of range");

            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid node ID");

            auto attributeId  = node.firstAttrID;
            while (attributeId != 0)
            {
                auto& attr = m_attributes.typedData()[attributeId];
                if (name.empty() || attr.name == name)
                    return attributeId;

                attributeId = attr.nextAttrID;
            }

            return 0;
        }

        //---

        StringView<char> DynamicDocument::attributeName(AttributeID id) const
        {
            ASSERT_EX(id < m_attributes.capacity(), "Attribute ID is out of range");

            auto& attr = m_attributes.typedData()[id];
            ASSERT_EX(attr.valid(), "Invalid attribute ID");

            return attr.name;
        }

        StringView<char> DynamicDocument::attributeValue(AttributeID id) const
        {
            ASSERT_EX(id < m_attributes.capacity(), "Attribute ID is out of range");

            auto& attr = m_attributes.typedData()[id];
            ASSERT_EX(attr.valid(), "Invalid attribute ID");

            return attr.value;
        }

        AttributeID DynamicDocument::nextAttribute(AttributeID id, StringView<char> name /*= nullptr*/) const
        {
            ASSERT_EX(id < m_attributes.capacity(), "Attribute ID is out of range");

            auto& attr = m_attributes.typedData()[id];
            ASSERT_EX(attr.valid(), "Invalid attribute ID");

            auto nextAttributeID  = attr.nextAttrID;
            while (nextAttributeID)
            {
                auto& nextAttribute = m_attributes.typedData()[nextAttributeID];

                if (name.empty() || nextAttribute.name == name)
                    return nextAttributeID;

                nextAttributeID = nextAttribute.nextAttrID;
            }

            return 0;
        }

        void DynamicDocument::attributeName(AttributeID id, StringView<char> name)
        {
            ASSERT_EX(id < m_attributes.capacity(), "Attribute ID is out of range");

            auto& attr = m_attributes.typedData()[id];
            ASSERT_EX(attr.valid(), "Invalid attribute ID");
            ASSERT_EX(attr.parentNodeID != 0, "Invalid attribute");

            ASSERT_EX(!name.empty(), "Unable to nullify a name of the attribute");
            attr.name = mapString(name);
        }

        void DynamicDocument::attributeValue(AttributeID id, StringView<char> value)
        {
            ASSERT_EX(id < m_attributes.capacity(), "Attribute ID is out of range");

            auto& attr = m_attributes.typedData()[id];
            ASSERT_EX(attr.valid(), "Invalid attribute ID");
            ASSERT_EX(attr.parentNodeID != 0, "Invalid attribute");

            attr.value = mapString(value);
        }

        void DynamicDocument::deleteAttibute(AttributeID id)
        {
            ASSERT_EX(id < m_attributes.capacity(), "Attribute ID is out of range");

            auto& attr = m_attributes.typedData()[id];
            ASSERT_EX(attr.valid(), "Invalid attribute ID");
            ASSERT_EX(attr.parentNodeID != 0, "Invalid attribute");

            auto& node = m_nodes.typedData()[attr.parentNodeID];
            ASSERT_EX(node.valid(), "Invalid attribute node");

            bool found = false;

            // unlink the attribute from the table
            auto& prevID = node.firstAttrID;
            while (prevID != 0)
            {
                if (prevID == id)
                {
                    prevID = attr.nextAttrID;
                    m_attributes.typedData()[id] = Attr();
                    m_attributes.free(id);

                    found = true;
                    break;
                }

                prevID = m_attributes.typedData()[prevID].nextAttrID;
            }

            // recompute the last ID of attribute if it was removed
            if (node.lastAttrID == id)
            {
                node.lastAttrID = 0;

                auto attrID  = node.firstAttrID;
                while (attrID != 0)
                {
                    node.lastAttrID = attrID;
                    attrID = m_attributes.typedData()[attrID].nextAttrID;
                }
            }

            ASSERT_EX(found, "Attribute not found in it's parent node");
        }

        //---

        NodeID DynamicDocument::createNode(NodeID parentNodeID, StringView<char> name)
        {
            ASSERT_EX(parentNodeID < m_nodes.capacity(), "Node ID is out of range");

            if (m_nodes.full())
                m_nodes.resize(m_nodes.capacity() * 2);

            auto id = m_nodes.emplace();
            auto outNode = m_nodes.typedData() + id;

            ASSERT_EX(!name.empty(), "Cannot allocate nodes without name");
            ASSERT_EX(outNode->empty(), "Allocated node is already in use");
            outNode->name = mapString(name);
            outNode->parentID = (ID)parentNodeID;

            auto& parentNode = m_nodes.typedData()[parentNodeID];
            ASSERT_EX(parentNode.valid(), "Invalid parent node ID");

            if (parentNode.lastChildID != 0)
            {
                m_nodes.typedData()[parentNode.lastChildID].nextSiblingID = id;
                parentNode.lastChildID = id;
            }
            else
            {
                parentNode.lastChildID = id;
                parentNode.firstChildID = id;
            }

            return id;
        }

        void DynamicDocument::deleteNode(NodeID id)
        {
            // root node cannot be deleted
            if (id <= 1)
                return;

            // get node
            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid attribute node");

            // get parent and scan it
            auto& parentNode = m_nodes.typedData()[node.parentID];
            ASSERT_EX(parentNode.valid(), "Invalid parent node ID");

            bool found = false;

            // unlink the child node from the table
            auto prevID  = &parentNode.firstChildID;
            while (*prevID != 0)
            {
                auto curId = (ID)*prevID;
                if (curId == id)
                {
                    *prevID = m_nodes.typedData()[curId].nextSiblingID;

                    m_nodes.typedData()[curId] = Node();
                    m_nodes.free(curId);

                    found = true;
                    break;
                }

                prevID = &m_nodes.typedData()[curId].nextSiblingID;
            }

            // recompute the last ID of attribute if it was removed
            if (parentNode.lastChildID == id)
            {
                parentNode.lastChildID = 0;

                auto childID = parentNode.firstChildID;
                while (childID != 0)
                {
                    parentNode.lastChildID = childID;
                    childID = m_nodes.typedData()[childID].nextSiblingID;
                }
            }

            ASSERT_EX(found, "Child not found in it's parent node");
        }

        void DynamicDocument::nodeValue(NodeID id, StringView<char> value)
        {
            ASSERT_EX(id < m_nodes.capacity(), "Node ID is out of range");
            ASSERT_EX(id > 1, "Cannot set name of the root node");

            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid node ID");

            node.value = mapString(value);
        }

        void DynamicDocument::nodeName(NodeID id, StringView<char> name)
        {
            ASSERT_EX(id < m_nodes.capacity(), "Node ID is out of range");
            ASSERT_EX(id > 1, "Cannot set name of the root node");

            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid node ID");

            ASSERT_EX(!name.empty(), "Unable to set empty name for the node");
            node.name = mapString(name);
        }

        void DynamicDocument::nodeAttribute(NodeID nodeID, StringView<char> name, StringView<char> value)
        {
            ASSERT_EX(nodeID < m_nodes.capacity(), "Node ID is out of range");
            //ASSERT_EX(nodeID > 1, "Cannot set name of the root node");

            auto& node = m_nodes.typedData()[nodeID];
            ASSERT_EX(node.valid(), "Invalid node ID");

            ASSERT_EX(!name.empty(), "Unable to set add attribute with no name to node");

            // modify existing attribute
            bool found = false;
            auto attrId = node.firstAttrID;
            while (attrId)
            {
                auto& attr = m_attributes.typedData()[attrId];
                if (attr.name == name)
                {
                    attr.value = mapString(value);
                    found = true;
                    break;
                }

                attrId = attr.nextAttrID;
            }

            // create new attribute
            if (!found)
            {
                if (m_attributes.full())
                    m_attributes.resize(m_attributes.capacity() * 2);

                attrId = m_attributes.emplace();
                auto* outAttribute = m_attributes.typedData() + attrId;

                ASSERT_EX(outAttribute->empty(), "Allocated attribute is already in use");
                outAttribute->name = mapString(name);
                outAttribute->value = mapString(value);
                outAttribute->parentNodeID = (ID)nodeID;

                ASSERT_EX(!name.empty(), "Cannot allocate nodes without name");

                auto& parentNode = m_nodes.typedData()[nodeID];
                if (parentNode.lastAttrID != 0)
                {
                    m_attributes.typedData()[parentNode.lastAttrID].nextAttrID = attrId;
                    parentNode.lastAttrID = attrId;
                }
                else
                {
                    parentNode.firstAttrID = attrId;
                    parentNode.lastAttrID = attrId;
                }
            }
        }

        void DynamicDocument::deleteNodeAttribute(NodeID id, StringView<char> name)
        {
            ASSERT_EX(id < m_nodes.capacity(), "Node ID is out of range");

            auto& node = m_nodes.typedData()[id];
            ASSERT_EX(node.valid(), "Invalid node ID");

            // modify existing attribute
            auto attrId = node.firstAttrID;
            while (attrId)
            {
                auto& attr = m_attributes.typedData()[attrId];
                if (attr.name == name)
                {
                    deleteAttibute(attrId);
                    break;
                }

                attrId = attr.nextAttrID;
            }
        }

        //--

    } // xml
} // base

