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
        // document reader
        class BASE_XML_API IDocument : public IReferencable
        {
        public:
            IDocument();
            virtual ~IDocument();

            //---

            static const NodeID INVALID_NODE_ID = 0;
            static const AttributeID INVALID_ATTRIBUTE_ID = 0;

            //---
            
            // is this a read only document ?
            virtual bool isReadOnly() const = 0;

            //---

            // get the root node (always there)
            virtual NodeID root() const = 0;

            // get first node child
            virtual NodeID nodeFirstChild(NodeID id, StringView<char> childName = nullptr) const = 0;

            // get next node sibling
            virtual NodeID nodeSibling(NodeID id, StringView<char> siblingName = nullptr) const = 0;

            // get parent node
            virtual NodeID nodeParent(NodeID id) const = 0;

            // get human readable node location information for easier debugging of files
            // NOTE: for dynamically built files this is not known
            virtual StringBuf nodeLocationInfo(NodeID id) const = 0;

            // get line number and position for given node ID
            // NOTE: for dynamically built files this is not known
            virtual bool nodeLocationInfo(NodeID id, uint32_t& outLineNumber, uint32_t& outPosition) const = 0;

            //---

            // get node value
            virtual StringView<char> nodeValue(NodeID id) const = 0;

            // get node name
            virtual StringView<char> nodeName(NodeID id) const = 0;

            // get value for attribute
            virtual StringView<char> nodeAttributeOfDefault(NodeID id, StringView<char> name, StringView<char> defaultVal = StringView<char>()) const = 0;

            // get first attribute
            virtual AttributeID nodeFirstAttribute(NodeID id, StringView<char> name = nullptr) const = 0;

            //---

            // get name of the node attribute
            virtual StringView<char> attributeName(AttributeID id) const = 0;

            // get value of the node attribute
            virtual StringView<char> attributeValue(AttributeID id) const = 0;

            // get next attribute
            virtual AttributeID nextAttribute(AttributeID id, StringView<char> name = StringView<char>()) const = 0;

            // set new attribute name
            virtual void attributeName(AttributeID id, StringView<char> name) = 0;

            // set new attribute value
            virtual void attributeValue(AttributeID id, StringView<char> value) = 0;

            // delete node attribute
            virtual void deleteAttibute(AttributeID id) = 0;

            //---

            // create child node
            virtual NodeID createNode(NodeID parentNodeID, StringView<char> name) = 0;

            // delete node (and all it's children)
            virtual void deleteNode(NodeID id) = 0;

            // set node value
            virtual void nodeValue(NodeID id, StringView<char> value) = 0;

            // set node name
            virtual void nodeName(NodeID id, StringView<char> name) = 0;

            // set node attribute
            virtual void nodeAttribute(NodeID id, StringView<char> name, StringView<char> value) = 0;

            // delete node attribute by name
            virtual void deleteNodeAttribute(NodeID id, StringView<char> name) = 0;

            //--

            // convert to a writable document, the given node is copied as the new root
            DocumentPtr createWritableCopy(NodeID id);

            // convert to an optimized read only copy, it will take much less memory and will have faster access
            DocumentPtr createReadOnlyCopy(NodeID id);

            //--

            // save node as binary stream, can be directly saved to file or transfered
            Buffer saveAsBinary(NodeID id) const;

            // save node as text stream
            void saveAsText(IFormatStream& f, NodeID id) const;
        };

    } // xml
} // base