/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\text #]
***/

#include "build.h"
#include "resource.h"
#include "resourceGeneralTextSaver.h"
#include "resourceGeneralTextStructureMapper.h"

#include "base/xml/include/xmlUtils.h"
#include "base/xml/include/xmlDocument.h"
#include "base/containers/include/stringBuilder.h"
#include "base/object/include/streamBinaryWriter.h"


namespace base
{
    namespace res
    {
        namespace text
        {

            //----

            TextSaver::TextSaver()
                : m_mem(POOL_RESOURCES)
                , m_context(nullptr)
            {
            }

            bool TextSaver::saveObjects(stream::IBinaryWriter& file, const stream::SavingContext& context)
            {
                // analyze structure of objects to save
                GeneralStructureMapper structureMapper;
                structureMapper.mapObjects(context);

                // reset memory allocator
                m_nodeStack.reset();
                m_objectStack.reset();
                m_pointerMap.reset();
                m_context = &context;
                m_mem.clear();

                // create the root node
                auto root  = createNode(nullptr);
                m_nodeStack.pushBack(root);

                // add some typical attributes
                createNodeToken(root, "document", false);
                createNodeAttribute(root, "tech", "Boomer Engine");
                createNodeAttribute(root, "version", "1");
                createNodeAttribute(root, "writer", TempString("{}@{}", base::GetUserName(), base::GetHostName()));

                // nothing to save, exit now
                if (context.m_initialExports.empty())
                    return true;

                // setup building stack
                m_saveEditorOnlyProperties = context.m_saveEditorOnlyProperties;
                m_objectStack.pushBack(nullptr);

                // setup flags
                auto numObjects = structureMapper.m_exports.size();
                m_runtimeExportInfo.reserve(numObjects);
                for (auto& info : structureMapper.m_exports)
                {
                    RuntimeExportInfo runtimeInfo;
                    runtimeInfo.root = info.root;
                    runtimeInfo.m_object = info.object;
                    runtimeInfo.numReferences = 1;
                    m_runtimeExportInfo.pushBack(runtimeInfo);
                }

                // setup maps
                m_pointerMap = std::move(structureMapper.m_objectIndices);

                // write each root objects, this will write all child objects (as long as they are referenced)
                uint32_t rootObjectIndex = 0;
                for (uint32_t exportIndex = 0; exportIndex < m_runtimeExportInfo.size(); ++exportIndex)
                {
                    auto& runtimeInfo = m_runtimeExportInfo[exportIndex];
                    if (runtimeInfo.root)
                    {
                        ASSERT_EX(!runtimeInfo.written, "Root objects must be written at the root");

                        auto objectNode  = createNode(root);
                        createNodeToken(objectNode, "object", true);
                        writeObjectAtNode(objectNode, exportIndex);

                        ASSERT_EX(m_nodeStack.size() == 1, "Node stack damaged");
                        ASSERT_EX(m_objectStack.size() == 1, "Object stack damaged");
                    }
                }

                // create child objects that were not reached through normal means
                for (;;)
                {
                    bool allWritten = true;
                    for (uint32_t exportIndex = 0; exportIndex < m_runtimeExportInfo.size(); ++exportIndex)
                    {
                        auto& runtimeInfo = m_runtimeExportInfo[exportIndex];

                        if (!runtimeInfo.written)
                        {
                            allWritten = false;

                            uint32_t parentIndex = 0;
                            m_pointerMap.find(runtimeInfo.m_object->parent(), parentIndex);
                            ASSERT_EX(parentIndex != 0, "Parent object NOT mapped in object that was not reachable");

                            // parent was written, write the object as well
                            auto& parentInfo = m_runtimeExportInfo[parentIndex - 1];
                            if (parentInfo.written)
                            {
                                auto objectNode  = createNode(parentInfo.exportNode);
                                createNodeToken(objectNode, "object", true);
                                writeObjectAtNode(objectNode, exportIndex);
                            }
                        }
                    }

                    // we are done when all nodes are written (it may take more than one pass)
                    if (allWritten)
                        break;
                }

                // every object that was referenced has to export it's access hash
                uint32_t objectIdAllocator = 1;
                for (uint32_t exportIndex = 0; exportIndex < m_runtimeExportInfo.size(); ++exportIndex)
                {
                    auto& runtimeInfo = m_runtimeExportInfo[exportIndex];
                    if (!runtimeInfo.m_refNodes.empty())
                    {
                        ASSERT_EX(runtimeInfo.exportNode != nullptr, "Object not written");

                        createNodeAttribute(runtimeInfo.exportNode, "id", TempString("{}", objectIdAllocator));

                        for (auto refNode : runtimeInfo.m_refNodes)
                            createNodeToken(refNode, TempString("{}", objectIdAllocator), false);

                        objectIdAllocator += 1;
                    }
                }

                // build the final structure and print to text
                StringBuilder printer;
                printNode(0, printer, root);

                // write to file
                file.write(printer.c_str(), printer.length());
                return true;
            }

