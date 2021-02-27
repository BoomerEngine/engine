/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\parser #]
***/

#pragma once

#include "dataValue.h"
#include "codeNode.h"
#include "program.h"
#include "programInstance.h"
#include "typeLibrary.h"
#include "typeUtils.h"
#include "function.h"
#include "fileParser.h"

#include "core/parser/include/textLanguageDefinition.h"
#include "core/parser/include/textSimpleLanguageDefinition.h"
#include "core/parser/include/textErrorReporter.h"
#include "core/parser/include/textToken.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//---

/// parser node
struct ParsingNode
{
    int tokenID = -1;
    ElementFlags flags = ElementFlags();
    parser::Location location;
    StringView stringData;
    Array<parser::Token*> tokens;
    double floatData = 0.0;
    int64_t intData = 0;
    Element* element = nullptr;
    TypeReference* typeRef = nullptr;
    Array<Element*> elements;

    INLINE explicit ParsingNode()
    {}

    INLINE explicit ParsingNode(const parser::Location& loc, StringView txt)
        : stringData(txt), location(loc)
    {}

    INLINE explicit ParsingNode(const parser::Location& loc, double val)
        : floatData(val), location(loc)
    {}

    INLINE explicit ParsingNode(const parser::Location& loc, int64_t val)
        : intData(val), location(loc)
    {}

    INLINE explicit ParsingNode(const parser::Location& loc, const ParsingNode& prevFlags, uint64_t flags)
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
class ParserTokenStream : public NoCopy
{
public:
    ParserTokenStream(parser::TokenList& tokens);

    /// read a token from the stream (used as yylex)
    int readToken(ParsingNode& outNode);

    /// extract inner tokens
    void extractInnerTokenStream(char delimiter, Array<parser::Token*>& outTokens);

    //--

    INLINE const parser::Location& location() const { return m_lastTokenLocation; }
    INLINE StringView text() const { return m_lastTokenText; }

private:
    parser::TokenList m_tokens;
                
    StringView m_lastTokenText;
    parser::Location m_lastTokenLocation;
};

//---

/// file structure parser helper
class ParsingFileContext : public NoCopy
{
public:
    ParsingFileContext(mem::LinearAllocator& mem, parser::IErrorReporter& errHandler, ParsingNode& result);
    ~ParsingFileContext();

    /// allocate some memory
    template< typename T, typename... Args >
    INLINE T* alloc(Args && ... args)
    {
        void* mem = m_mem.alloc(sizeof(T), __alignof(T));
        return new (mem) T(std::forward< Args >(args)...);
    }

    /// allocate string into persistent memory
    INLINE StringView strcpy(StringView txt)
    {
        auto copy  = m_mem.strcpy(txt.data(), txt.length());
        return StringView(copy, copy+txt.length());
    }

    /// get result node
    INLINE ParsingNode& result()
    {
        return m_result;
    }

    /// report error
    void reportError(const parser::Location& loc, StringView err);

private:
    mem::LinearAllocator& m_mem;
    parser::IErrorReporter& m_errHandler;
    ParsingNode& m_result;
};

//---

/// parser node for code parsing
struct CodeParsingNode
{
	parser::Location m_location;
	int m_tokenID = -1;
    StringView m_string;
	DataType m_type;
	CodeNode* m_code = nullptr;

	INLINE CodeParsingNode() {};

    CodeParsingNode& operator=(CodeNode* code);
	CodeParsingNode& operator=(std::nullptr_t);
};

//---

/// code token reader
class ParserCodeTokenStream : public NoCopy
{
public:
    ParserCodeTokenStream(Array<CodeParsingNode>& tokens);

    /// read a token from the stream (used as yylex)
    int readToken(CodeParsingNode& outNode);

    //--

    INLINE const parser::Location& location() const { return m_lastTokenLocation; }
    INLINE StringView text() const { return m_lastTokenText; }

private:
    Array<CodeParsingNode> m_tokens;
    int m_currentIndex;

    StringView m_lastTokenText;
    parser::Location m_lastTokenLocation;
};

//---

/// code structure parser helper
class ParsingCodeContext : public NoCopy
{
public:
    ParsingCodeContext(mem::LinearAllocator& mem, parser::IErrorReporter& errHandler, const CodeLibrary& lib, const Function* contextFunction, const Program* contextProgram);
    ~ParsingCodeContext();

    /// allocate some memory
    template< typename T, typename... Args >
    INLINE T* alloc(Args && ... args)
    {
        void* mem = m_mem.alloc(sizeof(T), __alignof(T));
        return new (mem) T(std::forward< Args >(args)...);
    }

    /// allocate string into persistent memory
    INLINE StringView strcpy(StringView txt)
    {
        auto copy  = m_mem.strcpy(txt.data(), txt.length());
        return StringView(copy, copy+txt.length());
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
    void reportError(const parser::Location& loc, StringView err);

    ///---

    /// create a function class node
    CodeNode* createFunctionCall(const parser::Location& loc, const StringView name, CodeNode* a = nullptr, CodeNode* b=nullptr, CodeNode* c=nullptr);

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
    mem::LinearAllocator& m_mem;
    parser::IErrorReporter& m_errHandler;
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

END_BOOMER_NAMESPACE_EX(gpu::compiler)
