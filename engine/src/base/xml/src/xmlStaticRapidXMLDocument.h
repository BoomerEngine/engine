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
            virtual NodeID nodeFirstChild(NodeID id, StringView childName = nullptr) const override final;
            virtual NodeID nodeSibling(NodeID id, StringView siblingName = nullptr) const override final;
            virtual NodeID nodeParent(NodeID id) const override final;
            virtual StringBuf nodeLocationInfo(NodeID id) const override final;
            virtual bool nodeLocationInfo(NodeID id, uint32_t& outLineNumber, uint32_t& outPosition) const override final;

            //---

            virtual StringView nodeValue(NodeID id) const override final;
            virtual StringView nodeName(NodeID id) const override final;
            virtual StringView nodeAttributeOfDefault(NodeID id, StringView name, StringView defaultVal = StringView()) const override final;
            virtual AttributeID nodeFirstAttribute(NodeID id, StringView name = nullptr) const override final;

            //---

            virtual StringView attributeName(AttributeID id) const override final;
            virtual StringView attributeValue(AttributeID id) const override final;
            virtual AttributeID nextAttribute(AttributeID id, StringView name = StringView()) const override final;

            virtual void attributeName(AttributeID id, StringView name) override final;
            virtual void attributeValue(AttributeID id, StringView value) override final;
            virtual void deleteAttibute(AttributeID id) override final;

            //---

            virtual NodeID createNode(NodeID parentNode, StringView name) override final;
            virtual void deleteNode(NodeID id) override final;
            virtual void nodeValue(NodeID id, StringView value) override final;
            virtual void nodeName(NodeID id, StringView name) override final;
            virtual void nodeAttribute(NodeID id, StringView name, StringView value) override final;
            virtual void deleteNodeAttribute(NodeID id, StringView name) override final;

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
