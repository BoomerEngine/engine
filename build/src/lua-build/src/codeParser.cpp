#include "common.h"
#include "codeParser.h"

//--

CodeTokenizer::CodeTokenizer()
{
    rootSection = new CodeSection();
    currentSection = rootSection;
}

CodeTokenizer::~CodeTokenizer()
{
    delete rootSection;
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

    inline CodeTokenizer::CodeToken token(const char* fromPos, int fromLine)
    {
        CodeTokenizer::CodeToken ret;
        ret.text = string_view(fromPos, pos - fromPos);
        ret.line = fromLine;
        return ret;
    }
};

static inline bool IsTokenChar(char ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || (ch == '_') || (ch == '.');
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
    currentSection->tokens.push_back(txt);
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

    emitToken(s.token(fromPos, fromLine));

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

    emitToken(s.token(fromPos, fromLine));
}

void CodeTokenizer::handleSingleChar(CodeParserState& s)
{
    auto fromPos = s.pos;
    auto fromLine = s.line;

    s.eat();

    emitToken(s.token(fromPos, fromLine));
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

    auto command = s.token(fromPos, fromLine);

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

    auto arguments = s.token(fromPos, fromLine);
    
    //cout << "(" << fromLine << "): " << command.text << " " << arguments.text << "\n";

    if (command.text == "endif")
    {
        if (conditionalStack.empty())
        {
            cout << contextPath.u8string() << "(" << fromLine << "): error:  Unexpected #endif\n";
            return false;
        }

        conditionalStack.pop_back();

        if (conditionalStack.empty())
            currentSection = rootSection;
        else
            currentSection = conditionalStack.back()->currentSection;
    }
    else if (command.text == "if" || command.text == "ifdef" || command.text == "ifndef")
    {
        auto* section = new CodeSection();

        auto* cond = new Conditional();
        cond->options.emplace_back();
        cond->options.back().section = section;
        cond->options.back().command = command;
        cond->options.back().arguments = arguments;
        cond->options.back().command.cond = cond;
        cond->currentSection = section;
        conditionalStack.push_back(cond);

        currentSection = section;
    }
    else if (command.text == "else" || command.text == "elif")
    {
        if (conditionalStack.empty())
        {
            cout << contextPath.u8string() << "(" << fromLine << "): error:  Unexpected #else\n";
            return false;
        }

        auto* section = new CodeSection();

        auto* cond = conditionalStack.back();
        cond->options.emplace_back();
        cond->options.back().section = section;
        cond->options.back().command = command;
        cond->options.back().arguments = arguments;
        cond->options.back().command.cond = cond;
        cond->currentSection = section;

        currentSection = section;
    }

    return true;
}

//--

bool CodeTokenizer::process()
{
    ScopeState state;
    return processCodeSection(rootSection, state);
}

struct TokenStream
{
    TokenStream(const vector<CodeTokenizer::CodeToken>& tokens)
        : tokens(tokens)
    {
        pos = 0;
        end = tokens.size();
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

bool CodeTokenizer::processCodeSection(CodeSection* section, ScopeState& state)
{
    TokenStream s(section->tokens);

    bool print = EndsWith(contextPath.u8string(), "file.c");

    while (s.hasContent())
    {
        const auto& token = s.peek();

        if (print)
            cout << "Token '" << token.text << "' at line " << token.line << "\n";

        if (token.text == "namespace")
        {
            const auto& ident = s.peek(1);
            const auto& bracket = s.peek(2);

            if (bracket.text == "{" && !ident.text.empty())
            {
                ScopeState::Scope scope;
                scope.entryToken = &bracket;
                scope.isNamespace = true;
                state.scopes.push_back(scope);

                state.namespaces.push_back(ident.text);
                s.eat(3);

                continue;
            }
            else if (bracket.text == ";")
            {
                // using namespace crap;
                s.eat(3);
            }
            else
            {
                //cout << "CRAP!\n";
                s.eat(1);
            }
        }
        else if (token.text == "{")
        {
            ScopeState::Scope scope;
            scope.entryToken = &token;
            scope.isNamespace = false;
            state.scopes.push_back(scope);
            s.eat();
        }
        else if (token.text == "}")
        {
            if (state.scopes.empty())
            {
                cout << contextPath.u8string() << "(" << token.line << "): error: Unexpected }\n";
                return false;
            }

            if (state.scopes.back().isNamespace)
            {
                if (state.namespaces.empty())
                {
                    cout << contextPath.u8string() << "(" << token.line << "): error: Unexpected } ends a namespace that was not properly recognized\n";
                    return false;
                }
                else
                {
                    state.namespaces.pop_back();
                }
            }

            state.scopes.pop_back();
            s.eat();
        }
        else
        {
            s.eat();
        }
    }

    return true;
}