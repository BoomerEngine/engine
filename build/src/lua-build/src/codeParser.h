#pragma once

#include "utils.h"

//--

struct CodeParserState;

struct CodeTokenizer
{
    //--

    struct Conditional;

    struct CodeToken
    {
        string_view text;
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
        string_view name;
        vector<string_view> namespaces;
    };

    struct CodeSection
    {        
        vector<CodeToken> tokens;
        vector<Declaration> declarations;
    };

    struct ConditionalOption
    {
        CodeToken command; // #ifdef #if #ifndef 
        CodeToken arguments; // actual argument
        CodeSection* section = nullptr;
    };

    struct Conditional
    {
        vector<ConditionalOption> options;
        CodeSection* currentSection = nullptr;
    };

    //--

    //
    // #if A
    //   code
    // #elif C
    //   code3
    //   #if B
    //     code2
    //   #endif
    //   code4
    // #else
    //   #if B
    //     code2
    //   #endif
    // #endif

    filesystem::path contextPath;

    CodeSection* rootSection = nullptr;

    CodeTokenizer();
    ~CodeTokenizer();

    bool tokenize(string_view txt);

    bool process();

private:
    string code;

    CodeSection* currentSection = nullptr;
    vector<Conditional*> conditionalStack;

    void emitToken(CodeToken txt);

    void handleComment(CodeParserState& s);
    void handleSingleLineComment(CodeParserState& s);
    void handleMultiLineComment(CodeParserState& s);
    void handleString(CodeParserState& s);
    void handleSingleChar(CodeParserState& s);
    void handleIdent(CodeParserState& s);
    bool handlePreprocessor(CodeParserState& s);


    struct ScopeState
    {
        struct Scope
        {
            const CodeToken* entryToken;
            bool isNamespace = false;
        };

        vector<Scope> scopes;
        vector<string_view> namespaces;
    };

    bool processCodeSection(CodeSection* section, ScopeState& state);
};


//--