/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\parser #]
***/

#include "build.h"
#include "renderingShaderNativeFunction.h"
#include "renderingShaderFileParser.h"
#include "renderingShaderFileParserHelper.h"
#include "renderingShaderFileStructureParser_Symbols.h"

#include "base/parser/include/textToken.h"

namespace rendering
{
    namespace compiler
    {
        namespace parser
        {

            //----


            ParsingNode& ParsingNode::link(const ParsingNode& child)
            {
                if (element && child.element != nullptr)
                    element->children.pushBack(child.element);

                return *this;
            }

            //----

            ParsingFileContext::ParsingFileContext(base::mem::LinearAllocator& mem, base::parser::IErrorReporter& errHandler, ParsingNode& result)
                : m_mem(mem)
                , m_errHandler(errHandler)
                , m_result(result)
            {}

            ParsingFileContext::~ParsingFileContext()
            {}

            void ParsingFileContext::reportError(const base::parser::Location& loc, base::StringView err)
            {
                m_errHandler.reportError(loc, err);
            }

            //----

            ParserTokenStream::ParserTokenStream(base::parser::TokenList& tokens)
                : m_tokens(std::move(tokens))
            {}

            int ParserTokenStream::readToken(ParsingNode& outNode)
            {
                // end of stream
                if (m_tokens.empty())
                    return 0;

                // get the token
                auto token  = m_tokens.popFront();

                // fill info
                if (token->isString())
                {
                    outNode.tokenID = TOKEN_STRING;
                    outNode.stringData = token->view();
                }
                else if (token->isName())
                {
                    outNode.tokenID = TOKEN_NAME;
                    outNode.stringData = token->view();
                }
                else if (token->isFloat())
                {
                    outNode.tokenID = TOKEN_FLOAT_NUMBER;
                    outNode.floatData = token->floatNumber();
                    outNode.stringData = token->view();
                }
                else if (token->isInteger())
                {
                    outNode.tokenID = TOKEN_INT_NUMBER;
                    outNode.intData = (int64_t)token->floatNumber();
                    outNode.stringData = token->view();
                }
                else if (token->isIdentifier())
                {
                    outNode.tokenID = TOKEN_IDENT;
                    // TODO: type name!
                    outNode.stringData = token->view();
                }
                else if (token->isChar())
                {
                    outNode.tokenID = token->ch();
                }
                else
                {
                    outNode.tokenID = token->keywordID();
                    outNode.stringData = token->view();
                }

                outNode.location = token->location();

                m_lastTokenText = token->view();
                m_lastTokenLocation = token->location();

                return outNode.tokenID;
            }

            void ParserTokenStream::extractInnerTokenStream(char delimiter, base::Array<base::parser::Token*>& outTokens)
            {
                uint32_t level = 0;

                while (m_tokens.head())
                {
                    if (m_tokens.head()->ch() == delimiter && !level)
                        if (level == 0)
                            break;

                    if (m_tokens.head()->ch() == '(' || m_tokens.head()->ch() == '{' || m_tokens.head()->ch() == '[')
                        level += 1;
                    else if (m_tokens.head()->ch() == ')' || m_tokens.head()->ch() == '}' || m_tokens.head()->ch() == ']')
                        level -= 1;

                    outTokens.pushBack(m_tokens.popFront());
                }
            }

            //----

            CodeParsingNode::CodeParsingNode()
                : m_tokenID(-1)
                , m_float(0.0)
                , m_int(0)
                , m_code(nullptr)
            {}

            CodeParsingNode::CodeParsingNode(const base::parser::Location& loc, base::StringView txt)
                : m_tokenID(-1)
                , m_float(0.0)
                , m_int(0)
                , m_string(txt)
                , m_code(nullptr)
                , m_location(loc)
            {}

            CodeParsingNode::CodeParsingNode(const base::parser::Location& loc, double val)
                : m_tokenID(-1)
                , m_float(val)
                , m_int((int64_t)val)
                , m_code(nullptr)
                , m_location(loc)
            {}

            CodeParsingNode::CodeParsingNode(const base::parser::Location& loc, int64_t val)
                : m_tokenID(-1)
                , m_float((double)val)
                , m_int(val)
                , m_code(nullptr)
                , m_location(loc)
            {}

