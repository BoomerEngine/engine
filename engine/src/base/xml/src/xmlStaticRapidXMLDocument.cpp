/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml\document #]
***/

#include "build.h"
#include "xmlStaticRapidXMLDocument.h"
#include "xmlUtils.h"

namespace base
{
    namespace xml
    {

        //---

        //---

        StaticRapidXMLDocument::StaticRapidXMLDocument()
            : m_root(nullptr)
        {
        }

        StaticRapidXMLDocument::~StaticRapidXMLDocument()
        {
        }

        namespace helper
        {
            static bool IsZeroTerminated(const Buffer& ptr)
            {
                if (!ptr)
                    return false;

                return 0 == ptr.data()[ptr.size() - 1];
            }
        }

        namespace helper
        {
            class RapidXMLErrorForwarder : public rapidxml::IErrorReporter
            {
            public:
                RapidXMLErrorForwarder(ILoadingReporter& ctx)
                    : m_ctx(ctx)
                {}

                virtual void onError(int line, int pos, const char* txt) override final
                {
                    m_hasErrorsReported = true;
                    m_ctx.onError(line, pos, txt);
                }

                INLINE bool hasErrorsReported() const
                {
                    return m_hasErrorsReported;
                }

            private:
                ILoadingReporter& m_ctx;
                bool m_hasErrorsReported;
            };
        }

        
        RefPtr<StaticRapidXMLDocument> StaticRapidXMLDocument::Load(ILoadingReporter& ctx, const Buffer& ptr, bool canStealBuffer /*= false*/)
        {
            auto ret  = RefNew<StaticRapidXMLDocument>();

            // buffer is to small
            if (ptr.size() < 10)
                return nullptr;

            // create copy of the buffer
            ret->m_data = Buffer::Create(POOL_MEM_BUFFER, ptr.size() + 2);
            if (!ret->m_data)
                return nullptr;

            // the buffer for the parser must be zero terminated
            auto bufferBase  = (char*)ret->m_data.data();
            memcpy(bufferBase, ptr.data(), ptr.size());
            bufferBase[ptr.size() + 0] = 0;
            bufferBase[ptr.size() + 1] = 0;

            // parse the document
            helper::RapidXMLErrorForwarder errorForwarder(ctx);
            if (!ret->m_doc.parse<0>(bufferBase, errorForwarder))
            {
                // make sure at least one error is reported if we've failed parsing
                if (!errorForwarder.hasErrorsReported())
                    ctx.onError(0, 0, "Unspecified error parsing XML data");
                return nullptr;
            }

            // get the root node
            ret->m_root = ret->m_doc.first_node();
            if (!ret->m_root)
            {
                TRACE_ERROR("XML has no root node and cannot be loaded");
                return nullptr;
            }

            return ret;
        }

        //---

        bool StaticRapidXMLDocument::isReadOnly() const
        {
            return true;
        }

        //---

        NodeID StaticRapidXMLDocument::root() const
        {
            return toNodeID(m_root);
        }

        NodeID StaticRapidXMLDocument::nodeFirstChild(NodeID id, StringView childName /*= nullptr*/) const
        {
            auto node  = fromNodeId(id);
            if (!node)
                return INVALID_NODE_ID;

            auto childNode  = childName.empty() ? node->first_node() : node->first_node(childName.data(), childName.length());
            while (childNode)
            {
                if (childNode->type() == rapidxml::node_element)
                    break;
                childNode = childName.empty() ? childNode->next_sibling() : childNode->next_sibling(childName.data(), childName.length());
            }

            return toNodeID(childNode);
        }

        NodeID StaticRapidXMLDocument::nodeSibling(NodeID id, StringView siblingName /*= nullptr*/) const
        {
            auto node  = fromNodeId(id);
            if (!node)
                return INVALID_NODE_ID;

            auto childNode = siblingName.empty() ? node->next_sibling() : node->next_sibling(siblingName.data(), siblingName.length());
            while (childNode)
            {
                if (childNode->type() == rapidxml::node_element)
                    break;
                childNode = siblingName.empty() ? childNode->next_sibling() : childNode->next_sibling(siblingName.data(), siblingName.length());
            }

            return toNodeID(childNode);
        }

        NodeID StaticRapidXMLDocument::nodeParent(NodeID id) const
        {
            auto node  = fromNodeId(id);
            if (!node)
                return INVALID_NODE_ID;

            return toNodeID(node->parent());
        }

