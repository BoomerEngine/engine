/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: parser #]
***/

#include "build.h"
#include "textSimpleLanguageDefinition.h"
#include "textParser.h"
#include "textFilePreprocessor.h"

//#define TRACE_DEEP(txt, ...) TRACE_INFO(txt, __VA_ARGS__)
#define TRACE_DEEP(txt, ...) 

BEGIN_BOOMER_NAMESPACE_EX(parser)

//---

static UniquePtr<ILanguageDefinition> BuildPreprocessorLanguageDefinition()
{
    SimpleLanguageDefinitionBuilder langBuilder;
    langBuilder.addChar('#');
    langBuilder.addChar('(');
    langBuilder.addChar(')');
    langBuilder.addChar(',');
    langBuilder.addKeyword("##", 1000);
    langBuilder.addPreprocessor("#define");
    langBuilder.addPreprocessor("#ifdef");
    langBuilder.addPreprocessor("#ifndef");
    langBuilder.addPreprocessor("#if");
    langBuilder.addPreprocessor("#else");
    langBuilder.addPreprocessor("#elif");
    langBuilder.addPreprocessor("#endif");
    langBuilder.addPreprocessor("#undef");
    langBuilder.addPreprocessor("#include");
    langBuilder.addPreprocessor("#pragma");
    langBuilder.enableStrings(true);
    langBuilder.identChars("", "");

    return langBuilder.buildLanguageDefinition();
}

//---

TextFilePreprocessor::TextFilePreprocessor(LinearAllocator& allocator, IIncludeHandler& includeHandler, IErrorReporter& errorHandler, ICommentEater& commentEater, const ILanguageDefinition& parentLanguage)
    : m_includeHandler(includeHandler)
    , m_errorHandler(errorHandler)
    , m_commentEater(commentEater)
    , m_parentLanguage(parentLanguage)
    , m_allocator(allocator)
{
    m_filterStatus = true;
}

TextFilePreprocessor::~TextFilePreprocessor()
{
    for (auto define  : m_defineList)
        define->~MacroDefinition();
}

bool TextFilePreprocessor::defineSymbol(StringView name, StringView value)
{
    if (name.empty())
    {
        m_errorHandler.reportWarning(Location(), "Invalid define name");
        return false;
    }

    StringBuf valueStr = StringBuf(value);

    TokenList replacements;
    if (!createTokens(nullptr, valueStr.view(), replacements))
    {
        m_errorHandler.reportWarning(Location(), TempString("Unable to parse macro value from '{}'", value));
        return false;
    }

    auto macro  = createDefine(name);
    macro->m_definedAt = Location();
    macro->m_defined = true;
    macro->m_arguments.clear();
    macro->m_replacement = std::move(replacements);
    macro->m_values.pushBack(valueStr);

    TRACE_DEEP("Defined '{}' = '{}'", name, macro->m_replacement);
    return true;
}

Token* TextFilePreprocessor::copyToken(const Token* source, const Token* baseLocataion)
{
    auto ret  = m_allocator.createNoCleanup<Token>(*source);

    if (!baseLocataion)
        ret->assignLocation(source->location());
    else
        ret->assignLocation(baseLocataion->location());

    return ret;
}

void TextFilePreprocessor::copyTokens(const TokenList& list, TokenList& outList, const Token* baseLocataion /*= nullptr*/, bool isMacroArgument)
{
    auto cur  = list.head();
    while (cur != nullptr)
    {
        if (auto copy  = copyToken(cur, baseLocataion))
        {
            copy->m_hackFromMacroArguments = isMacroArgument;
            outList.pushBack(copy);
        }
        cur = cur->next();
    }
}

bool TextFilePreprocessor::createTokens(const Token* baseLocataion, StringView text, TokenList& outList)
{
    auto ptr  = text.data();
    auto end  = ptr + text.length();

    uint32_t lineIndex = 0;
    while (ptr < end)
    {
        while (ptr < end)
        {
            auto oldPtr  = ptr;
            m_commentEater.eatWhiteSpaces(ptr, end, lineIndex);
            if (ptr == oldPtr)
                break;
        }

        if (ptr == end)
            break;

        auto token  = m_allocator.createNoCleanup<Token>();
        if (!m_parentLanguage.eatToken(ptr, end, *token))
            return false;

        if (baseLocataion != nullptr)
            token->assignLocation(baseLocataion->location());

        outList.pushBack(token);
    }

    return true;
}

