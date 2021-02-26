/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptLibrary.h"
#include "scriptFunctionCodeParser.h"
#include "scriptFunctionCodeParserHelper.h"
#include "scriptFunctionCodeParser_Symbols.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//---

FunctionParsingTokenStream::FunctionParsingTokenStream(const Array<parser::Token*>& tokens, FunctionParsingContext& ctx)
    : m_ctx(ctx)
{
    for (auto tok  : tokens)
        m_tokens.pushBack(tok);
}

uint32_t FunctionParsingTokenStream::matchTypeName(StringID& outTypeName, FunctionTypeInfo& outTypeInfo) const
{
    /*StringBuilder name;

	// if our previous shit is a ident. than we can't be a type name
	if (tokens[-2].isIdentifier() && tokens[-1].isChar() && tokens[-1].ch() == '.')
		return 0;

    // push first token
    uint32_t pos = 1;
    ASSERT(tokens[0].isIdentifier());
    name << tokens[0].view();

    // best so far
    StringID bestTypeName;
    FunctionTypeInfo bestTypeInfo;
    uint32_t bestSize = 0;

    // check
    while (tokens)
    {
        // find a a type
        auto typeName = StringID(name.view());
        auto typeStub = m_ctx.resolveTypeFromName(typeName);
        if (typeStub)
        {
            // save as the best so far
            bestTypeName = typeName;
            bestTypeInfo = typeStub;
            bestSize = pos;
        }

        // do we have more of the '.Ident' pattern ?
        auto& next0 = tokens[pos];
        auto& next1 = tokens[pos+1];
        if (next0.isChar() && next0.ch() == '.' && next1.isIdentifier())
        {
            name << "." << next1.view();
            pos += 2;
        }
        else
        {
            // end of pattern
            break;
        }
    }

    // return best
    if (bestSize > 0)
    {
        outTypeInfo = bestTypeInfo;
        outTypeName = bestTypeName;
        return bestSize;
    }

    // no matching type found*/
    return 0;
}

int FunctionParsingTokenStream::readToken(FunctionParsingNode& outNode)
{
    // end of stream
    if (m_tokens.empty())
        return 0;

    // get the token
    auto token  = m_tokens.popFront();
    m_lastTokenText = token->view();
    m_lastTokenLocation = token->location();

    // copy location of the token
    outNode.location = token->location();

    // fill info
    if (token->isString())
    {
        outNode.tokenID = TOKEN_STRING;
        outNode.stringValue = token->view();
    }
    else if (token->isName())
    {
        outNode.tokenID = TOKEN_NAME;
        outNode.stringValue = token->view();
        outNode.name = StringID(token->view());
    }
    else if (token->isFloat())
    {
        outNode.tokenID = TOKEN_FLOAT_NUMBER;
        outNode.floatValue = token->floatNumber();
        outNode.stringValue = token->view();
    }
    else if (token->isInteger())
    {
        outNode.tokenID = TOKEN_INT_NUMBER;
        outNode.intValue = (int64_t)token->floatNumber();
        outNode.stringValue = token->view();
    }
    else if (token->isIdentifier())
    {
        if (auto len = matchTypeName(outNode.name, outNode.type))
        {
            outNode.tokenID = TOKEN_TYPE;
            // tokens.advance(len); // eat more tokens
        }
        else
        {
            outNode.name = StringID(token->view());
            outNode.tokenID = TOKEN_IDENT;
            outNode.stringValue = token->view();
        }
    }
    else if (token->isChar())
    {
        outNode.tokenID = token->ch();
    }
    else
    {
        outNode.tokenID = token->keywordID();
    }

    return outNode.tokenID;
}

//--

FunctionParsingContext::FunctionParsingContext(mem::LinearAllocator& mem, FunctionParser& fileParser, const StubFunction* function, FunctionCode& outCode)
    : m_parser(fileParser)
    , m_function(function)
    , m_code(outCode)
    , m_mem(mem)
{
}

StubLocation FunctionParsingContext::mapLocation(const parser::Location& location)
{
    StubLocation ret;
    ret.file = m_function->location.file;
    ret.line = location.line(); // we assume functions don't span multiple files :P
    return ret;
}

FunctionNode* FunctionParsingContext::createNode(const parser::Location& location, FunctionNodeOp op)
{
    auto ret  = m_mem.create<FunctionNode>();
    ret->location = mapLocation(location);
    ret->op = op;
    return ret;
}

FunctionNode* FunctionParsingContext::createNode(const parser::Location& location, FunctionNodeOp op, FunctionNode* a)
{
    auto ret  = m_mem.create<FunctionNode>();
    ret->location = mapLocation(location);
    ret->op = op;
    ret->add(a);
    return ret;
}

