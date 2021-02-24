#include "common.h"
#include "codeParser.h"

//--

CodeTokenizer::CodeTokenizer()
{
}

CodeTokenizer::~CodeTokenizer()
{
}

struct CodeParserState
{
    const string_view txt;
    const char* pos = nullptr;
    const char* end = nullptr;
    int line = 1;
    bool lineStart = true;

    CodeParserState(string_view txt)
        : txt(txt)
    {
        pos = txt.data();
        end = pos + txt.length();
    }

    inline bool hasContent() const
    {
        return pos < end;
    }

    inline char peek() const
    {
        return pos < end ? *pos : 0;
    }

    inline void eat()
    {
        if (pos < end)
        {
            char ch = *pos++;
            if (ch == '\n')
            {
                lineStart = true;
                line++;
            }
            else if (ch > ' ')
            {
                lineStart = false;
            }
        }
    }

    inline CodeTokenizer::CodeToken token(const char* fromPos, int fromLine, CodeTokenizer::CodeTokenType type)
    {
        CodeTokenizer::CodeToken ret;
        ret.type = type;
        ret.text = string_view(fromPos, pos - fromPos);
        ret.line = fromLine;
        return ret;
    }
};

static inline bool IsTokenChar(char ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || (ch == '_') || (ch == '.');
}

static inline bool IsNumberChar(char ch)
{
    return (ch >= '0' && ch <= '9');
}

bool CodeTokenizer::tokenize(string_view txt)
{
    code = txt;

    CodeParserState state(code);

    while (state.hasContent())
    {
        char ch = state.peek();

        if (ch == '/')
        {
            handleComment(state);
        }
        else if (ch == '\"' || ch == '\'')
        {
            handleString(state);
        }
        else if (ch <= ' ')
        {
            state.eat(); // whitespace
        }
        else if (ch == '#' && state.lineStart)
        {
            if (!handlePreprocessor(state))
                return false;
        }
        else if (IsTokenChar(ch))
        {
            handleIdent(state);
        }
        else if (IsNumberChar(ch))
        {
            handleNumber(state);
        }
        else
        {
            handleSingleChar(state);
        }
    }

    return true;
}

void CodeTokenizer::handleComment(CodeParserState& s)
{
    s.eat();

    if (s.peek() == '*') {
        s.eat();
        handleMultiLineComment(s);
    }
    else if (s.peek() == '/')
    {
        s.eat();
        handleSingleLineComment(s);
    }
}

void CodeTokenizer::handleSingleLineComment(CodeParserState& s)
{
    while (s.hasContent())
    {
        char ch = s.peek();
        if (ch == '\n')
            break;
        s.eat();
    }
}

void CodeTokenizer::handleMultiLineComment(CodeParserState& s)
{
    while (s.hasContent())
    {
        char ch = s.peek();
        if (ch == '*') {
            s.eat();

            ch = s.peek();
            if (ch == '/') {
                s.eat();
                break;
            }
        }
        else
        {
            s.eat();
        }
    }
}

void CodeTokenizer::emitToken(CodeToken txt)
{
    tokens.push_back(txt);
}

void CodeTokenizer::handleString(CodeParserState& s)
{
    char delim = s.peek();
    s.eat();

    auto fromPos = s.pos;
    auto fromLine = s.line;

    while (s.hasContent())
    {
        char ch = s.peek();

        if (ch == '\\')
        {
            s.eat();
            s.eat(); // eat the escaped character
        }
        else if (ch == delim)
        {
            break;
        }
        else
        {
            s.eat();
        }
    }

    emitToken(s.token(fromPos, fromLine, CodeTokenType::STRING));

    s.eat();    
}

void CodeTokenizer::handleIdent(CodeParserState& s)
{
    auto fromPos = s.pos;
    auto fromLine = s.line;

    while (s.hasContent())
    {
        char ch = s.peek();
        if (!IsTokenChar(ch))
            break;
        s.eat();
    }

    emitToken(s.token(fromPos, fromLine, CodeTokenType::IDENT));
}

void CodeTokenizer::handleNumber(CodeParserState& s)
{
    auto fromPos = s.pos;
    auto fromLine = s.line;

    while (s.hasContent())
    {
        char ch = s.peek();
        if (!IsNumberChar(ch))
            break;
        s.eat();
    }

    emitToken(s.token(fromPos, fromLine, CodeTokenType::NUMBER));
}

void CodeTokenizer::handleSingleChar(CodeParserState& s)
{
    auto fromPos = s.pos;
    auto fromLine = s.line;

    s.eat();

    emitToken(s.token(fromPos, fromLine, CodeTokenType::CHAR));
}

bool CodeTokenizer::handlePreprocessor(CodeParserState& s)
{
    const auto* lineStart = s.pos;

    s.eat(); // #

    while (s.hasContent())
    {
        char ch = s.peek();
        if (ch == '\n')
        {
            cout << contextPath.u8string() << "(" << s.line << "): error: Invalid preprocessor directive\n";
            return false;
        }

        if (ch > ' ')
            break;
        s.eat();
    }

    auto fromPos = s.pos;
    auto fromLine = s.line;

    while (s.hasContent())
    {
        char ch = s.peek();
        if (ch <= ' ')
            break;
        if (ch == '(' || ch == '<' || ch == '\"')
            break;
        s.eat();
    }

    auto command = s.token(fromPos, fromLine, CodeTokenType::IDENT);

    /*while (s.hasContent())
    {
        char ch = s.peek();
        if (ch > ' ')
            break;
        s.eat();
    }*/

    fromPos = s.pos;
    while (s.hasContent())
    {
        char ch = s.peek();
        if (ch == '\n')
            break;
        s.eat();
    }

    auto arguments = s.token(fromPos, fromLine, CodeTokenType::STRING);

    // TODO: any "#pragma" ?

    return true;
}