bool TextFilePreprocessor::processFileContent(StringView content, StringView contextPath, TokenList& outTokenList)
{
    static auto lang  = BuildPreprocessorLanguageDefinition();

    TextParser parser(contextPath, m_errorHandler, m_commentEater);
    parser.reset(content);

    while (parser.findNextContent())
    {
        auto token  = parser.parseToken(m_allocator, *lang);
        if (!token)
            token = parser.parseToken(m_allocator, m_parentLanguage);

        if (token)
        {
            outTokenList.pushBack(token);
        }
        else
        {
            Location location(parser.contextName(), parser.line(), 0);

            auto lookAhead  = parser.view().leftPart(20).trim().beforeFirstOrFull(" ").beforeFirstOrFull("\n").beforeFirstOrFull("\r").beforeFirstOrFull("\t");

            if (outTokenList.empty())
                m_errorHandler.reportError(location, TempString("Unable to parse text at this location: '{}'", lookAhead));
            else
                m_errorHandler.reportError(location, TempString("Unable to parse text at this location: '{}', last valid token: '{}'", lookAhead, *outTokenList.tail()));

            return false;
        }
    }

    return !parser.hasErrors();
}

static bool HasArgumentList(const Token* identToken, const Token* nextToken)
{
    if (!nextToken || !nextToken->isChar() && nextToken->ch() != '(')
        return false;

    auto basePos  = identToken->location().charPos();
    auto expectedEnd  = basePos + identToken->view().length();
    auto nextCharPos  = nextToken->location().charPos();
    return expectedEnd == nextCharPos;
}

bool TextFilePreprocessor::extractArgumentList(TokenList& line, Array<StringBuf>& outArguments) const
{
    if (line.empty())
        return true;

    auto start  = line.popFront();
    if (start->ch() != '(')
    {
        m_errorHandler.reportError(line.head()->location(), "Expected '('");
        return false;
    }

    bool eatSeparator = false;

    while (line.head())
    {
        auto cur  = line.popFront();
        if (cur->ch() == ')')
        {
            TRACE_DEEP("{}: found {} arguments", start->location(), outArguments.size());
            return true;
        }

        if (eatSeparator)
        {
            if (cur->ch() == ',')
            {
                eatSeparator = false;
                continue;
            }
            else
            {
                m_errorHandler.reportError(cur->location(), "macro parameters must be comma-separated");
                return false;
            }
        }

        if (!cur->isIdentifier())
        {
            m_errorHandler.reportError(cur->location(), "parameter name missing");
            return false;
        }

        TRACE_DEEP("{}: macro has argument '{}'", start->location(), cur->string());
        outArguments.pushBack(cur->string());
        eatSeparator = true;
    }

    m_errorHandler.reportError(start->location(), "missing ')' in macro parameter list");
    return false;
}

bool TextFilePreprocessor::processDefine(const Token* head, TokenList& line)
{
    ASSERT(m_filterStatus);

    auto identToken = line.popFront();
    if (!identToken || !identToken->isIdentifier())
    { 
        m_errorHandler.reportError(head->location(), "Expected macro name after the #define");
        return false;
    }

    auto ident = identToken->string();
    auto macro  = createDefine(ident);
    if (!macro)
    {
        m_errorHandler.reportError(head->location(), TempString("Unable to create definition of macro '{}'", ident));
        return false;
    }

    if (!macro->m_definedAt.contextName().empty())
    {
        m_errorHandler.reportWarning(head->location(), TempString("Macro '{}' is already defined at {}", ident, macro->m_definedAt));
    }

    macro->m_definedAt = head->location();
    macro->m_defined = true;
    macro->m_arguments.clear();
    macro->m_replacement.clear();
    macro->m_hasArguments = false;

    if (!line.empty())
    {
        // do we have argument list ?
        if (HasArgumentList(identToken, line.head()))
        {
            TRACE_DEEP("{}: macro '{}' has argument list", identToken->location(), ident);
            macro->m_hasArguments = true;

            if (!extractArgumentList(line, macro->m_arguments))
                return false;
        }
        else
        {
            TRACE_DEEP("{}: macro '{}' has no argument list", identToken->location(), ident);
        }

        // replacements
        macro->m_replacement = std::move(line);
    }

    return true;
}

