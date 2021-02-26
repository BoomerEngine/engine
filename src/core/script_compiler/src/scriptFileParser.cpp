/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptLibrary.h"
#include "scriptFileParser.h"
#include "scriptFileParserHelper.h"

#include "core/parser/include/textLanguageDefinition.h"
#include "core/parser/include/textSimpleLanguageDefinition.h"
#include "core/parser/include/textErrorReporter.h"
#include "core/parser/include/textFilePreprocessor.h"
#include "core/parser/include/textToken.h"

#include "scriptFileStructureParser_Symbols.h"
//#include "renderingShaderFileCodeParser_Symbols.h"

extern int bsc_parse(boomer::script::FileParsingContext& context, boomer::script::FileParsingTokenStream& tokens);

BEGIN_BOOMER_NAMESPACE_EX(script)

///---

class LanguageDefinition : public ISingleton
{
    DECLARE_SINGLETON(LanguageDefinition);

public:
    LanguageDefinition()
    {
        parser::SimpleLanguageDefinitionBuilder defs;

        //auto IdentFirstChars  = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01"; // NOTE: added 01 for swizzles
        //auto IdentNextChars  = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        //defs.identChars(IdentFirstChars, IdentNextChars);

        defs.enableIntegerNumbers(true);
        defs.enableFloatNumbers(true);
        defs.enableStrings(true);
        defs.enableNames(true);

        defs.addKeyword("using", TOKEN_USING);
        defs.addKeyword("override", TOKEN_OVERRIDE);
        defs.addKeyword("this", TOKEN_THIS);
        defs.addKeyword("super", TOKEN_SUPER);
        defs.addKeyword("in", TOKEN_IN);
        defs.addKeyword("extends", TOKEN_EXTENDS);
        defs.addKeyword("parent", TOKEN_PARENT);
        defs.addKeyword("state", TOKEN_STATE);
        defs.addKeyword("struct", TOKEN_STRUCT);
        defs.addKeyword("const", TOKEN_CONST);
        defs.addKeyword("class", TOKEN_CLASS);
        defs.addKeyword("array", TOKEN_ARRAY);
        defs.addKeyword("function", TOKEN_FUNCTION);
        defs.addKeyword("alias", TOKEN_ALIAS);
        defs.addKeyword("signal", TOKEN_SIGNAL);
        defs.addKeyword("property", TOKEN_PROPERTY);
        defs.addKeyword("var", TOKEN_VAR);
        defs.addKeyword("local", TOKEN_LOCAL);
        defs.addKeyword("ptr", TOKEN_PTR);
        defs.addKeyword("weak", TOKEN_WEAK);
        defs.addKeyword("enum", TOKEN_ENUM);
        defs.addKeyword("true", TOKEN_BOOL_TRUE);
        defs.addKeyword("false", TOKEN_BOOL_FALSE);
        defs.addKeyword("ref", TOKEN_REF);
        defs.addKeyword("out", TOKEN_OUT);
        defs.addKeyword("protected", TOKEN_PROTECTED);
        defs.addKeyword("private", TOKEN_PRIVATE);
        defs.addKeyword("import", TOKEN_IMPORT);
        defs.addKeyword("abstract", TOKEN_ABSTRACT);
        defs.addKeyword("static", TOKEN_STATIC);
        defs.addKeyword("final", TOKEN_FINAL);
        defs.addKeyword("inlined", TOKEN_INLINED);
        defs.addKeyword("editable", TOKEN_EDITABLE);
        defs.addKeyword("break", TOKEN_BREAK);
        defs.addKeyword("continue", TOKEN_CONTINUE);
        defs.addKeyword("return", TOKEN_RETURN);
        defs.addKeyword("do", TOKEN_DO);
        defs.addKeyword("while", TOKEN_WHILE);
        defs.addKeyword("if", TOKEN_IF);
        defs.addKeyword("else", TOKEN_ELSE);
        defs.addKeyword("for", TOKEN_FOR);
        defs.addKeyword("case", TOKEN_CASE);
        defs.addKeyword("switch", TOKEN_SWITCH);
        defs.addKeyword("default", TOKEN_DEFAULT);
        defs.addKeyword("opcode", TOKEN_OPCODE);
        defs.addKeyword("operator", TOKEN_OPERATOR);
        defs.addKeyword("cast", TOKEN_CAST);
        defs.addKeyword("explicit", TOKEN_EXPLICIT);
        defs.addKeyword("unsafe", TOKEN_UNSAFE);
        defs.addKeyword("typename", TOKEN_TYPENAME);
        defs.addKeyword("engineType", TOKEN_ENGINE_TYPE);
        defs.addKeyword("type", TOKEN_TYPE_TYPE);
        defs.addKeyword("new", TOKEN_NEW);
        defs.addKeyword("null", TOKEN_NULL);

        defs.addKeyword("+=", TOKEN_ADD_ASSIGN);
        defs.addKeyword("-=", TOKEN_SUB_ASSIGN);
        defs.addKeyword("*=", TOKEN_MUL_ASSIGN);
        defs.addKeyword("/=", TOKEN_DIV_ASSIGN);
        defs.addKeyword("&=", TOKEN_AND_ASSIGN);
        defs.addKeyword("|=", TOKEN_OR_ASSIGN);
        defs.addKeyword("||", TOKEN_OR_OP);
        defs.addKeyword("&&", TOKEN_AND_OP);
        defs.addKeyword("!=", TOKEN_NE_OP);
        defs.addKeyword("==", TOKEN_EQ_OP);
        defs.addKeyword(">=", TOKEN_GE_OP);
        defs.addKeyword("<=", TOKEN_LE_OP);
        defs.addKeyword("<<", TOKEN_LEFT_OP);
        defs.addKeyword(">>", TOKEN_RIGHT_OP);
        defs.addKeyword(">>>", TOKEN_RIGHT_RIGHT_OP);
        defs.addKeyword("<<=", TOKEN_LEFT_ASSIGN);
        defs.addKeyword(">>=", TOKEN_RIGHT_ASSIGN);
        defs.addKeyword(">>>=", TOKEN_RIGHT_RIGHT_ASSIGN);
        defs.addKeyword("@=", TOKEN_AT_ASSIGN);
        defs.addKeyword("^=", TOKEN_HAT_ASSIGN);
        defs.addKeyword("#=", TOKEN_HASH_ASSIGN);
        defs.addKeyword("$=", TOKEN_DOLAR_ASSIGN);
        defs.addKeyword("++", TOKEN_INC_OP);
        defs.addKeyword("--", TOKEN_DEC_OP);

        defs.addChar('(');
        defs.addChar(')');
        defs.addChar('{');
        defs.addChar('}');
        defs.addChar('[');
        defs.addChar(']');
        defs.addChar('<');
        defs.addChar('>');
        defs.addChar(',');
        defs.addChar('=');
        defs.addChar(';');
        defs.addChar('&');
        defs.addChar('|');
        defs.addChar('^');
        defs.addChar('+');
        defs.addChar('-');
        defs.addChar('*');
        defs.addChar('/');
        defs.addChar('%');
        defs.addChar('.');
        defs.addChar('!');
        defs.addChar('?');
        defs.addChar(':');
        defs.addChar('~');

        m_language = defs.buildLanguageDefinition();
    }

