/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml #]
***/

#include "build.h"
#include "xmlUtils.h"
#include "xmlDocument.h"
#include "xmlWrappers.h"

namespace base
{
    namespace xml
    {

        //--

        Node::Node(const IDocument* doc)
        {
            if (doc)
            {
                m_document = doc;
                m_id = doc->root();
            }
        }

        Node::Node(const IDocument* doc, NodeID node)
        {
            if (node && doc)
            {
                m_document = doc;
                m_id = node;
            }
        }

        Node Node::firstChild(StringView<char> childName /*= nullptr*/) const
        {
            if (m_document)
                return Node(m_document, m_document->nodeFirstChild(m_id, childName));
            return Node();
        }

        Node Node::sibling(StringView<char> siblingName /*= nullptr*/) const
        {
            if (m_document)
                return Node(m_document, m_document->nodeSibling(m_id, siblingName));
            return Node();
        }

        Node Node::parent() const
        {
            if (m_document)
                return Node(m_document, m_document->nodeParent(m_id));
            return Node();
        }

        StringView<char> Node::name() const
        {
            if (m_document)
                return m_document->nodeName(m_id);
            return "";
        }

        StringView<char> Node::value() const
        {
            if (m_document)
                return m_document->nodeValue(m_id);
            return "";
        }

        int Node::valueInt(int defaultValue /*= 0*/) const
        {
            int ret = defaultValue;
            value().match(ret);
            return ret;
        }

        float Node::valueFloat(float defaultValue) const
        {
            float ret = defaultValue;
            value().match(ret);
            return ret;
        }

        bool Node::valueBool(bool defaultValue) const
        {
            bool ret = defaultValue;
            value().match(ret);
            return ret;
        }

        StringBuf Node::valueString() const
        {
            return StringBuf(value());
        }
        
        //--

        StringView<char> Node::attribute(StringView<char> name, StringView<char> defaultVal) const
        {
            if (m_document)
            {
                auto id = m_document->nodeFirstAttribute(m_id, name);
                if (id != IDocument::INVALID_ATTRIBUTE_ID)
                    return m_document->attributeValue(id);
            }


            return defaultVal;
        }

        int Node::attributeInt(StringView<char> name, int defaultValue) const
        {
            int ret = defaultValue;

            if (m_document)
            {
                auto id = m_document->nodeFirstAttribute(m_id, name);
                if (id != IDocument::INVALID_ATTRIBUTE_ID)
                    m_document->attributeValue(id).match(ret);;
            }

            return ret;
        }

        float Node::attributeFloat(StringView<char> name, float defaultValue) const
        {
            float ret = defaultValue;

            if (m_document)
            {
                auto id = m_document->nodeFirstAttribute(m_id, name);
                if (id != IDocument::INVALID_ATTRIBUTE_ID)
                    m_document->attributeValue(id).match(ret);;
            }

            return ret;
        }

        bool Node::attributeBool(StringView<char> name, bool defaultValue) const
        {
            bool ret = defaultValue;

            if (m_document)
            {
                auto id = m_document->nodeFirstAttribute(m_id, name);
                if (id != IDocument::INVALID_ATTRIBUTE_ID)
                    m_document->attributeValue(id).match(ret);;
            }

            return ret;
        }

        StringBuf Node::attributeString(StringView<char> name, StringView<char> defaultValue) const
        {
            if (m_document)
            {
                auto id = m_document->nodeFirstAttribute(m_id, name);
                if (id != IDocument::INVALID_ATTRIBUTE_ID)
                    return StringBuf(m_document->attributeValue(id));
            }

            return StringBuf(defaultValue);
        }

        //--

        NodeIterator::NodeIterator(const IDocument* doc, StringView<char> name)
        {
            if (doc)
                m_current = Node(doc, doc->nodeFirstChild(doc->root(), name));
        }

        NodeIterator::NodeIterator(Node node, StringView<char> name)
        {
            if (node)
                m_current = node.firstChild(name);
        }

        NodeIterator::~NodeIterator()
        {}

        void NodeIterator::next()
        {
            if (m_current)
                m_current = m_current.sibling();
        }

        //--

    } // xml
} // base