bool TextFilePreprocessor::processUndef(const Token* head, TokenList& line)
{
    ASSERT(m_filterStatus);

    auto identToken = line.popFront();
    if (!identToken || !identToken->isIdentifier())
    {
        m_errorHandler.reportError(head->location(), "Expected macro name after the #undef");
        return false;
    }

    auto ident = identToken->string();
    auto macro  = define(ident);
    if (!macro)
    {
        m_errorHandler.reportWarning(head->location(), TempString("Macro '{}' is not defined", ident));
        return true; // continue
    }

    macro->m_definedAt = Location();
    macro->m_defined = false;
    macro->m_arguments.clear();
    macro->m_replacement.clear();
    macro->m_hasArguments = false;
    return true;
}

bool TextFilePreprocessor::processInclude(const Token* head, TokenList& line)
{
    ASSERT(m_filterStatus);

    auto path = line.popFront();

    bool includeGlobal = false;
    StringBuf includePath;
    if (path && path->isString())
    {
        includePath = path->string();

        if (!line.empty())
            m_errorHandler.reportWarning(head->location(), "Garbage at the end of #include directive");
    }
    else if (path && path->isChar() && path->ch() == '<')
    {
        StringBuilder pathStr;
        while (auto token = line.popFront())
        {
            if (token->isChar() && token->ch() == '>')
                break;
            pathStr.append(token->view());
        }

        if (!line.empty())
            m_errorHandler.reportWarning(head->location(), "Garbage at the end of #include directive");

        includePath = pathStr.toString();
        includeGlobal = true;
    }
    else
    {
        m_errorHandler.reportError(head->location(), "Expected include path name");
        return false;
    }

    Buffer loadedContent;
    StringBuf loadedContentContextPath;
    if (m_includeHandler.loadInclude(includeGlobal, includePath, head->location().contextName(), loadedContent, loadedContentContextPath))
    {
        if (m_currentIncludeStack.size() > 32)
        {
            m_errorHandler.reportError(head->location(), TempString("To many #include recursion levels when trying to include '{}'", includePath));
            return false;
        }

        m_currentIncludeStack.pushBack(loadedContentContextPath);

        auto contextName  = m_allocator.strcpy(loadedContentContextPath.c_str());

        StringView includedContent;
        if (loadedContent)
        {
            m_loadedIncludeBuffers.pushBack(loadedContent);
            includedContent = StringView((const char*)loadedContent.data(), (const char*)loadedContent.data() + loadedContent.size());
        }

        if (!processContent(includedContent, loadedContentContextPath.view()))
        {
            m_currentIncludeStack.popBack();
            return false;
        }

        m_currentIncludeStack.popBack();
    }
    else
    {
        m_errorHandler.reportError(head->location(), TempString("Unable to resolve include '{}'", includePath));
        return false;
    }

    return true;
}

bool TextFilePreprocessor::processIf(const Token* head, TokenList& line)
{
    ASSERT(m_filterStatus);

    TokenList expandedList;
    if (!expandAllPossibleMacros(line, expandedList))
        return false;

    if (expandedList.empty())
    {
        m_errorHandler.reportError(head->location(), "#if with no expression");
        return false;
    }

    bool result = false;
    if (!evaluateExpression(expandedList, result))
        return false;

    auto& info = m_filterStack.emplaceBack();
    info.m_token = head;
    info.m_enabled = result;
    info.m_anyTaken = result;
    m_filterStatus = result;
    return true;
}

