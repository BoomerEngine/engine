/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\xml #]
***/

#pragma once

#include "base/object/include/streamTextReader.h"
#include "base/containers/include/inplaceArray.h"

namespace base
{
    namespace res
    {
        namespace xml
        {
            namespace prv
            {

                class LoaderObjectRegistry;
                class LoaderReferenceResolver;

                // stream implementation for the loader
                class TextReader : public stream::ITextReader
                {
                public:
                    TextReader(const base::xml::IDocument& doc, base::xml::NodeID objectNode, const LoaderObjectRegistry& objectRegistry, LoaderReferenceResolver& referenceResolver);
                    virtual ~TextReader();

                private:
                    const LoaderObjectRegistry* m_objectRegistry;
                    LoaderReferenceResolver* m_referenceResolver;

                    const base::xml::IDocument* m_doc;

                    enum class NodeType
                    {
                        Object,
                        ArrayElement,
                        Property,
                    };

                    struct Node
                    {
                        base::xml::NodeID id;
                        NodeType type = NodeType::Object;

                        base::xml::NodeID nextId = 0;
                        base::xml::NodeID nextPropertyId = 0;

                        INLINE Node(base::xml::NodeID id, NodeType type)
                            : id(id), type(type)
                        {}
                    };

                    InplaceArray<Node, 32> m_nodeStack;

                    INLINE Node& topNode()
                    {
                        return m_nodeStack.back();
                    }

                    //---

                    virtual bool hasErrors() const override final;

                    virtual bool beginArrayElement() override final;
                    virtual void endArrayElement() override final;

                    virtual bool beginProperty(StringView<char>& outPropertyName) override final;
                    virtual void endProperty() override final;

                    virtual bool readValue(StringView<char>& outValue) override final;
                    virtual bool readValue(ObjectPtr& outValue) override final;
                    virtual bool readValue(Buffer& outData) override final;
                    virtual bool readValue(stream::ResourceLoadingPolicy policy, StringBuf& outPath, ClassType& outClass, ObjectPtr& outObject) override final;

                    void pushElement(const base::xml::NodeID nodeId, NodeType type);
                };

            } // prv
        } // xml
    } // res
} // base