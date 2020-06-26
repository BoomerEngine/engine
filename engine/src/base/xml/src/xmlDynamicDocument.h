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
#include "base/containers/include/bitPool.h"
#include "base/containers/include/hashMap.h"
#include "base/memory/include/linearAllocator.h"

namespace base
{
    namespace xml
    {

        /// a dynamic document that can be built at runtime
        class DynamicDocument : public IDocument
        {
        public:
            DynamicDocument(StringView<char> rootNodeName);

            //---

            virtual bool isReadOnly() const override final;

            //---

            virtual NodeID root() const override final;
            virtual NodeID nodeFirstChild(NodeID id, StringView<char> childName = nullptr) const override final;
            virtual NodeID nodeSibling(NodeID id, StringView<char> siblingName = nullptr) const override final;
            virtual NodeID nodeParent(NodeID id) const override final;
            virtual StringBuf nodeLocationInfo(NodeID id) const override final;
            virtual bool nodeLocationInfo(NodeID id, uint32_t& outLineNumber, uint32_t& outPosition) const override final;

            //---

            virtual StringView<char> nodeValue(NodeID id) const override final;
            virtual StringView<char> nodeName(NodeID id) const override final;
            virtual StringView<char> nodeAttributeOfDefault(NodeID id, StringView<char> name, StringView<char> defaultVal = StringView<char>()) const override final;
            virtual AttributeID nodeFirstAttribute(NodeID id, StringView<char> name = StringView<char>()) const override final;

            //---

            virtual StringView<char> attributeName(AttributeID id) const override final;
            virtual StringView<char> attributeValue(AttributeID id) const override final;
            virtual AttributeID nextAttribute(AttributeID id, StringView<char> name = StringView<char>()) const override final;

            virtual void attributeName(AttributeID id, StringView<char> name) override final;
            virtual void attributeValue(AttributeID id, StringView<char> value) override final;
            virtual void deleteAttibute(AttributeID id) override final;

            //---

            virtual NodeID createNode(NodeID parentNodeID, StringView<char> name) override final;
            virtual void deleteNode(NodeID id) override final;
            virtual void nodeValue(NodeID id, StringView<char> value) override final;
            virtual void nodeName(NodeID id, StringView<char> name) override final;
            virtual void nodeAttribute(NodeID id, StringView<char> name, StringView<char> value) override final;
            virtual void deleteNodeAttribute(NodeID id, StringView<char> name) override final;

        private:
            typedef uint32_t ID;

            static const uint32_t INITIAL_NODE_RESERVE = 2048;

            struct Attr
            {
                StringView<char> name;
                StringView<char> value;

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
                StringView<char> name;
                StringView<char> value;

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

            typedef Array<Node> TNodes;
            typedef Array<Attr> TAtttributes;

            mem::LinearAllocator m_pool;
            HashMap<uint64_t, StringView<char>> m_stringMap;

            TNodes m_nodes;
            BitPool<> m_nodeIDs;

            TAtttributes m_attributes;
			BitPool<> m_attributesIDs;

            //--

            StringView<char> mapString(StringView<char> sourceString);
        };
        
    } // xml
} // base