bool TextFilePreprocessor::processIfdef(const Token* head, TokenList& line, bool swap)
{
    ASSERT(m_filterStatus);

    auto identToken = line.popFront();
    if (!identToken || !identToken->isIdentifier())
    {
        m_errorHandler.reportError(head->location(), "Expected macro name after the #ifdef");
        return false;
    }

    auto ident = identToken->string();
    auto macro  = define(ident);
    auto valid = macro && macro->m_defined;
    if (valid == swap)
    {
        auto& info = m_filterStack.emplaceBack();
        info.m_token = head;
        info.m_enabled = false;
        m_filterStatus = false;
    }
    else
    {
        auto& info = m_filterStack.emplaceBack();
        info.m_token = head;
        info.m_enabled = true;
        info.m_anyTaken = true;
    }

    return true;
}

bool TextFilePreprocessor::processElif(const Token* head, TokenList& line)
{
    if (m_filterStack.empty())
    {
        m_errorHandler.reportError(head->location(), "#elif without #if");
        return false;
    }

    auto& filter = m_filterStack.back();
    if (filter.m_hadElse)
    {
        m_errorHandler.reportError(head->location(), "#elif after #else");
        return false;
    }

    TokenList expandedList;
    if (!expandAllPossibleMacros(line, expandedList))
        return false;

    if (expandedList.empty())
    {
        m_errorHandler.reportError(head->location(), "#if with no expression");
        return false;
    }

    bool result = false;
    if (!evaluateExpression(expandedList, result))
        return false;

    filter.m_token = head;
    filter.m_enabled = result;
    filter.m_anyTaken |= result;
    m_filterStatus = result;
    return true;
}

bool TextFilePreprocessor::processElse(const Token* head, TokenList& line)
{
    if (m_filterStack.empty())
    {
        m_errorHandler.reportError(head->location(), "#else without #if");
        return false;
    }

    auto& filter = m_filterStack.back();
    if (filter.m_hadElse)
    {
        m_errorHandler.reportError(head->location(), "#else after #else");
        return false;
    }

    filter.m_hadElse = true;

    filter.m_token = head;
    filter.m_enabled = !filter.m_anyTaken;
    filter.m_anyTaken = true;
    m_filterStatus = filter.m_enabled;
    return true;
}

bool TextFilePreprocessor::processEndif(const Token* head, TokenList& line)
{
    if (m_filterStack.empty())
    {
        m_errorHandler.reportError(head->location(), "#endif without #if");
        return false;
    }

    m_filterStack.popBack();
    m_filterStatus = true;
    return true;
}

bool TextFilePreprocessor::processPragma(const Token* head, TokenList& line, bool& keepProcessing)
{
    if (line.empty())
    {
        m_errorHandler.reportError(head->location(), "empty #pragma directive");
        return false;
    }

    auto pragmaType = line.popFront()->view();
    if (pragmaType == "once")
    {
        if (!m_currentIncludeStack.empty())
        {
            auto includePath = m_currentIncludeStack.back();
            if (!m_pragmaOnceFilePaths.insert(includePath))
            {
                TRACE_DEEP("{}: detected that file was already processed", head->location());
                keepProcessing = false;
            }
        }
    }
    else if (pragmaType == "line")
    {

    }
    else
    {
        // TODO: emit back to source code
    }

    return true;
} 

bool TextFilePreprocessor::processPreprocessorDeclaration(TokenList& line, bool& keepProcessing)
{
    ASSERT(!line.empty());

    auto type  = line.popFront();
    ASSERT(type->isPreprocessor());

    if (!m_filterStatus && (type->view() != "#endif" && type->view() != "#else" && type->view() != "#elif"))
        return true;

    if (type->view() == "#define")
        return processDefine(type, line);
    else if (type->view() == "#include")
        return processInclude(type, line);
    else if (type->view() == "#undef")
        return processUndef(type, line);
    else if (type->view() == "#if")
        return processIf(type, line);
    else if (type->view() == "#ifdef")
        return processIfdef(type, line);
    else if (type->view() == "#ifndef")
        return processIfdef(type, line, true);
    else if (type->view() == "#else")
        return processElse(type, line);
    else if (type->view() == "#elif")
        return processElif(type, line);
    else if (type->view() == "#endif")
        return processEndif(type, line);
    else if (type->view() == "#pragma")
        return processPragma(type, line, keepProcessing);
    else
    {
        m_errorHandler.reportError(type->location(), TempString("Unknown preprocessor directive '{}'", type->view()));
        return false;
    }
}