            CodeParsingNode::CodeParsingNode(const base::parser::Location& loc, const DataType& knownType)
                : m_tokenID(-1)
                , m_float(0.0f)
                , m_int(0)
                , m_code(nullptr)
                , m_type(knownType)
                , m_location(loc)
            {}

            CodeParsingNode& CodeParsingNode::operator=(CodeNode* code)
            {
                if (code)
                {
                    m_code = code;
                    m_location = code->location();
                }

				return *this;
            }

            //----

            ParserCodeTokenStream::ParserCodeTokenStream(base::Array<CodeParsingNode>& tokens)
                : m_tokens(std::move(tokens))
                , m_currentIndex(0)
            {}

            static CodeParsingNode GEmptyToken;

            int ParserCodeTokenStream::readToken(CodeParsingNode& outNode)
            {
                if (m_currentIndex > m_tokens.lastValidIndex())
                    return 0;

                outNode = m_tokens[m_currentIndex++];

                m_lastTokenLocation = outNode.m_location;
                m_lastTokenText = outNode.m_string;

                return outNode.m_tokenID;
            }

            //----

            ParsingCodeContext::ParsingCodeContext(base::mem::LinearAllocator& mem, base::parser::IErrorReporter& errHandler, CodeNode*& result, const CodeLibrary& lib, const Function* contextFunction, const Program* contextProgram)
                : m_mem(mem)
                , m_errHandler(errHandler)
                , m_result(result)
                , m_lib(lib)
                , m_contextFunction(contextFunction)
                , m_contextProgram(contextProgram)
                , m_currentAttribute(0)
            {}

            ParsingCodeContext::~ParsingCodeContext()
            {}

            void ParsingCodeContext::reportError(const base::parser::Location& loc, base::StringView err)
            {
                m_errHandler.reportError(loc, err);
            }

            CodeNode* ParsingCodeContext::mergeExpressionList(CodeNode* a, CodeNode* b)
            {
                if (!a) return b;
                if (!b) return a;

                if (a->opCode() == OpCode::First)
                {
                    a->addChild(b);
                    return a;
                }
                /*else if (b->opCode() == OpCode::First)
                {
                    b->addChild(a);
                    return b;
                }*/

                auto ret = m_mem.create<CodeNode>(a->location(), OpCode::First);
                ret->addChild(a);
                ret->addChild(b);
                return ret;
            }

            static void AddChild(CodeNode* ret, CodeNode* b)
            {
                if (b->opCode() == OpCode::First)
                {
                    for (auto bchild  : b->children())
                        ret->moveChildren((CodeNode*)bchild);
                }
                else
                {
                    ret->moveChildren(b);
                }
            }

            CodeNode* ParsingCodeContext::mergeStatementList(CodeNode *a, CodeNode *b)
            {
                if (!a) return b;
                if (!b) return a;

                if (a->opCode() == OpCode::Scope)
                {
                    AddChild(a, b);
                    return a;
                }
                /*else if (b->opCode() == OpCode::Scope)
                {
                    b->moveChildren(a);
                    return b;
                }*/

                auto ret = m_mem.create<CodeNode>(a->location(), OpCode::Scope);
                AddChild(ret, a);
                AddChild(ret, b);
                return ret;
            }

            CodeNode* ParsingCodeContext::createFunctionCall(const base::parser::Location& loc, const base::StringView name, CodeNode* a, CodeNode* b, CodeNode* c)
            {
                if (name.empty())
                    return a;

                auto node = m_mem.create<CodeNode>(loc, OpCode::Call);

                {
                    auto ident = m_mem.create<CodeNode>(loc, OpCode::Ident);
                    ident->extraData().m_name = base::StringID(name);
                    node->addChild(ident);
                }

                if (a)
                {
                    node->moveChildren(a);
                    if (b)
                    {
                        node->moveChildren(b);
                        if (c)
                        {
                            node->moveChildren(c);
                        }
                    }
                }

                return node;
            }

            void ParsingCodeContext::attributeVal(uint32_t attribute)
            {
                m_currentAttribute = attribute;
            }

            uint32_t ParsingCodeContext::consumeAttribute()
            {
                auto ret = m_currentAttribute;
                m_currentAttribute = 0;
                return ret;
            }

            //----

        } // parser
    } // shader
} // rendering
