/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "core/memory/include/linearAllocator.h"
#include "scriptFunctionCode.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//---

class FunctionParser;

//---

/// parser node
struct FunctionParsingNode
{
    int tokenID = -1;
    TextTokenLocation location;
    StringView stringValue;
    StringID name;
    double floatValue = 0.0f;
    int64_t intValue = 0;
    uint64_t uintValue = 0U;
    Array<StringID> names;
    FunctionNode* node = nullptr;
    Array<FunctionNode*> nodes;
    FunctionTypeInfo type;
};

class FunctionParsingContext;

/// token reader
class FunctionParsingTokenStream : public NoCopy
{
public:
    FunctionParsingTokenStream(const Array<Token*>& tokens, FunctionParsingContext& ctx);

    int readToken(FunctionParsingNode& outNode);

    INLINE const TextTokenLocation& location() const { return m_lastTokenLocation; }
    INLINE StringView text() const { return m_lastTokenText; }

private:
    TokenList m_tokens;
    FunctionParsingContext& m_ctx;

    StringView m_lastTokenText;
    TextTokenLocation m_lastTokenLocation;

    uint32_t matchTypeName(StringID& outTypeName, FunctionTypeInfo& outTypeInfo) const;
};

/// helper class for parsing function
class FunctionParsingContext : public NoCopy
{
public:
    FunctionParsingContext(LinearAllocator& mem, FunctionParser& fileParser, const StubFunction* function, FunctionCode& outCode);

    //--

    StubLocation mapLocation(const TextTokenLocation& location);

    FunctionNode* createNode(const TextTokenLocation& location, FunctionNodeOp op);
    FunctionNode* createNode(const TextTokenLocation& location, FunctionNodeOp op, FunctionNode* a);
    FunctionNode* createNode(const TextTokenLocation& location, FunctionNodeOp op, FunctionNode* a, FunctionNode* b);
    FunctionNode* createNode(const TextTokenLocation& location, FunctionNodeOp op, FunctionNode* a, FunctionNode* b, FunctionNode* c);

    FunctionNode* makeBreakpoint(FunctionNode* node);

    FunctionTypeInfo createStaticArrayType(FunctionTypeInfo innerType, uint32_t arraySize);
    FunctionTypeInfo createDynamicArrayType(FunctionTypeInfo innerType);
    FunctionTypeInfo createClassType(const TextTokenLocation& location, StringID className);
    FunctionTypeInfo createPtrType(const TextTokenLocation& location, StringID className);
    FunctionTypeInfo createWeakPtrType(const TextTokenLocation& location, StringID className);

    FunctionTypeInfo resolveTypeFromName(StringID name);

    FunctionNode* createIntConst(const TextTokenLocation& location, int64_t val);
    FunctionNode* createUintConst(const TextTokenLocation& location, uint64_t val);
    FunctionNode* createFloatConst(const TextTokenLocation& location, double val);
    FunctionNode* createBoolConst(const TextTokenLocation& location, bool val);
    FunctionNode* createStringConst(const TextTokenLocation& location, StringView val);
    FunctionNode* createNameConst(const TextTokenLocation& location, StringID val);
    FunctionNode* createNullConst(const TextTokenLocation& location);
    FunctionNode* createClassTypeConst(const TextTokenLocation& location, StringID className);

    void rootStatement(FunctionNode* node);

    void pushContextNode(FunctionNode* node);
    void popContextNode();

    FunctionNode* findContextNode(FunctionNodeOp op);
    FunctionNode* findBreakContextNode();
    FunctionNode* findContinueContextNode();

    void reportError(const TextTokenLocation& location, StringView message);

    FunctionTypeInfo currentType;

private:
    FunctionParser& m_parser;
    const StubFunction* m_function;
    FunctionCode& m_code;
    LinearAllocator& m_mem;

    Array<FunctionNode*> m_contextStack;
};

//---

END_BOOMER_NAMESPACE_EX(script)