//--

struct TokenStream
{
    TokenStream(const vector<CodeTokenizer::CodeToken>& tokens)
        : tokens(tokens)
    {
        pos = 0;
        end = (int)tokens.size();
    }

    inline bool hasContent() const
    {
        return pos < end;
    }

    inline const CodeTokenizer::CodeToken& peek(int offset = 0) const
    {
        static CodeTokenizer::CodeToken theEmptyToken;
        return (pos+offset) < end ? tokens[pos+offset] : theEmptyToken;
    }

    inline void eat(int count = 1)
    {
        pos += count;
    }

    const vector<CodeTokenizer::CodeToken>& tokens;
    int pos = 0;
    int end = 0;
};

bool CodeTokenizer::ExtractNamespaceName(TokenStream& s, string& outName)
{
    stringstream ss;

    {
        const auto& bracket = s.peek();
        if (bracket.type != CodeTokenType::CHAR || bracket.text != "(")
            return false;
        s.eat();
    }

    bool hasParts = false;

    while (s.hasContent())
    {
        if (s.peek().text == ")")
        {
            s.eat();

            outName = ss.str();
            return true;
        }

        if (hasParts)
        {
            if (s.peek(0).text != ":" || s.peek(1).text != ":")
                return false;
            s.eat(2);
            ss << "::";
        }

        if (s.peek().type != CodeTokenType::IDENT)
            return false;

        ss << s.peek().text;
        hasParts = true;

        s.eat();
    }

    return false;
}

bool CodeTokenizer::process()
{
    TokenStream s(tokens);

    bool print = EndsWith(contextPath.u8string(), "file.c");

    string activeNamespace = "";

    while (s.hasContent())
    {
        const auto& token = s.peek();
        s.eat();

        if (print)
            cout << "Token '" << token.text << "' at line " << token.line << "\n";

        if (token.text == "BEGIN_BOOMER_NAMESPACE" || token.text == "BEGIN_BOOMER_NAMESPACE_EX")
        {
            if (!activeNamespace.empty())
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Nested BEGIN_BOOMER_NAMESPACE are not allowed\n";
                return false;
            }

            if (!ExtractNamespaceName(s, activeNamespace))
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Unable to parse namespace's name\n";
                return false;
            }
        }
        else if (token.text == "END_BOOMER_NAMESPACE")
        {
            if (activeNamespace.empty())
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Found END_BOOMER_NAMESPACE without previous BEGIN_BOOMER_NAMESPACE\n";
                return false;
            }

            string name;
            if (!ExtractNamespaceName(s, name))
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Unable to parse namespace's name\n";
                return false;
            }

            if (name != activeNamespace)
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Inconsistent namesapce name between BEGIN and END macros\n";
                return false;
            }

            activeNamespace.clear();
        }
        else if (token.text == "RTTI_BEGIN_TYPE_ENUM" || token.text == "BEGIN_BOOMER_TYPE_ENUM")
        {
            if (activeNamespace.empty())
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Type declaration can only happen inside the boomer namespace BEGIN/END block\n";
                return false;
            }

            string name;
            if (!ExtractNamespaceName(s, name))
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Unable to parse type's name\n";
                return false;
            }

            Declaration decl;
            decl.name = name;
            decl.scope = activeNamespace;
            decl.type = DeclarationType::ENUM;
            declarations.push_back(decl);
        }
        else if (token.text == "RTTI_BEGIN_TYPE_BITFIELD" || token.text == "BEGIN_BOOMER_TYPE_BITFIELD")
        {
            if (activeNamespace.empty())
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Type declaration can only happen inside the boomer namespace BEGIN/END block\n";
                return false;
            }

            string name;
            if (!ExtractNamespaceName(s, name))
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Unable to parse type's name\n";
                return false;
            }

            Declaration decl;
            decl.name = name;
            decl.scope = activeNamespace;
            decl.type = DeclarationType::BITFIELD;
            declarations.push_back(decl);
        }
        else if (token.text == "RTTI_BEGIN_TYPE_NATIVE_CLASS" || token.text == "BEGIN_BOOMER_TYPE_RUNTIME_CLASS" 
            || token.text == "RTTI_BEGIN_TYPE_ABSTRACT_CLASS" || token.text == "BEGIN_BOOMER_TYPE_ABSTRACT_CLASS"
            || token.text == "RTTI_BEGIN_TYPE_CLASS" || token.text == "BEGIN_BOOMER_TYPE_CLASS"
            || token.text == "RTTI_BEGIN_TYPE_STRUCT" || token.text == "BEGIN_BOOMER_TYPE_STRUCT")
            
        {
            if (activeNamespace.empty())
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Type declaration can only happen inside the boomer namespace BEGIN/END block\n";
                return false;
            }

            string name;
            if (!ExtractNamespaceName(s, name))
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Unable to parse type's name\n";
                return false;
            }

            Declaration decl;
            decl.name = name;
            decl.scope = activeNamespace;
            decl.type = DeclarationType::CLASS;
            declarations.push_back(decl);
        }
        else if (token.text == "RTTI_BEGIN_CUSTOM_TYPE" || token.text == "BEGIN_BOOMER_CUSTOM_TYPE")
        {
            if (activeNamespace.empty())
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Type declaration can only happen inside the boomer namespace BEGIN/END block\n";
                return false;
            }

            string name;
            if (!ExtractNamespaceName(s, name))
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Unable to parse type's name\n";
                return false;
            }

            Declaration decl;
            decl.name = name;
            decl.scope = activeNamespace;
            decl.type = DeclarationType::CUSTOM_TYPE;
            declarations.push_back(decl);
        }
    }

    return true;
}