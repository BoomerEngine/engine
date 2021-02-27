/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\parser #]
***/

#include "build.h"
#include "nativeFunction.h"
#include "fileParser.h"
#include "fileParserHelper.h"
#include "structureParser_Symbols.h"

#include "core/parser/include/textToken.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//----

ParsingNode& ParsingNode::link(const ParsingNode& child)
{
    if (element && child.element != nullptr)
        element->children.pushBack(child.element);

    return *this;
}

//----

ParsingFileContext::ParsingFileContext(mem::LinearAllocator& mem, parser::IErrorReporter& errHandler, ParsingNode& result)
    : m_mem(mem)
    , m_errHandler(errHandler)
    , m_result(result)
{}

ParsingFileContext::~ParsingFileContext()
{}

void ParsingFileContext::reportError(const parser::Location& loc, StringView err)
{
    m_errHandler.reportError(loc, err);
}

//----

ParserTokenStream::ParserTokenStream(parser::TokenList& tokens)
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

void ParserTokenStream::extractInnerTokenStream(char delimiter, Array<parser::Token*>& outTokens)
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

CodeParsingNode& CodeParsingNode::operator=(CodeNode* code)
{
	m_tokenID = -1;
	m_code = code;

	if (code)
        m_location = code->location();

	return *this;
}

CodeParsingNode& CodeParsingNode::operator=(std::nullptr_t)
{
	m_tokenID = -1;
	m_code = nullptr;
	m_string = StringView();
	return *this;
}
			
//----

ParserCodeTokenStream::ParserCodeTokenStream(Array<CodeParsingNode>& tokens)
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

ParsingCodeContext::ParsingCodeContext(mem::LinearAllocator& mem, parser::IErrorReporter& errHandler, const CodeLibrary& lib, const Function* contextFunction, const Program* contextProgram)
    : m_mem(mem)
    , m_errHandler(errHandler)
    , m_lib(lib)
    , m_contextFunction(contextFunction)
    , m_contextProgram(contextProgram)
    , m_currentAttribute(0)
{}

ParsingCodeContext::~ParsingCodeContext()
{}

void ParsingCodeContext::reportError(const parser::Location& loc, StringView err)
{
    m_errHandler.reportError(loc, err);
}

static void ExtractExpressionList(CodeNode* scopeNode, CodeNode* child)
{
	if (!child)
		return;

	if (child->opCode() == OpCode::ListElement)
	{
		ASSERT(child->children().size() == 2);
		ExtractExpressionList(scopeNode, (CodeNode*)child->children()[0]);
		ExtractExpressionList(scopeNode, (CodeNode*)child->children()[1]);
	}
	else
	{
		scopeNode->addChild(child);
	}
}

static void ExtractNodesIntoSingleScope(CodeNode* scopeNode, CodeNode* child)
{
	ExtractExpressionList(scopeNode, child);

	if (child->extraData().m_nextStatement)
	{
		ExtractNodesIntoSingleScope(scopeNode, child->extraData().m_nextStatement);
		child->extraData().m_nextStatement = nullptr;
	}
}

CodeNode* ParsingCodeContext::createScope(CodeNode* src, bool explicitScope)
{
	if (!src)
		return m_mem.create<CodeNode>(parser::Location(), OpCode::Nop);

	if (!explicitScope && !src->extraData().m_nextStatement)
		return src;

	auto scope = m_mem.create<CodeNode>(src->location(), OpCode::Scope);		
	ExtractNodesIntoSingleScope(scope, src);

	return scope;
}

static void CheckNoSelfLinks(CodeNode* node, HashSet<const CodeNode*>& visited)
{
	ASSERT_EX(!visited.contains(node), "Node already in chain");
	visited.insert(node);

	if (node->extraData().m_nextStatement)
		CheckNoSelfLinks(node->extraData().m_nextStatement, visited);
}

CodeNode* ParsingCodeContext::linkStatements(CodeNode* first, CodeNode* second)
{
	if (!second)
		return first;
	if (!first)
		return second;

	ASSERT(first != second);

	{
		HashSet<const CodeNode* > a;
		CheckNoSelfLinks(first, a);
	}

	{
		HashSet<const CodeNode* > b;
		CheckNoSelfLinks(second, b);
	}

	auto** prevLink = &first->extraData().m_nextStatement;
	while (*prevLink)
		prevLink = &((*prevLink)->extraData().m_nextStatement);

	ASSERT((*prevLink) == nullptr);
	(*prevLink) = second;

	{
		HashSet<const CodeNode* > b;
		CheckNoSelfLinks(first, b);
	}


	return first;
}

CodeNode* ParsingCodeContext::createExpressionList(CodeNode* first, CodeNode* second)
{
	if (!first)
		return second;
	if (!second)
		return first;

	auto scope = m_mem.create<CodeNode>(first->location(), OpCode::ListElement);
	scope->addChild(first);
	scope->addChild(second);
	return scope;
}

void ParsingCodeContext::extractChildrenFromExpressionList(CodeNode* target, CodeNode* src)
{
	ExtractExpressionList(target, src);
}

static void ExtractAttribute(CodeNode* target, const ExtraAttribute& attr)
{
	for (auto& targetAttr : target->extraData().m_attributesMap)
	{
		if (targetAttr.key == attr.key)
		{
			TRACE_INFO("Extracted attribute '{}' = '{}'", attr.key, attr.value);
			targetAttr.value = attr.value;
			return;
		}
	}

	TRACE_INFO("Extracted attribute '{}' = '{}'", attr.key, attr.value);
	target->extraData().m_attributesMap.emplaceBack(attr);
}

static void ExtractAttributeList(CodeNode* target, const CodeNode* node)
{
	if (!node)
		return;

	ASSERT(node->opCode() == OpCode::ListElement);

	for (const auto& attr : node->extraData().m_attributesMap)
		ExtractAttribute(target, attr);

	for (const auto* child : node->children())
		ExtractAttributeList(target, child);
}

CodeNode* ParsingCodeContext::extractAttributes(const CodeNode* attributeList, CodeNode* target)
{
	if (target->opCode() == OpCode::Scope)
		target = (CodeNode*)target->children()[0];

	ExtractAttributeList(target, attributeList);
	return target;
}

CodeNode* ParsingCodeContext::createFunctionCall(const parser::Location& loc, const StringView name, CodeNode* a, CodeNode* b, CodeNode* c)
{
    if (name.empty())
        return a;

    auto node = m_mem.create<CodeNode>(loc, OpCode::Call);

    {
        auto ident = m_mem.create<CodeNode>(loc, OpCode::Ident);
        ident->extraData().m_name = StringID(name);
        node->addChild(ident);
    }

    if (a)
    {
		ASSERT(a->opCode() != OpCode::ListElement);
		ASSERT(a->extraData().m_nextStatement == nullptr);
        node->addChild(a);

        if (b)
        {
			ASSERT(b->opCode() != OpCode::ListElement);
			ASSERT(b->extraData().m_nextStatement == nullptr);
            node->addChild(b);

            if (c)
            {
				ASSERT(c->opCode() != OpCode::ListElement);
				ASSERT(c->extraData().m_nextStatement == nullptr);
                node->addChild(c);
            }
        }
    }

    return node;
}

//----

END_BOOMER_NAMESPACE_EX(gpu::compiler)