bool TextFilePreprocessor::processContent(StringView content, StringView contextPath)
{
    bool status = true;

    TokenList tokens;
    if (!processFileContent(content, contextPath, tokens))
        return false;

    bool keepProcessing = true;
    while (tokens.head() && keepProcessing)
    {
        if (tokens.head()->isPreprocessor())
        {
            TokenList line;
            tokens.unlinkLine(tokens.head(), line);
            TRACE_DEEP("Found preprocessor line: '{}'", line);

            status &= processPreprocessorDeclaration(line, keepProcessing);
        }
        else if (m_filterStatus)
        {
            MacroPlacement macro;
            if (shouldExpandMacro(tokens, macro))
            {
                status &= expandMacro(macro, m_finalTokens);
            }
            else
                m_finalTokens.pushBack(tokens.popFront());
        }
        else
        {
            tokens.popFront();
        }
    }

    /*{
        const auto* token = m_finalTokens.head();
        while (token)
        {
            TRACE_DEEP("Token '{}'", *token);
            token = token->next();
        }
    }*/

    if (keepProcessing && m_currentIncludeStack.empty())
    {
        while (!m_filterStack.empty())
        {
            auto& lastFilter = m_filterStack.back();
            m_errorHandler.reportError(lastFilter.m_token->location(), "Missing #endif");
            m_filterStack.popBack();
            status = false;
        }
    }

    return status;
}

//---

TextFilePreprocessor::MacroDefinition* TextFilePreprocessor::define(StringView name) const
{
    MacroDefinition* ret = nullptr;
    m_defineMap.find(name, ret);
    return ret;
}

TextFilePreprocessor::MacroDefinition* TextFilePreprocessor::createDefine(StringView name)
{
    MacroDefinition* ret = nullptr;
    if (!m_defineMap.find(name, ret))
    {
        ret = m_allocator.createNoCleanup<MacroDefinition>();
        ret->m_name = StringBuf(name);
        m_defineMap[ret->m_name] = ret;
        m_defineList.pushBack(ret);
    }
    return ret;
}

bool TextFilePreprocessor::evaluateExpression(TokenList& list, bool& result)
{
    if (list.empty())
        return false;

    auto cur = list.head();
    if (!cur->isInteger())
    {
        m_errorHandler.reportError(cur->location(), TempString("preprocessor expression can only work on integers"));
        return false;
    }

    int value = 0;
    cur->view().match(value);

    result = value != 0;;
    return true;
}

bool TextFilePreprocessor::extractReplacementListForArgument(const Token* start, TokenList& list, TokenList& outputList, bool& outEnd)
{
    int block = 0;

    outputList.clear();

    while (list.head())
    {
        if (list.head()->location().line() != start->location().line())
            break;

        if (list.head()->ch() == ',')
        {
            if (block == 0)
            {
                TRACE_DEEP("Extracted replacement '{}'", outputList);
                outEnd = false;
                list.popFront();
                return true;
            }
        }
        else if (list.head()->ch() == ')')
        {
            if (block == 0)
            {
                TRACE_DEEP("Extracted final replacement '{}'", outputList);
                outEnd = true;
                list.popFront();
                return true;
            }
            else
            {
                block -= 1;
                outputList.pushBack(list.popFront());
                continue;
            }
        }
        else if (list.head()->ch() == '(')
        {
            block += 1;
            outputList.pushBack(list.popFront());
            continue;
        }

        outputList.pushBack(list.popFront());
    }

    m_errorHandler.reportError(start->location(), TempString("unterminated argument list invoking macro '{}'", start->view()));
    return false;
}

