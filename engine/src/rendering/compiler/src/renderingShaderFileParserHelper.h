/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\parser #]
***/

#pragma once

#include "renderingShaderDataValue.h"
#include "renderingShaderCodeNode.h"
#include "renderingShaderProgram.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderTypeUtils.h"
#include "renderingShaderFunction.h"
#include "renderingShaderFileParser.h"

#include "base/parser/include/textLanguageDefinition.h"
#include "base/parser/include/textSimpleLanguageDefinition.h"
#include "base/parser/include/textErrorReporter.h"
#include "base/parser/include/textToken.h"

namespace rendering
{
    namespace compiler
    {
        namespace parser
        {
            //---

            /// parser node
            struct ParsingNode
            {
                int tokenID = -1;
                ElementFlags flags = ElementFlags();
                base::parser::Location location;
                base::StringView stringData;
                base::Array<base::parser::Token*> tokens;
                double floatData = 0.0;
                int64_t intData = 0;
                Element* element = nullptr;
                TypeReference* typeRef = nullptr;
                base::Array<Element*> elements;

                INLINE explicit ParsingNode()
                {}

                INLINE explicit ParsingNode(const base::parser::Location& loc, base::StringView txt)
                    : stringData(txt), location(loc)
                {}

                INLINE explicit ParsingNode(const base::parser::Location& loc, double val)
                    : floatData(val), location(loc)
                {}

                INLINE explicit ParsingNode(const base::parser::Location& loc, int64_t val)
                    : intData(val), location(loc)
                {}

                INLINE explicit ParsingNode(const base::parser::Location& loc, const ParsingNode& prevFlags, uint64_t flags)
                    : flags(prevFlags.flags | ElementFlag(flags)), location(loc)
                {}

                INLINE explicit ParsingNode(Element* element)
                {
                    if (element != nullptr )
                    {
                        this->element = element;
                        this->location = element->location;
                    }
                }

                INLINE explicit ParsingNode(TypeReference* typeRef)
                {
                    if (typeRef)
                    {
                        this->typeRef = typeRef;
                        this->location = typeRef->location;
                    }
                }

                ParsingNode& link(const ParsingNode& child);
            };

            //---

            /// token reader
            class ParserTokenStream : public base::NoCopy
            {
            public:
                ParserTokenStream(base::parser::TokenList& tokens);

                /// read a token from the stream (used as yylex)
                int readToken(ParsingNode& outNode);

                /// extract inner tokens
                void extractInnerTokenStream(char delimiter, base::Array<base::parser::Token*>& outTokens);

                //--

                INLINE const base::parser::Location& location() const { return m_lastTokenLocation; }
                INLINE base::StringView text() const { return m_lastTokenText; }

            private:
                base::parser::TokenList m_tokens;
                
                base::StringView m_lastTokenText;
                base::parser::Location m_lastTokenLocation;
            };

            //---

            /// file structure parser helper
            class ParsingFileContext : public base::NoCopy
            {
            public:
                ParsingFileContext(base::mem::LinearAllocator& mem, base::parser::IErrorReporter& errHandler, ParsingNode& result);
                ~ParsingFileContext();

                /// allocate some memory
                template< typename T, typename... Args >
                INLINE T* alloc(Args && ... args)
                {
                    void* mem = m_mem.alloc(sizeof(T), __alignof(T));
                    return new (mem) T(std::forward< Args >(args)...);
                }

                /// allocate string into persistent memory
                INLINE base::StringView strcpy(base::StringView txt)
                {
                    auto copy  = m_mem.strcpy(txt.data(), txt.length());
                    return base::StringView(copy, copy+txt.length());
                }

                /// get result node
                INLINE ParsingNode& result()
                {
                    return m_result;
                }

                /// report error
                void reportError(const base::parser::Location& loc, base::StringView err);

            private:
                base::mem::LinearAllocator& m_mem;
                base::parser::IErrorReporter& m_errHandler;
                ParsingNode& m_result;
            };

            //---

            /// parser node for code parsing
            struct CodeParsingNode
            {
				base::parser::Location m_location;
				int m_tokenID = -1;
                base::StringView m_string;
				DataType m_type;
				CodeNode* m_code = nullptr;

				INLINE CodeParsingNode() {};

                CodeParsingNode& operator=(CodeNode* code);
				CodeParsingNode& operator=(std::nullptr_t);
            };

            //---

            /// code token reader
            class ParserCodeTokenStream : public base::NoCopy
            {
            public:
                ParserCodeTokenStream(base::Array<CodeParsingNode>& tokens);

                /// read a token from the stream (used as yylex)
                int readToken(CodeParsingNode& outNode);

                //--

                INLINE const base::parser::Location& location() const { return m_lastTokenLocation; }
                INLINE base::StringView text() const { return m_lastTokenText; }

            private:
                base::Array<CodeParsingNode> m_tokens;
                int m_currentIndex;

                base::StringView m_lastTokenText;
                base::parser::Location m_lastTokenLocation;
            };

            //---

            /// code structure parser helper
            class ParsingCodeContext : public base::NoCopy
            {
            public:
                ParsingCodeContext(base::mem::LinearAllocator& mem, base::parser::IErrorReporter& errHandler, const CodeLibrary& lib, const Function* contextFunction, const Program* contextProgram);
                ~ParsingCodeContext();

                /// allocate some memory
                template< typename T, typename... Args >
                INLINE T* alloc(Args && ... args)
                {
                    void* mem = m_mem.alloc(sizeof(T), __alignof(T));
                    return new (mem) T(std::forward< Args >(args)...);
                }

                /// allocate string into persistent memory
                INLINE base::StringView strcpy(base::StringView txt)
                {
                    auto copy  = m_mem.strcpy(txt.data(), txt.length());
                    return base::StringView(copy, copy+txt.length());
                }

                /// get result node
                INLINE void root(CodeNode* node)
                {
                    m_result = node;
                }

				/// get the root node
				INLINE CodeNode* root() const
				{
					return m_result;
				}

                /// report error
                void reportError(const base::parser::Location& loc, base::StringView err);

                ///---

                /// create a function class node
                CodeNode* createFunctionCall(const base::parser::Location& loc, const base::StringView name, CodeNode* a = nullptr, CodeNode* b=nullptr, CodeNode* c=nullptr);

				///---

				/// create scope node from linked list of statements
				CodeNode* createScope(CodeNode* src, bool explicitScope = false);

				/// link statements
				CodeNode* linkStatements(CodeNode* first, CodeNode* second);

				/// create an ordered expression list
				CodeNode* createExpressionList(CodeNode* first, CodeNode* second);

				/// extract children from tree of OpCode::ListElement nodes
				void extractChildrenFromExpressionList(CodeNode* target, CodeNode* src);

				/// extract attributes
				CodeNode* extractAttributes(const CodeNode* attributeList, CodeNode* target);

				///--

            private:
                base::mem::LinearAllocator& m_mem;
                base::parser::IErrorReporter& m_errHandler;
				CodeNode* m_result = nullptr;

                uint32_t m_currentAttribute;

                const CodeLibrary& m_lib;
                const Function* m_contextFunction;
                const Program* m_contextProgram;

            public:
                DataType m_contextType;
                bool m_contextConstVar;
            };

            //---


        } // parser
    } // shader
} // rendering
