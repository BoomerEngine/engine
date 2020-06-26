/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml\document #]
***/

#pragma once

#include "rapidxml/rapidxml.hpp"
#include "xmlDocument.h"

namespace base
{
    namespace xml
    {
        class ILoadingReporter;

        typedef rapidxml::xml_document<char> RapidDoc;
        typedef rapidxml::xml_node<char> RapidNode;
        typedef rapidxml::xml_attribute<char> RapidAttr;

        /// static, rapid XML based document
        class StaticRapidXMLDocument : public IDocument
        {
        public:
            StaticRapidXMLDocument();
            virtual ~StaticRapidXMLDocument();

            /// create from text
            static RefPtr<StaticRapidXMLDocument> Load(ILoadingReporter& ctx, const Buffer& data, bool canStealBuffer = false);

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
            virtual AttributeID nodeFirstAttribute(NodeID id, StringView<char> name = nullptr) const override final;

            //---

            virtual StringView<char> attributeName(AttributeID id) const override final;
            virtual StringView<char> attributeValue(AttributeID id) const override final;
            virtual AttributeID nextAttribute(AttributeID id, StringView<char> name = StringView<char>()) const override final;

            virtual void attributeName(AttributeID id, StringView<char> name) override final;
            virtual void attributeValue(AttributeID id, StringView<char> value) override final;
            virtual void deleteAttibute(AttributeID id) override final;

            //---

            virtual NodeID createNode(NodeID parentNode, StringView<char> name) override final;
            virtual void deleteNode(NodeID id) override final;
            virtual void nodeValue(NodeID id, StringView<char> value) override final;
            virtual void nodeName(NodeID id, StringView<char> name) override final;
            virtual void nodeAttribute(NodeID id, StringView<char> name, StringView<char> value) override final;
            virtual void deleteNodeAttribute(NodeID id, StringView<char> name) override final;

        private:
            Buffer m_data;
            RapidDoc m_doc;

            RapidNode* m_root;

            Array<uint32_t> m_lineStarts; // starting positions of each line, sorted

            NodeID toNodeID(const RapidNode* node) const;
            const RapidNode* fromNodeId(NodeID id) const;

            AttributeID toAttributeID(const RapidAttr* attr) const;
            const RapidAttr* fromAttributeID(AttributeID id) const;

            void readonlyError();
        };

    } // xml
} // base