bool TextFilePreprocessor::shouldExpandMacro(TokenList& list, MacroPlacement& macroList, bool fromParamsOnly)
{
    if (!list.head() || !list.head()->isIdentifier())
        return false;

    auto identToken = list.head();
    auto ident = identToken->view();
    auto macro  = define(ident);
    if (!macro || !macro->m_defined)
        return false; // we have no matching macro

    TRACE_DEEP("{}: found potential macro '{}' for expansion", identToken->location(), macro->m_name);

    if (fromParamsOnly && !identToken->m_hackFromMacroArguments)
        return false; // it's not our time

    // function-like macros must have arguments provided
    if (macro->m_hasArguments && (!list.head()->next() || list.head()->next()->ch() != '('))
        return false;

    // yay, we will have valid token
    macroList.m_def = macro;
    macroList.m_placementRef = list.head();
    macroList.m_arguments.clear();

    // remove the ID
    list.popFront();

    // if we have arguments, extract them
    if (macro->m_hasArguments)
    {
        auto start  = list.popFront();
        if (!start || start->ch() != '(')
            return false;

        TokenList argumentTokens;
        bool lastArgument = false;
        while (extractReplacementListForArgument(identToken, list, argumentTokens, lastArgument))
        {
            // do not output empty list if we are already full
            if (lastArgument && argumentTokens.empty() && macroList.m_arguments.size() == macro->m_arguments.size())
                break;

            // emit as argument name
            auto& entry = macroList.m_arguments.emplaceBack();
            entry.m_tokens = std::move(argumentTokens);

            if (macroList.m_arguments.size() <= macro->m_arguments.size())
                entry.m_name = macro->m_arguments[macroList.m_arguments.lastValidIndex()];

            if (lastArgument)
                break;
        }

        if (macroList.m_arguments.size() > macro->m_arguments.size())
            m_errorHandler.reportError(macroList.m_placementRef->location(), TempString("macro '{}' passed {} arguments, but takes just {}", macro->m_name, macroList.m_arguments.size(), macro->m_arguments.size()));
        else if (macroList.m_arguments.size() < macro->m_arguments.size())
            m_errorHandler.reportError(macroList.m_placementRef->location(), TempString("macro '{}' requires {} arguments, but only {} given", macro->m_name, macroList.m_arguments.size(), macro->m_arguments.size()));

        TRACE_DEEP("{}: found macro '{}' to expand", identToken->location(), macro->m_name);
        for (auto& arg : macroList.m_arguments)
            TRACE_DEEP("{}: tokens for param '{}' = '{}'", identToken->location(), arg.m_name, arg.m_tokens);
    }

    return true;
}

bool TextFilePreprocessor::expandAllPossibleMacros(TokenList& list, TokenList& outputList, bool fromParamsOnly)
{
    bool status = true;

    while (list.head())
    {
        MacroPlacement macro;
        if (shouldExpandMacro(list, macro, fromParamsOnly))
            status &= expandMacro(macro, outputList);
        else
            outputList.pushBack(list.popFront());
    }

    return status;
}

bool TextFilePreprocessor::macroInitialExpand(const MacroPlacement& macro, TokenList& outputList)
{
    bool status = true;

    auto token  = macro.m_def->m_replacement.head();
    while (token != nullptr)
    {
        auto copy  = copyToken(token, macro.m_placementRef);
        outputList.pushBack(copy);

        token = token->next();
    }

    return status;
}

const TextFilePreprocessor::MacroArgument* TextFilePreprocessor::MacroPlacement::arg(const Token* token) const
{
    if (!token || !token->isIdentifier())
        return nullptr;

    for (auto& arg : m_arguments)
        if (arg.m_name == token->view())
            return &arg;

    return nullptr;
}

static void Stringify(const TokenList& list, IFormatStream& f)
{
    f << "\"";

    auto pos = list.head()->location().charPos();

    auto cur  = list.head();
    while (cur)
    {
        if (cur->location().charPos() > pos)
        {
            auto paddingCount = cur->location().charPos() - pos;
            f.appendPadding(' ', paddingCount);
        }

        f << cur->view();

        pos = cur->location().charPos() + cur->view().length();
        cur = cur->next();
    }

    f << "\"";
}

bool TextFilePreprocessor::macroSringify(const MacroPlacement& macro, TokenList& inputList, TokenList& outputList)
{
    bool status = true;

    while (inputList.head())
    {
        auto pop  = inputList.popFront();
        if (pop->ch() == '#')
        {
            auto identToken  = inputList.popFront();
            if (auto arg  = macro.arg(identToken))
            {
                TempString f;
                Stringify(arg->m_tokens, f);

                auto text  = m_allocator.strcpy(f.c_str());

                TRACE_DEEP("{}: stringified to '{}'", pop->location(), text);
                status &= createTokens(macro.m_placementRef, text, outputList);
            }
            else
            {
                m_errorHandler.reportError(macro.m_placementRef->location(), "Expected argument name after #");
            }
        }
        else
        {
            outputList.pushBack(pop);
        }
    }

    return status;
}

