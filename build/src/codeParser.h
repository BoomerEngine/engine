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
        std::string_view text;
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
        std::string name;
        std::string scope; // namespace
        std::string typeName; // namespace without the "boomer::"
    };

    //--

    fs::path contextPath;

    std::vector<CodeToken> tokens;

    std::vector<Declaration> declarations;

    CodeTokenizer();
    ~CodeTokenizer();

    bool tokenize(std::string_view txt);

    bool process();

private:
    std::string code;

    void emitToken(CodeToken txt);

    void handleComment(CodeParserState& s);
    void handleSingleLineComment(CodeParserState& s);
    void handleMultiLineComment(CodeParserState& s);
    void handleString(CodeParserState& s);
    void handleSingleChar(CodeParserState& s);
    void handleIdent(CodeParserState& s);
    void handleNumber(CodeParserState& s);
    bool handlePreprocessor(CodeParserState& s);

    static bool ExtractNamespaceName(TokenStream& tokens, std::string& outName);
    static bool ExtractEmptyBrackets(TokenStream& tokens);
};


//--