/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml\document #]
***/

#pragma once

#include "xmlDocument.h"
#include "base/containers/include/array.h"
#include "base/containers/include/hashMap.h"
#include "base/memory/include/linearAllocator.h"
#include "base/containers/include/staticStructurePool.h"

BEGIN_BOOMER_NAMESPACE(base::xml)

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
    virtual void nodeValue(NodeID id, StringView value) override final;
    virtual void nodeName(NodeID id, StringView name) override final;
    virtual void nodeAttribute(NodeID id, StringView name, StringView value) override final;
    virtual void deleteNodeAttribute(NodeID id, StringView name) override final;

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

        ID parentID = 0;
        ID firstChildID = 0;
        ID lastChildID = 0;
        ID nextSiblingID = 0;

        ID firstAttrID = 0;
        ID lastAttrID = 0;

        INLINE bool empty() const
        {
            return name.empty() && value.empty() && (parentID == 0) && (firstAttrID == 0) && (firstChildID == 0);
        }

        INLINE bool valid() const
        {
            return !name.empty();
        }
    };

    mem::LinearAllocator m_pool;
    HashMap<uint64_t, StringView> m_stringMap;

    StaticStructurePool<Node> m_nodes;
    StaticStructurePool<Attr> m_attributes;

    //--

    StringView mapString(StringView sourceString);
};

END_BOOMER_NAMESPACE(base::xml)