    INLINE const parser::ILanguageDefinition& definitions() const
    {
        return *m_language;
    }

private:
    UniquePtr<parser::ILanguageDefinition> m_language;

    virtual void deinit() override
    {
        m_language.reset();
    }
};

///---

// adapter for error interface
class ParserErrorHandler : public parser::IErrorReporter
{
public:
    ParserErrorHandler(IErrorHandler& err)
        : m_err(err)
    {}

    virtual void reportError(const parser::Location& loc, StringView message) override final
    {
        m_err.reportError(loc.contextName(), loc.line(), message);
    }

    virtual void reportWarning(const parser::Location& loc, StringView message) override final
    {
        m_err.reportWarning(loc.contextName(), loc.line(), message);
    }

private:
    IErrorHandler& m_err;
};

///---

IErrorHandler::~IErrorHandler()
{}

///---

FileParser::FileParser(mem::LinearAllocator& mem, IErrorHandler& err, StubLibrary& library)
    : m_mem(mem)
    , m_err(err)
    , m_library(library)
{}

FileParser::~FileParser()
{}

bool FileParser::processCode(const StubFile* fileStub, const Buffer& code)
{
    // get language definitions
    auto& lang = LanguageDefinition::GetInstance().definitions();

    // tokenize
    ParserErrorHandler parserErrorHandler(m_err);
    auto fileParser  = m_mem.create<parser::TextFilePreprocessor>(m_mem, parser::IIncludeHandler::GetEmptyHandler(), parserErrorHandler, parser::ICommentEater::StandardComments(), lang);
    if (!fileParser->processContent(code, fileStub->depotPath))
        return true; // errors will be reported via the interface

    //tokens.printFull(logging::ILogStream::GetStream());

    // parse the shit
    FileParsingTokenStream stream(std::move(fileParser->tokens()));
    FileParsingContext fileContext(*this, fileStub, m_library.primaryModule());
    auto ret = bsc_parse(fileContext, stream);
    if (ret != 0)
        return false; // errors will be reported via callback

    // stubs processed
    return true;
}

///---

END_BOOMER_NAMESPACE_EX(script)