FunctionNode* FunctionParsingContext::createNode(const parser::Location& location, FunctionNodeOp op, FunctionNode* a, FunctionNode* b)
{
    auto ret  = m_mem.create<FunctionNode>();
    ret->location = mapLocation(location);
    ret->op = op;
    ret->add(a);
    ret->add(b);
    return ret;
}

FunctionNode* FunctionParsingContext::createNode(const parser::Location& location, FunctionNodeOp op, FunctionNode* a, FunctionNode* b, FunctionNode* c)
{
    auto ret  = m_mem.create<FunctionNode>();
    ret->location = mapLocation(location);
    ret->op = op;
    ret->add(a);
    ret->add(b);
    ret->add(c);
    return ret;
}

void FunctionParsingContext::rootStatement(FunctionNode* node)
{
    m_code.rootNode(node);
}

FunctionNode* FunctionParsingContext::makeBreakpoint(FunctionNode* node)
{
    if (!node)
        return nullptr;

    auto ret  = m_mem.create<FunctionNode>();
    ret->location = node->location;
    ret->op = FunctionNodeOp::Statement;
    ret->add(node);
    return ret;
}

FunctionTypeInfo FunctionParsingContext::createStaticArrayType(FunctionTypeInfo innerType, uint32_t arraySize)
{
    if (!innerType)
        return FunctionTypeInfo();

    auto typeDecl  = m_parser.stubs().createStaticArrayType(innerType.type->location, innerType.type, arraySize);
    return FunctionTypeInfo(typeDecl);
}

FunctionTypeInfo FunctionParsingContext::createDynamicArrayType(FunctionTypeInfo innerType)
{
    if (!innerType)
        return FunctionTypeInfo();

    auto typeDecl  = m_parser.stubs().createDynamicArrayType(innerType.type->location, innerType.type);
    return FunctionTypeInfo(typeDecl);
}

FunctionTypeInfo FunctionParsingContext::resolveTypeFromName(StringID name)
{
    return m_parser.stubs().resolveSimpleTypeReference(name, m_function);
}

FunctionTypeInfo FunctionParsingContext::createClassType(const parser::Location& location, StringID className)
{
    auto typeDecl  = m_parser.stubs().resolveSimpleTypeReference(className, m_function);
    if (!typeDecl)
    {
        reportError(location, TempString("Unrecognized type '{}'", className));
        return FunctionTypeInfo();
    }
    else if (!typeDecl->referencedType->resolvedStub->asClass())
    {
        reportError(location, TempString("Type '{}' is not a class", className));
        return FunctionTypeInfo();
    }

    return m_parser.stubs().createClassType(mapLocation(location), typeDecl->referencedType->resolvedStub);
}

FunctionTypeInfo FunctionParsingContext::createPtrType(const parser::Location& location, StringID className)
{
    auto typeDecl  = m_parser.stubs().resolveSimpleTypeReference(className, m_function);
    if (!typeDecl)
    {
        reportError(location, TempString("Unrecognized type '{}'", className));
        return FunctionTypeInfo();
    }
    else if (!typeDecl->referencedType->resolvedStub->asClass())
    {
        reportError(location, TempString("Type '{}' is not a class", className));
        return FunctionTypeInfo();
    }

    return m_parser.stubs().createSharedPointerType(mapLocation(location), typeDecl->referencedType->resolvedStub);
}

FunctionTypeInfo FunctionParsingContext::createWeakPtrType(const parser::Location& location, StringID className)
{
    auto typeDecl  = m_parser.stubs().resolveSimpleTypeReference(className, m_function);
    if (!typeDecl)
    {
        reportError(location, TempString("Unrecognized type '{}'", className));
        return FunctionTypeInfo();
    }
    else if (!typeDecl->referencedType->resolvedStub->asClass())
    {
        reportError(location, TempString("Type '{}' is not a class", className));
        return FunctionTypeInfo();
    }

    return m_parser.stubs().createWeakPointerType(mapLocation(location), typeDecl->referencedType->resolvedStub);
}

FunctionNode* FunctionParsingContext::createIntConst(const parser::Location& location, int64_t val)
{
    auto ret  = createNode(location, FunctionNodeOp::Const);
    ret->data.number = FunctionNumber(val);

    /*if (val >= std::numeric_limits<char>::min() && val <= std::numeric_limits<char>::max())
        ret->type = resolveTypeFromName("int8"_id).makeConst();
    else if (val >= std::numeric_limits<short>::min() && val <= std::numeric_limits<short>::max())
        ret->type = resolveTypeFromName("int16"_id).makeConst();
    else*/ if (val >= std::numeric_limits<int>::min() && val <= std::numeric_limits<int>::max())
        ret->type = resolveTypeFromName("int"_id).makeConst();
    else
        ret->type = resolveTypeFromName("int64"_id).makeConst();

    return ret;
}