            namespace helper
            {
                static bool ShouldEscapeString(const char* txt)
                {
                    if (*txt == 0)
                        return true; // escape empty strings

                    while (*txt)
                    {
                        auto ch = *txt++;
                        if (ch <= 32 || ch == '\"' || ch == '=')
                            return true;
                    }
                    return false;
                }

                static void WriteEscapedString(StringBuilder& builder, const char* txt)
                {
                    builder.append("\"");
                    while (*txt)
                    {
                        auto ch = *txt++;

                        if (ch == '\n')
                        {
                            builder.append("\\n");
                        }
                        else if (ch == '\r')
                        {
                            builder.append("\\r");
                        }
                        else if (ch == '\t')
                        {
                            builder.append("\\t");
                        }
                        else if (ch == '\b')
                        {
                            builder.append("\\b");
                        }
                        else if (ch == '\"')
                        {
                            builder.append("\\\"");
                        }
                        else if (ch < 32 || ch > 127)
                        {
                            builder.append("\\x%04u", ch);
                        }
                        else
                        {
                            const char str[] = {ch, 0};
                            builder.append(str);
                        }
                    }

                    builder.append("\"");
                }

            } // helper

            void TextSaver::printString(StringBuilder& builder, const char* txt)
            {
                if (helper::ShouldEscapeString(txt))
                    helper::WriteEscapedString(builder, txt);
                else
                    builder.append(txt);
            }

            void TextSaver::printNode(uint32_t indent, StringBuilder& builder, SaveNode* node)
            {
                // determine attributes to pull
                InplaceArray<SaveNode*, 16> attributeNodes;
                {
                    uint32_t maxAttributeValueNameLength = 100;
                    uint32_t maxAttributeNameLength = 9;
                    uint32_t maxTotalAttributesLength = 180;
                    uint32_t totalAttributeTextWritten = 0;
                    for (auto child = node->children; child != nullptr; child = child->nextChild)
                    {
                        if (child->numChildren == 0 && child->numTokens == 2) // is this an attribute
                        {
                            auto keyToken = child->tokens;
                            auto valueToken = child->tokens->nextToken;
                            if (keyToken->length < maxAttributeNameLength && valueToken->length < maxAttributeValueNameLength)
                            {
                                auto totalSize = keyToken->length + valueToken->length;
                                if (totalAttributeTextWritten + totalSize < maxTotalAttributesLength)
                                {
                                    attributeNodes.pushBack(child);
                                    child->saved = true;
                                    totalAttributeTextWritten += keyToken->length;
                                    totalAttributeTextWritten += valueToken->length;
                                }
                            }
                        }
                    }
                }

                // check if the node has any content that may cause skippable tokens not to be written
                auto canSkipOptionalTokens = (node->numRequiredTokens >= 1) || !attributeNodes.empty();

                // write indentation for the node
                builder.appendPadding(' ', indent*2);

                // write node tokens, the first token can be skipped
                bool requiresSeparation = false;
                for (auto token  = node->tokens; token != nullptr; token = token->nextToken)
                {
                    if (token->canSkip && canSkipOptionalTokens)
                        continue;

                    if (requiresSeparation)
                        builder.append(" ");
                    requiresSeparation = true;

                    printString(builder, token->text);
                }

                // write attributes
                for (auto child  : attributeNodes)
                {
                    auto keyToken = child->tokens;
                    auto valueToken = child->tokens->nextToken;

                    if (requiresSeparation)
                        builder.append(" ");
                    requiresSeparation = true;

                    builder.append(keyToken->text);
                    builder.append("=");
                    printString(builder, valueToken->text);
                }

                // end the line
                builder.append("\n");

                // write non-written child nodes
                for (auto child  = node->children; child != nullptr; child = child->nextChild)
                    if (!child->saved)
                        printNode(indent+1, builder, child);
            }

            TextSaver::SaveNode* TextSaver::createNode(SaveNode* parent)
            {
                auto node  = m_mem.create<SaveNode>();

                if (parent != nullptr)
                {
                    parent->numChildren += 1;
                    if (parent->lastChild)
                    {
                        parent->lastChild->nextChild = node;
                        parent->lastChild = node;
                        node->nextChild = nullptr;
                    }
                    else
                    {
                        parent->children = node;
                        parent->lastChild = node;
                        node->nextChild = nullptr;
                    }
                }


                return node;
            }

            static bool DoesStringRequireEscapement(const char* text)
            {
                // empty strings require escapement
                if (*text == 0)
                    return true;

                while (*text)
                {
                    auto ch = *text++;
                    if (ch == '+' || ch == '-' || ch == '.' || ch == '_'
                        || (ch >= '0' && ch <= '9')
                        || (ch >= 'a' && ch <= 'z')
                        || (ch >= 'A' && ch <= 'Z'))
                        continue;

                    return true;
                }

                return false;
            }

