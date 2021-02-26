/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "textParser.h"
#include "textParsingTreeBuilder.h"
#include "textSimpleLanguageDefinition.h"

#include "core/test/include/gtest/gtest.h"

BEGIN_BOOMER_NAMESPACE()

DECLARE_TEST_FILE(TextParser);

UniquePtr<parser::ILanguageDefinition> BuildLanguage()
{
    parser::SimpleLanguageDefinitionBuilder defs;
    defs.enableIntegerNumbers(true);
    defs.enableFloatNumbers(true);
    defs.enableStrings(true);
    defs.enableNames(true);

    int keyword = 300;
    defs.addKeyword("import", keyword++);
    defs.addKeyword("importonly", keyword++);
    defs.addKeyword("abstract", keyword++);
    defs.addKeyword("const", keyword++);
    defs.addKeyword("extends", keyword++);
    defs.addKeyword("in", keyword++);
    defs.addKeyword("class", keyword++);
    defs.addKeyword("enum", keyword++);
    defs.addKeyword("struct", keyword++);
    defs.addKeyword("function", 400);
    defs.addKeyword("def", keyword++);
    defs.addKeyword("editable", keyword++);
    defs.addKeyword("replicated", keyword++);
    defs.addKeyword("final", keyword++);
    defs.addKeyword("virtual", keyword++);
    defs.addKeyword("override", keyword++);
    defs.addKeyword("out", keyword++);
    defs.addKeyword("optional", keyword++);
    defs.addKeyword("skip", keyword++);
    defs.addKeyword("local", keyword++);
    defs.addKeyword("inlined", keyword++);
    defs.addKeyword("private", keyword++);
    defs.addKeyword("protected", keyword++);
    defs.addKeyword("public", keyword++);
    defs.addKeyword("event", keyword++);
    defs.addKeyword("timer", keyword++);
    defs.addKeyword("array", keyword++);
    defs.addKeyword("hint", keyword++);
    defs.addKeyword("true", keyword++);
    defs.addKeyword("false", keyword++);
    defs.addKeyword("NULL", keyword++);
    defs.addKeyword("var", keyword++);
    defs.addKeyword("exec", keyword++);
    defs.addKeyword("saved", keyword++);
    defs.addKeyword("weak", keyword++);
    defs.addKeyword("operator", keyword++);
    defs.addKeyword("cast", keyword++);
    defs.addKeyword("implicit", keyword++);
    defs.addKeyword("static", keyword++);
    defs.addKeyword("multicast", keyword++);
    defs.addKeyword("server", keyword++);
    defs.addKeyword("client", keyword++);
    defs.addKeyword("reliable", keyword++);
    defs.addKeyword("undecorated", keyword++);
    defs.addKeyword("new", 402);
    defs.addKeyword("delete", keyword++);
    defs.addKeyword("if", keyword++);
    defs.addKeyword("else", keyword++);
    defs.addKeyword("switch", keyword++);
    defs.addKeyword("case", keyword++);
    defs.addKeyword("default", keyword++);
    defs.addKeyword("for", keyword++);
    defs.addKeyword("while", keyword++);
    defs.addKeyword("do", keyword++);
    defs.addKeyword("return", 401);
    defs.addKeyword("break", keyword++);
    defs.addKeyword("continue", keyword++);
    defs.addKeyword("this", keyword++);
    defs.addKeyword("super", keyword++);
    defs.addKeyword("+=", keyword++);
    defs.addKeyword("-=", keyword++);
    defs.addKeyword("*=", keyword++);
    defs.addKeyword("/=", keyword++);
    defs.addKeyword("&=", keyword++);
    defs.addKeyword("|=", keyword++);
    defs.addKeyword("||", keyword++);
    defs.addKeyword("&&", keyword++);
    defs.addKeyword("!=", keyword++);
    defs.addKeyword("==", keyword++);
    defs.addKeyword(">=", keyword++);
    defs.addKeyword("<=", keyword++);

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

    return defs.buildLanguageDefinition();
}


TEST(ComplexParser, FullTest)
{
    auto lang = BuildLanguage();

    parser::TextParser p;
    auto str  = "void function Test() { return new Dupa(123, \"Test\", 'Ident', 3.14); }";
    p.reset(str);

    parser::Token token;

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isIdentifier());
        ASSERT_EQ(StringBuf("void"), t.string());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isKeyword());
        ASSERT_EQ(StringBuf("function"), t.string());
        ASSERT_EQ(400, t.keywordID());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isIdentifier());
        ASSERT_EQ(StringBuf("Test"), t.string());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isChar());
        ASSERT_EQ(StringBuf("("), t.string());
        ASSERT_EQ('(', t.ch());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isChar());
        ASSERT_EQ(StringBuf(")"), t.string());
        ASSERT_EQ(')', t.ch());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isChar());
        ASSERT_EQ(StringBuf("{"), t.string());
        ASSERT_EQ('{', t.ch());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isKeyword());
        ASSERT_EQ(StringBuf("return"), t.string());
        ASSERT_EQ(401, t.keywordID());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isKeyword());
        ASSERT_EQ(StringBuf("new"), t.string());
        ASSERT_EQ(402, t.keywordID());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isIdentifier());
        ASSERT_EQ(StringBuf("Dupa"), t.string());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isChar());
        ASSERT_EQ(StringBuf("("), t.string());
        ASSERT_EQ('(', t.ch());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isNumber());
        ASSERT_TRUE(t.isInteger());
        ASSERT_EQ(123.0, t.floatNumber());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isChar());
        ASSERT_EQ(StringBuf(","), t.string());
        ASSERT_EQ(',', t.ch());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isString());
        ASSERT_EQ(StringBuf("Test"), t.string());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isChar());
        ASSERT_EQ(StringBuf(","), t.string());
        ASSERT_EQ(',', t.ch());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isName());
        ASSERT_EQ(StringBuf("Ident"), t.string());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isChar());
        ASSERT_EQ(StringBuf(","), t.string());
        ASSERT_EQ(',', t.ch());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isNumber());
        ASSERT_TRUE(t.isFloat());
        ASSERT_TRUE(abs(3.14 - t.floatNumber()) < 0.001);
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isChar());
        ASSERT_EQ(StringBuf(")"), t.string());
        ASSERT_EQ(')', t.ch());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isChar());
        ASSERT_EQ(StringBuf(";"), t.string());
        ASSERT_EQ(';', t.ch());
    }

    {
        auto t = p.parseToken(*lang);
        ASSERT_TRUE(t.isChar());
        ASSERT_EQ(StringBuf("}"), t.string());
        ASSERT_EQ('}', t.ch());
    }
}

END_BOOMER_NAMESPACE()