bool TextFilePreprocessor::macroReplaceParameters(const MacroPlacement& macro, TokenList& inputList, TokenList& outputList)
{
    bool status = true;

    while (inputList.head())
    {
        auto pop  = inputList.popFront();
        if (pop->isIdentifier())
        {
            if (auto arg  = macro.arg(pop))
                copyTokens(arg->m_tokens, outputList, macro.m_placementRef, true);
            else
                outputList.pushBack(pop);
        }
        else
        {
            outputList.pushBack(pop);
        }
    }

    return status;
}

bool TextFilePreprocessor::macroConcatenate(const MacroPlacement& macro, TokenList& inputList, TokenList& outputList)
{
    bool status = true;

    for (;;)
    {
        bool hasSomethingMerged = false;
        while (inputList.head())
        {
            auto merge  = inputList.head()->next();
            if (merge && merge->view() == "##" && merge->next())
            {
                auto first  = inputList.popFront();
                inputList.popFront();
                auto second  = inputList.popFront();

                TempString fullText;
                fullText << first->view();
                fullText << second->view();

                auto text  = m_allocator.strcpy(fullText.c_str());
                TRACE_DEEP("{}: merged to '{}'", merge->location(), text);

                status &= createTokens(macro.m_placementRef, text, outputList);
                hasSomethingMerged = true;
            }
            else
            {
                outputList.pushBack(inputList.popFront());
            }
        }

        if (hasSomethingMerged)
            inputList = std::move(outputList);
        else
            break;
    }

    if (outputList.head() && outputList.head()->view() == "##")
    {
        m_errorHandler.reportError(macro.m_placementRef->location(), "'##' cannot appear at either end of a macro expansion");
        status = false;
    }
    else if (outputList.tail() && outputList.tail()->view() == "##")
    {
        m_errorHandler.reportError(macro.m_placementRef->location(), "'##' cannot appear at either end of a macro expansion");
        status = false;
    }

    return status;
}

// https://en.wikipedia.org/wiki/C_preprocessor#Order_of_expansion
bool TextFilePreprocessor::expandMacro(const MacroPlacement& macro, TokenList& outputList)
{
    bool status = true;

    TRACE_DEEP("Expanding tokens for macro '{}'", macro.m_def->m_name);

    // 0) Instantiate the replacement tokens from macro as they are, we don't do any substitution yet
    TokenList list0;
    status &= macroInitialExpand(macro, list0);
    TRACE_DEEP("Phase0: '{}'", list0);

    // 1) Stringification operations are replaced with the textual representation of their argument's replacement list (without performing expansion).
    TokenList list1; 
    status &= macroSringify(macro, list0, list1);
    TRACE_DEEP("Phase1: '{}'", list1);

    // 2) Parameters are replaced with their replacement list (without performing expansion).
    TokenList list2;
    status &= macroReplaceParameters(macro, list1, list2);
    TRACE_DEEP("Phase2: '{}'", list2);
            
    // 3) Concatenation operations are replaced with the concatenated result of the two operands (without expanding the resulting token).
    TokenList list3;
    status &= macroConcatenate(macro, list2, list3);
    TRACE_DEEP("Phase3: '{}'", list3);

    // 4) Tokens originating from parameters are expanded.
    TokenList list4;
    status &= expandAllPossibleMacros(list3, list4, true);
    TRACE_DEEP("Phase4: '{}'", list4);

    // 5) The resulting tokens are expanded as normal.
    TokenList list5;
    status &= expandAllPossibleMacros(list4, list5);
    TRACE_DEEP("Phase5: '{}'", list5);

    // 6) Copy to final list
    while (!list5.empty())
        outputList.pushBack(list5.popFront());

    return status;
}

//---

END_BOOMER_NAMESPACE_EX(parser)