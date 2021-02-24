#pragma once

#include "utils.h"

//--

struct CodeParserState;
struct TokenStream;

struct CodeTokenizer
{
    //--

    struct Conditional;

    enum class CodeTokenType : uint8_t
    {
        CHAR,
        IDENT,
        NUMBER,
        STRING,
    };

    struct CodeToken
    {
        string_view text;
        CodeTokenType type;
        int line = 0;

        Conditional* cond = nullptr;
    };

    enum class DeclarationType : uint8_t
    {
        CLASS,
        CUSTOM_TYPE,
        ENUM,
        BITFIELD,
        GLOBAL_FUNC,
    };

    struct Declaration
    {
        DeclarationType type;
        string name;
        string scope; // namespace
    };

    //--

    filesystem::path contextPath;

    vector<CodeToken> tokens;

    vector<Declaration> declarations;

    CodeTokenizer();
    ~CodeTokenizer();

    bool tokenize(string_view txt);

    bool process();

private:
    string code;

    void emitToken(CodeToken txt);

    void handleComment(CodeParserState& s);
    void handleSingleLineComment(CodeParserState& s);
    void handleMultiLineComment(CodeParserState& s);
    void handleString(CodeParserState& s);
    void handleSingleChar(CodeParserState& s);
    void handleIdent(CodeParserState& s);
    void handleNumber(CodeParserState& s);
    bool handlePreprocessor(CodeParserState& s);

    static bool ExtractNamespaceName(TokenStream& tokens, string& outName);
};


//--