/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: xml #]
***/

#pragma once

#include "base/containers/include/stringBuf.h"

namespace base
{
    namespace xml
    {
        //--

        // helper - node "wrapper"
        class BASE_XML_API Node
        {
        public:
            INLINE Node() {}; // empty node
            INLINE Node(const Node& other) = default;
            INLINE Node& operator=(const Node& other) = default;
            Node(const IDocument* doc, NodeID node); // node must be valid
            Node(const IDocument* doc); // root node
            Node(IDocument* doc, NodeID node); // node must be valid
            Node(IDocument* doc); // root node

            // get the document
            INLINE const IDocument* document() const { return m_document; }

            // get the document
            INLINE IDocument* document() { return m_document; }

            // get the id of the node
            INLINE NodeID id() const { return m_id; }

            // is this valid node ?
            INLINE bool valid() const { return m_document != nullptr; }

            // operator bool check for easier "ifs"
            INLINE operator bool() const { return m_document != nullptr; }

            //--

            // get first node child
            Node firstChild(StringView<char> childName = nullptr) const;

            // get next node sibling
            Node sibling(StringView<char> siblingName = nullptr) const;

            // get parent node
            Node parent() const;

            //--

            // get node name
            // NOTE: returned string data is owned by the XML document, make a copy before destroying the document
            StringView<char> name() const;

            //--

            // get node value
            // NOTE: returned string data is owned by the XML document, make a copy before destroying the document
            StringView<char> value() const;

            // parse node value as int
            int valueInt(int defaultValue = 0) const;

            // parse node value as float
            float valueFloat(float defaultValue = 0.0) const;

            // parse boolean value of given attribute
            bool valueBool(bool defaultValue = false) const;

            // get node value as string, no default - an empty string will be returned; 
            StringBuf valueString() const;

            //--

            // get value for given attribute
            StringView<char> attribute(StringView<char> name, StringView<char> defaultVal = "") const;

            // parse integer value of given attribute
            int attributeInt(StringView<char> name, int defaultValue = 0) const;

            // parse float value of given attribute
            float attributeFloat(StringView<char> name, float defaultValue = 0.0) const;

            // parse boolean value of given attribute
            bool attributeBool(StringView<char> name, bool defaultValue = false) const;

            // parse string value of given attribute
            StringBuf attributeString(StringView<char> name, StringView<char> defaultValue = "") const;

            //--

            // write node value
            void writeValue(StringView<char> txt);

            // add node attribute
            void writeAttribute(StringView<char> name, StringView<char> value);

            // create child node
            Node writeChild(StringView<char> childName);

            //--

        private:
            IDocument* m_document = nullptr;
            NodeID m_id = 0;
        };

        ///---

        // helper - iterate over all nodes of given name in given parent node
        struct BASE_XML_API NodeIterator : public NoCopy
        {
        public:
            NodeIterator(const IDocument* doc, StringView<char> name);
            NodeIterator(Node node, StringView<char> name);
            ~NodeIterator();

            // get current node
            INLINE Node operator*() const { return m_current; }

            // get current node
            INLINE const Node* operator->() const { return &m_current; }

            // go to next element
            INLINE void operator++() { next(); }
            INLINE void operator++(int) { next(); }

            // is the entry valid ?
            INLINE operator bool() const { return m_current.valid(); }


        private:
            void next();

            Node m_current;
        };

        ///---

    } // xml
} // base