            TextSaver::SaveToken* TextSaver::createNodeToken(SaveNode* parent, StringView<char> text, bool canSkip)
            {
                auto textLength = text.length();
                auto mem  = m_mem.alloc(sizeof(SaveToken) + textLength + 1, 1);
                memset(mem, 0, sizeof(SaveToken) + textLength + 1);

                auto token = new (mem) SaveToken;
                memcpy(token->text, text.data(), textLength);
                token->canSkip = canSkip;
                token->length = textLength;

                parent->numTokens += 1;

                if (!canSkip)
                    parent->numRequiredTokens += 1;

                if (parent->lastToken)
                {
                    parent->lastToken->nextToken = token;
                    parent->lastToken = token;
                    token->nextToken = nullptr;
                }
                else
                {
                    parent->tokens = token;
                    parent->lastToken = token;
                    token->nextToken = nullptr;
                }

                return token;
            }

            void TextSaver::createNodeAttribute(SaveNode* node, StringView<char> name, StringView<char> value)
            {
                auto child  = createNode(node);
                createNodeToken(child, name, false);
                createNodeToken(child, value, false);
            }

            void TextSaver::writeObjectAtNode(SaveNode* node, uint32_t exportIndex)
            {
                // update
                auto& info = m_runtimeExportInfo[exportIndex];
                ASSERT_EX(!info.written, "Object already written");
                info.written = true;
                info.exportNode = node;

                // write the class attribute
                createNodeAttribute(info.exportNode, "class", info.m_object->cls()->name().c_str());

                // write inner structure
                m_nodeStack.pushBack(info.exportNode);
                m_objectStack.pushBack(info.m_object);
                info.m_object->onWriteText(*this);
                m_objectStack.popBack();
                m_nodeStack.popBack();
            }

            void TextSaver::writeObject(uint32_t objectIndex)
            {
                if (objectIndex == 0)
                {
                    createNodeToken(topNode(), "null", false);
                }
                else
                {
                    auto exportIndex = objectIndex - 1;
                    if (canWriteFullObject(exportIndex))
                    {
                        writeObjectAtNode(topNode(), exportIndex);
                    }
                    else
                    {
                        auto refNode  = createNode(topNode());
                        createNodeToken(refNode, "ref", true);

                        auto& info = m_runtimeExportInfo[exportIndex];
                        info.numReferences += 1;
                        info.m_refNodes.pushBack(refNode);
                    }
                }
            }

            bool TextSaver::canWriteFullObject(uint32_t exportIndex)
            {
                auto& info = m_runtimeExportInfo[exportIndex];

                if (info.written)
                    return false;

                const IObject* parentObject = topObject();
                if (!parentObject)
                {
                    if (info.root)
                        return true;
                }
                else
                {
                    if (info.m_object->parent() == parentObject)
                        return true;
                }

                return false;
            }

            //----

            void TextSaver::beginArrayElement()
            {
                auto elementNode  = createNode(topNode());
                createNodeToken(elementNode, "element", true);
                m_nodeStack.pushBack(elementNode);
            }

            void TextSaver::endArrayElement()
            {
                m_nodeStack.popBack();
            }

            void TextSaver::beginProperty(const char* name)
            {
                auto propNode  = createNode(topNode());
                createNodeToken(propNode, name, false);
                m_nodeStack.pushBack(propNode);
            }

            void TextSaver::endProperty()
            {
                m_nodeStack.popBack();
            }

            void TextSaver::writeValue(const Buffer& ptr)
            {
                // TODO: solve this somehow - additioanl files on disk ?
            }

            void TextSaver::writeValue(StringView<char> str)
            {
                createNodeToken(topNode(), str, false);
            }

            void TextSaver::writeValue(const IObject* object)
            {
                uint32_t index = 0;
                if (m_pointerMap.find(object, index))
                {
                    writeObject(index);
                }
                else
                {
                    FATAL_ERROR("Trying to save unmapped object");
                }
            }

            void TextSaver::writeValue(StringView<char> resourcePath, ClassType resourceClass, const IObject* loadedObject)
            {
                if (resourcePath)
                {
                    if (resourceClass)
                    {
                        beginProperty("class");
                        writeValue(resourceClass.name().view());
                        endProperty();
                    }

                    if (loadedObject)
                    {
                        beginProperty("loaded");
                        writeValue("true");
                        endProperty();
                    }

                    if (m_context->m_baseReferncePath.empty() || resourcePath.beginsWith(m_context->m_baseReferncePath))
                    {
                        beginProperty("path");
                        writeValue(resourcePath.afterFirst(m_context->m_baseReferncePath));
                        endProperty();
                    }
                    else
                    {
                        beginProperty("absPath");
                        writeValue(resourcePath);
                        endProperty();
                    }
                }
                else if (loadedObject)
                {   
                    writeValue(loadedObject); // will generate class=
                }
                else
                {
                    writeValue("null");
                }
            }

        } // text
    } // res
} // base
