/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptFunctionCode.h"
#include "scriptFunctionCodeParser.h"
#include "scriptFunctionCodeParserHelper.h"

#include "core/parser/include/textToken.h"
#include "scriptFunctionCodeParser_Symbols.h"

extern int bfc_parse(boomer::script::FunctionParsingContext& context, boomer::script::FunctionParsingTokenStream& tokens);

BEGIN_BOOMER_NAMESPACE_EX(script)

///---

FunctionParser::FunctionParser(LinearAllocator& mem, IErrorHandler& err, StubLibrary& stubs)
    : m_mem(mem)
    , m_err(err)
    , m_stubs(stubs)
{}

FunctionParser::~FunctionParser()
{}

bool FunctionParser::processCode(const StubFunction* functionStub, FunctionCode& outCode)
{
    // empty function, nothing to parser
    if (functionStub->tokens.empty())
        return true;

    // functionStub->tokens.printFull(logging::ILogStream::GetStream());

    // parse the shit
    FunctionParsingContext functionContext(m_mem, *this, functionStub, outCode);
    FunctionParsingTokenStream stream(functionStub->tokens, functionContext);
    auto ret = bfc_parse(functionContext, stream);
    if (ret != 0)
        return false; // errors will be reported via callback

    // parsed
    return true;
}

///---

END_BOOMER_NAMESPACE_EX(script)