FunctionNode* FunctionParsingContext::createUintConst(const parser::Location& location, uint64_t val)
{
    auto ret  = createNode(location, FunctionNodeOp::Const);
    ret->data.number = FunctionNumber(val);

    /*if (val <= std::numeric_limits<uint8_t>::max())
        ret->type = resolveTypeFromName("uint8"_id).makeConst();
    else if (val <= std::numeric_limits<uint16_t>::max())
        ret->type = resolveTypeFromName("uint16"_id).makeConst();
    else*/ if (val <= std::numeric_limits<uint32_t>::max())
        ret->type = resolveTypeFromName("uint"_id).makeConst();
    else
        ret->type = resolveTypeFromName("uint64"_id).makeConst();

    return ret;
}

FunctionNode* FunctionParsingContext::createFloatConst(const parser::Location& location, double val)
{
    auto ret  = createNode(location, FunctionNodeOp::Const);
    ret->data.number = FunctionNumber(val);
    ret->type = resolveTypeFromName("float"_id).makeConst();
    return ret;
}

FunctionNode* FunctionParsingContext::createBoolConst(const parser::Location& location, bool val)
{
    auto ret  = createNode(location, FunctionNodeOp::Const);
    ret->data.number = FunctionNumber((int64_t)(val ? 1 : 0));
    ret->type = resolveTypeFromName("bool"_id).makeConst();
    return ret;
}

FunctionNode* FunctionParsingContext::createStringConst(const parser::Location& location, StringView val)
{
    auto ret  = createNode(location, FunctionNodeOp::Const);
    ret->data.text = val;
    ret->type = resolveTypeFromName("string"_id).makeConst().makeRef();
    return ret;
}

FunctionNode* FunctionParsingContext::createNameConst(const parser::Location& location, StringID val)
{
    auto ret  = createNode(location, FunctionNodeOp::Const);
    ret->data.name = val;
    ret->type = resolveTypeFromName("strid"_id).makeConst();
    return ret;
}

FunctionNode* FunctionParsingContext::createNullConst(const parser::Location& location)
{
    auto ret  = createNode(location, FunctionNodeOp::Null);
    return ret;
}

FunctionNode* FunctionParsingContext::createClassTypeConst(const parser::Location& location, StringID className)
{
    auto typeDecl  = m_parser.stubs().resolveSimpleTypeReference(className, m_function);
    if (!typeDecl)
    {
        reportError(location, TempString("Unrecognized type '{}'", className));
        return nullptr;
    }
    else if (!typeDecl->referencedType->resolvedStub->asClass())
    {
        reportError(location, TempString("Type '{}' is not a class", className));
        return nullptr;
    }

    auto ret  = createNode(location, FunctionNodeOp::Const);
    ret->type = m_parser.stubs().createClassType(mapLocation(location), typeDecl->referencedType->resolvedStub);
    ret->data.type = ret->type;
    return ret;
}

void FunctionParsingContext::pushContextNode(FunctionNode* node)
{
    m_contextStack.pushBack(node);
}

void FunctionParsingContext::popContextNode()
{
    if (!m_contextStack.empty())
        m_contextStack.popBack();
}

FunctionNode* FunctionParsingContext::findContextNode(FunctionNodeOp op)
{
    for (int i=m_contextStack.lastValidIndex(); i >= 0; --i)
        if (m_contextStack[i]->op == op)
            return m_contextStack[i];

    return nullptr;
}

FunctionNode* FunctionParsingContext::findBreakContextNode()
{
    for (int i=m_contextStack.lastValidIndex(); i >= 0; --i)
        if (m_contextStack[i]->op == FunctionNodeOp::For || m_contextStack[i]->op == FunctionNodeOp::DoWhile || m_contextStack[i]->op == FunctionNodeOp::While || m_contextStack[i]->op == FunctionNodeOp::Switch)
            return m_contextStack[i];

    return nullptr;
}

FunctionNode* FunctionParsingContext::findContinueContextNode()
{
    for (int i=m_contextStack.lastValidIndex(); i >= 0; --i)
        if (m_contextStack[i]->op == FunctionNodeOp::For || m_contextStack[i]->op == FunctionNodeOp::DoWhile || m_contextStack[i]->op == FunctionNodeOp::While)
            return m_contextStack[i];

    return nullptr;
}

void FunctionParsingContext::reportError(const parser::Location& location, StringView message)
{
    m_parser.errorHandler().reportError(mapLocation(location).file->absolutePath, location.line(), message);
}

//--

END_BOOMER_NAMESPACE_EX(script)
