/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\text #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"
#include "base/containers/include/hashMap.h"
#include "base/containers/include/inplaceArray.h"
#include "base/object/include/serializationSaver.h"
#include "base/object/include/streamTextWriter.h"

namespace base
{
    namespace res
    {
        namespace text
        {
            /// text based saver for the common format
            class BASE_RESOURCE_API TextSaver : public stream::ISaver, public stream::ITextWriter
            {
            public:
                TextSaver();

                // ISaver
                virtual bool saveObjects(stream::IBinaryWriter& file, const stream::SavingContext& context) override final;

            private:
                bool canWriteFullObject(uint32_t index);

                virtual void beginArrayElement() override final;
                virtual void endArrayElement() override final;

                virtual void beginProperty(const char* name) override final;
                virtual void endProperty() override final;

                virtual void writeValue(const Buffer& ptr) override final;
                virtual void writeValue(StringView<char> str) override final;
                virtual void writeValue(const IObject* object) override final;
                virtual void writeValue(StringView<char> resourcePath, ClassType resourceClass, const IObject* loadedObject) override final;

                mem::LinearAllocator m_mem;
                const stream::SavingContext* m_context;

                struct SaveToken
                {
                    SaveToken* nextToken = nullptr;
                    bool canSkip = false;
                    uint32_t length = 0;
                    char text[1] = {0};
                };

                struct SaveNode
                {
                    SaveToken* tokens = nullptr;
                    SaveToken* lastToken = nullptr;
                    uint32_t numTokens = 0;
                    uint32_t numRequiredTokens = 0;

                    SaveNode* children = nullptr;
                    SaveNode* nextChild = nullptr;
                    SaveNode* lastChild = nullptr;
                    uint32_t numChildren = 0;

                    bool saved = false;
                };

                SaveNode* m_root;
                InplaceArray<SaveNode*, 64> m_nodeStack;
                InplaceArray<const IObject*, 64> m_objectStack;

                typedef HashMap< const void*, uint32_t > TPointerMap;
                TPointerMap m_pointerMap;

                struct RuntimeExportInfo
                {
                    SaveNode* exportNode = nullptr;
                    Array<SaveNode*> m_refNodes;

                    const IObject* m_object;
                    uint32_t numReferences = 0;
                    bool written = false;
                    bool root = false;
                };

                InplaceArray< RuntimeExportInfo, 64> m_runtimeExportInfo;

                INLINE SaveNode* topNode() const
                {
                    return m_nodeStack.back();
                }

                INLINE const IObject* topObject() const
                {
                    return m_objectStack.back();
                }

                void writeObject(uint32_t index);

                // fill in node with object
                void writeObjectAtNode(SaveNode* node, uint32_t exportIndex);

                // construct a saving node
                // NOTE: memory is allocated from the internal allocataor
                // NOTE: node is automatically linked to parent
                SaveNode* createNode(SaveNode* parent);

                // construct a data token
                // NOTE: memory is allocated from the internal allocataor
                // NOTE: token is automatically linked to parent
                SaveToken* createNodeToken(SaveNode* parent, StringView<char> text, bool canSkip);

                // add an attribute to the node
                void createNodeAttribute(SaveNode* node, StringView<char> name, StringView<char> value);

                // convert the save node tree into a text
                void printNode(uint32_t indent, StringBuilder& builder, SaveNode* node);

                // print string
                void printString(StringBuilder& builder, const char* txt);
            };

        } // text
    } // res
} // base
