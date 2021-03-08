/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml\document #]
***/

#pragma once

#include "xmlDocument.h"
#include "core/containers/include/array.h"
#include "core/containers/include/hashMap.h"
#include "core/memory/include/linearAllocator.h"
#include "core/containers/include/staticStructurePool.h"

BEGIN_BOOMER_NAMESPACE_EX(xml)

/// a dynamic document that can be built at runtime
class DynamicDocument : public IDocument
{
public:
    DynamicDocument(StringView rootNodeName);

    //---

    virtual bool isReadOnly() const override final;

    //---

    virtual NodeID root() const override final;
    virtual NodeID nodeFirstChild(NodeID id, StringView childName = nullptr) const override final;
    virtual NodeID nodeSibling(NodeID id, StringView siblingName = nullptr) const override final;
    virtual NodeID nodeParent(NodeID id) const override final;
    virtual StringBuf nodeLocationInfo(NodeID id) const override final;
    virtual bool nodeLocationInfo(NodeID id, uint32_t& outLineNumber, uint32_t& outPosition) const override final;

    //---

    virtual StringView nodeValue(NodeID id) const override final;
    virtual Buffer nodeValueBuffer(NodeID id) const override final;
    virtual StringView nodeName(NodeID id) const override final;
    virtual StringView nodeAttributeOfDefault(NodeID id, StringView name, StringView defaultVal = StringView()) const override final;
    virtual AttributeID nodeFirstAttribute(NodeID id, StringView name = StringView()) const override final;

    //---

    virtual StringView attributeName(AttributeID id) const override final;
    virtual StringView attributeValue(AttributeID id) const override final;
    virtual AttributeID nextAttribute(AttributeID id, StringView name = StringView()) const override final;

    virtual void attributeName(AttributeID id, StringView name) override final;
    virtual void attributeValue(AttributeID id, StringView value) override final;
    virtual void deleteAttibute(AttributeID id) override final;

    //---

    virtual NodeID createNode(NodeID parentNodeID, StringView name) override final;
    virtual void deleteNode(NodeID id) override final;
    virtual void deleteNodeAttribute(NodeID id, StringView name) override final;

    virtual void writeNodeValue(NodeID id, StringView value) override final;
    virtual void writeNodeValue(NodeID id, Buffer value) override final;
    virtual void writeNodeName(NodeID id, StringView name) override final;
    virtual void writeNodeAttribute(NodeID id, StringView name, StringView value) override final;

private:
    typedef uint32_t ID;

    static const uint32_t INITIAL_NODE_RESERVE = 2048;

    struct Attr
    {
        StringView name;
        StringView value;

        ID parentNodeID = 0;
        ID nextAttrID = 0;

        INLINE bool empty() const
        {
            return name.empty() && value.empty() && (parentNodeID == 0) && (nextAttrID == 0);
        }

        INLINE bool valid() const
        {
            return !name.empty() && (parentNodeID != 0);
        }
    };

    struct Node
    {
        StringView name;
        StringView value;

        int bufferIndex = -1;

        ID parentID = 0;
        ID firstChildID = 0;
        ID lastChildID = 0;
        ID nextSiblingID = 0;

        ID firstAttrID = 0;
        ID lastAttrID = 0;

        INLINE bool empty() const
        {
            return name.empty() && value.empty() && (parentID == 0) && (firstAttrID == 0) && (firstChildID == 0) && (bufferIndex == -1);
        }

        INLINE bool valid() const
        {
            return !name.empty();
        }
    };

    LinearAllocator m_pool;
    HashMap<uint64_t, StringView> m_stringMap;

    StaticStructurePool<Node> m_nodes;
    StaticStructurePool<Attr> m_attributes;
    
    Array<Buffer> m_buffers;

    //--

    StringView mapString(StringView sourceString);
};

END_BOOMER_NAMESPACE_EX(xml)