        StringBuf StaticRapidXMLDocument::nodeLocationInfo(NodeID id) const
        {
            uint32_t lineNumber = 0, position = 0;
            if (!nodeLocationInfo(id, lineNumber, position))
                return StringBuf("unknown");

            return TempString("line {}, pos {}", lineNumber, position);
        }

        bool StaticRapidXMLDocument::nodeLocationInfo(NodeID id, uint32_t& outLineNumber, uint32_t& outPosition) const
        {
            if (!id)
                return false;

            auto node  = fromNodeId(id);
            if (!node)
                return false;

            outLineNumber = node->location().line();
            outPosition = node->location().pos();
            return true;
        }

        //---

        StringView StaticRapidXMLDocument::nodeValue(NodeID id) const
        {
            auto node  = fromNodeId(id);
            if (!node)
                return StringBuf::EMPTY();

            return StringView((const char*)node->value(), (uint32_t)node->value_size());
        }

        StringView StaticRapidXMLDocument::nodeName(NodeID id) const
        {
            auto node  = fromNodeId(id);
            if (!node)
                return StringBuf::EMPTY();

            return StringView(node->name(), (uint32_t)node->name_size());
        }

        StringView StaticRapidXMLDocument::nodeAttributeOfDefault(NodeID id, StringView name, StringView defaultVal /*= StringBuf::EMPTY()*/) const
        {
            auto node  = fromNodeId(id);
            if (!node)
                return defaultVal;

            auto attr  = name.empty() ? node->first_attribute() : node->first_attribute(name.data(), name.length());
            if (!attr)
                return defaultVal;

            return StringView(attr->value(), (uint32_t)attr->value_size());
        }

        AttributeID StaticRapidXMLDocument::nodeFirstAttribute(NodeID id, StringView name /*= nullptr*/) const
        {
            auto node  = fromNodeId(id);
            if (!node)
                return 0;

            return toAttributeID(name.empty() ? node->first_attribute() : node->first_attribute(name.data(), name.length()));
        }

        //---

        StringView StaticRapidXMLDocument::attributeName(AttributeID id) const
        {
            auto attr  = fromAttributeID(id);
            if (!attr)
                return StringView();

            return StringView(attr->name(), (uint32_t)attr->name_size());
        }

        StringView StaticRapidXMLDocument::attributeValue(AttributeID id) const
        {
            auto attr  = fromAttributeID(id);
            if (!attr)
                return StringView();

            return StringView(attr->value(), (uint32_t)attr->value_size());
        }

        AttributeID StaticRapidXMLDocument::nextAttribute(AttributeID id, StringView name /*= nullptr*/) const
        {
            auto attr  = fromAttributeID(id);
            if (!attr)
                return 0;

            return toAttributeID(name.empty() ? attr->next_attribute() : attr->next_attribute(name.data(), name.length()));
        }

        void StaticRapidXMLDocument::attributeName(AttributeID id, StringView name)
        {
            readonlyError();
        }

        void StaticRapidXMLDocument::attributeValue(AttributeID id, StringView value)
        {
            readonlyError();
        }

        void StaticRapidXMLDocument::deleteAttibute(AttributeID id)
        {
            readonlyError();
        }

        //---

        NodeID StaticRapidXMLDocument::createNode(NodeID parentNode, StringView name)
        {
            readonlyError();
            return 0;
        }

        void StaticRapidXMLDocument::deleteNode(NodeID id)
        {
            readonlyError();
        }

        void StaticRapidXMLDocument::nodeValue(NodeID id, StringView value)
        {
            readonlyError();
        }

        void StaticRapidXMLDocument::nodeName(NodeID id, StringView value)
        {
            readonlyError();
        }

        void StaticRapidXMLDocument::nodeAttribute(NodeID id, StringView name, StringView value)
        {
            readonlyError();
        }

        void StaticRapidXMLDocument::deleteNodeAttribute(NodeID id, StringView name)
        {
            readonlyError();
        }

         //---

         NodeID StaticRapidXMLDocument::toNodeID(const RapidNode* node) const
         {
             return (NodeID)node;
         }

         const RapidNode* StaticRapidXMLDocument::fromNodeId(NodeID id) const
         {
             return (RapidNode*)id;
         }

         //---

         AttributeID StaticRapidXMLDocument::toAttributeID(const RapidAttr* attr) const
         {
             return (AttributeID)attr;
         }

         const RapidAttr* StaticRapidXMLDocument::fromAttributeID(AttributeID id) const
         {
             return (RapidAttr*)id;
         }

         void StaticRapidXMLDocument::readonlyError()
         {
             FATAL_ERROR("Unable to modify read only XML document");
         }

         //---

    } // xml
} // base
