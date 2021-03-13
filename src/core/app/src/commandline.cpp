/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"

#include "commandline.h"
#include "core/containers/include/stringParser.h"
#include "core/containers/include/stringBuilder.h"
#include "core/io/include/io.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_STRUCT(CommandLineParam);
    RTTI_BIND_NATIVE_COMPARE(CommandLineParam);
    RTTI_PROPERTY(name);
    RTTI_PROPERTY(value);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(CommandLine);
    RTTI_PROPERTY(m_commands);
    RTTI_PROPERTY(m_params);
    RTTI_FUNCTION("Empty", empty);
    RTTI_FUNCTION("SetParam", paramStr);
    RTTI_FUNCTION("RemoveParam", removeParamStr);
    RTTI_FUNCTION("HasParam", hasParamStr);
    RTTI_FUNCTION("GetSingleValue", singleValueStr);
    RTTI_FUNCTION("GetSingleValueInt", singleValueIntStr);
    RTTI_FUNCTION("GetSingleValueBool", singleValueBoolStr);
    RTTI_FUNCTION("GetSingleValueFloat", singleValueFloatStr);
    //RTTI_FUNCTION("getSingleValueUTF16", singleValueUTF16);
    RTTI_FUNCTION("GetAllValues", allValuesStr);
    RTTI_FUNCTION("ToString", toString);
    RTTI_STATIC_FUNCTION("Parse", Parse);
RTTI_END_TYPE();

//--

CommandLine::CommandLine() = default;
CommandLine::CommandLine(const CommandLine& other) = default;
CommandLine::CommandLine(CommandLine&& other) = default;
CommandLine& CommandLine::operator=(const CommandLine& other) = default;
CommandLine& CommandLine::operator=(CommandLine&& other) = default;

/*bool CommandLine::operator==(const CommandLine& other) const
{
    if (!(m_commands == other.m_commands))
        return false;
    if (m_params.size() != other.m_params.size())
        return false;

    for (auto& param : m_params)
    {
        bool matched = false;
        for (auto& otherParam : other.m_params)
            if (param.name == otherParam.name)
                if (param.value == otherParam.value)
                {
                    matched = true;
                    break;
                }

        if (!matched)
            return false;
    }

    return true;
}

bool CommandLine::operator!=(const CommandLine& other) const
{
    return !operator==(other);
}*/

//--

bool CommandLine::parse(StringView commandline, bool skipProgramPath)
{
    StringParser parser(commandline);

    // skip the program path
    if (skipProgramPath)
    {
        StringView programPath;
        if (!parser.parseString(programPath))
        {
            TRACE_WARNING("No program path in passed commandline");
            return false;
        }
    }

    // parse the ordered commands
    bool parseInitialChar = true;
    while (parser.parseWhitespaces())
    {
        // detect start of parameters
        if (parser.parseKeyword("-"))
        {
            parseInitialChar = false;
            break;
        }

        // get the command
        StringView commandName;
        if (!parser.parseIdentifier(commandName))
        {
            TRACE_ERROR("Commandline parsing error: expecting command name. Application may not work as expected.");
            return false;
        }

        m_commands.pushBack(StringBuf(commandName));
    }

    // parse the text
    while (parser.parseWhitespaces())
    {
        // Skip the initial '-'
        if (!parser.parseKeyword("-") && parseInitialChar)
            break;
        parseInitialChar = true;

        // Get the name of the parameter
        StringView paramName;
        if (!parser.parseIdentifier(paramName))
        {
            TRACE_ERROR("Commandline parsing error: expecting param name after '-'. Application may not work as expected.");
            return false;
        }

        // Do we have a value ?
        StringView paramValue;
        if (parser.parseKeyword("="))
        {
            // Read value
            if (!parser.parseString(paramValue))
            {
                TRACE_ERROR("Commandline parsing error: expecting param value after '=' for param '{}'. Application may not work as expected.", paramName);
                return false;
            }
        }

        // add parameter
        m_params.emplaceBack(CommandLineParam{ StringBuf(paramName), StringBuf(paramValue) });
    }

    // parsed
    return true;
}

bool CommandLine::parse(const BaseStringView<wchar_t>& commandline, bool skipProgramPath)
{
    StringBuf ansiCommandline(commandline);
    return parse(ansiCommandline.c_str(), skipProgramPath);
}

bool CommandLine::parse(int argc, wchar_t **argv)
{
    StringBuilder str;
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
            str.append(" ");

        str.append(argv[i]);
    }

    return parse(str.c_str(), false);
}

bool CommandLine::parse(int argc, char **argv)
{
    StringBuilder str;
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
            str.append(" ");

        str.append(argv[i]);
    }

    return parse(str.c_str(), false);
}

void CommandLine::addCommand(StringView command)
{
    if (!hasCommand(command))
        m_commands.pushBack(StringBuf(command));
}

void CommandLine::removeCommand(StringView command)
{
    for (uint32_t i=0; i<m_commands.size(); ++i)
    {
        if (m_commands[i].compareWithNoCase(command))
        {
            m_commands.erase(i);
        }
    }
}

bool CommandLine::hasCommand(StringView command) const
{
    for (auto& ptr : m_commands)
        if (ptr.compareWithNoCase(command))
            return true;

    return false;
}

void CommandLine::param(StringView name, StringView value)
{
    for (auto& param : m_params)
    {
        if (param.name == name)
        {
            param.value = StringBuf(value);
            return;
        }
    }

    m_params.emplaceBack(CommandLineParam{ StringBuf(name), StringBuf(value) });
}

void CommandLine::removeParam(StringView name)
{
    for (int i = (int)m_params.size() - 1; i >= 0; --i)
    {
        if (m_params[i].name == name)
        {
            m_params.erase(i);
        }
    }
}

bool CommandLine::hasParam(StringView paramName) const
{
    // Linear search
    for (auto& param : m_params)
        if (param.name == paramName)
            return true;

    // Param was not defined
    return false;
}

const StringBuf& CommandLine::singleValue(StringView paramName) const
{
    // Linear search
    for (auto& param : m_params)
        if (param.name == paramName)
            return param.value;

    // No value
    return StringBuf::EMPTY();
}

int CommandLine::singleValueInt(StringView paramName, int defaultValue /*= 0*/) const
{
    // Linear search
    for (auto& param : m_params)
    {
        if (param.name == paramName)
        {
            int ret = defaultValue;
            if (MatchResult::OK == param.value.view().match(ret))
                return ret;
        }
    }

    // No value
    return defaultValue;
}

float CommandLine::singleValueFloat(StringView paramName, float defaultValue /*= 0*/) const
{
    // Linear search
    for (auto& param : m_params)
    {
        if (param.name == paramName)
        {
            auto str  = param.value.c_str();

            float ret = defaultValue;
            if (MatchResult::OK == param.value.view().match(ret))
                return ret;
        }
    }

    // No value
    return defaultValue;
}

bool CommandLine::singleValueBool(StringView paramName, bool defaultValue /*= 0*/) const
{
    // Linear search
    for (auto& param : m_params)
    {
        if (param.name == paramName)
        {
            if (param.value == "true")
                return true;
            else if (param.value == "false")
                return false;

            int ret = defaultValue;
            if (MatchResult::OK == param.value.view().match(ret))
                return ret != 0;
        }
    }

    // No value
    return defaultValue;
}

Array< StringBuf > CommandLine::allValues(StringView paramName) const
{
    Array< StringBuf > ret;

    // Extract all
    for (auto& param : m_params)
        if (param.name == paramName)
            if (!param.value.empty())
                ret.pushBack(param.value);

    return ret;
}

//--

CommandLine CommandLine::Parse(const StringBuf& buf)
{
    CommandLine ret;
    ret.parse(buf.view(), false);
    return ret;
}

//--

static bool ShouldWrapPram(StringView str)
{
    for (auto ch : str)
    {
        if (ch == '\"' || ch == '\'' || ch <= 32 || ch == '=' || ch == '/' || ch == '\\')
            return true;
    }
    return false;
}

StringBuf CommandLine::toString() const
{
    StringBuilder ret;

    // commands
    for (auto& cmd : m_commands)
    {
        if (!ret.empty())
            ret << " ";
        ret << cmd;
    }

    // parameters
    for (auto& param : m_params)
    {
        if (!ret.empty())
            ret << " ";

        ret << "-" << param.name;

        if (!param.value.empty())
        {
            ret << "=";

            if (ShouldWrapPram(param.value))
                ret << "\"" << param.value << "\"";
            else
                ret << param.value;
        }
    }

    return ret.toString();
}

UTF16StringVector CommandLine::toUTF16String() const
{
    return UTF16StringVector(toString());
}

Array<UTF16StringVector> CommandLine::toUTF16StringArray() const
{
    Array<UTF16StringVector> ret;

    for (auto& cmd : m_commands)
        ret.emplaceBack(cmd.uni_str());

    for (auto& param : m_params)
    {
        auto& txt = ret.emplaceBack();
        txt += "-";
        txt += param.name;

        if (!param.value.empty())
        {
            txt += "=";

            if (ShouldWrapPram(param.value))
            {
                txt += "\"";
                txt += param.value;
                txt += "\"";
            }
            else
            {
                txt += param.value;
            }
        }
    }

    return ret;
}

//--

void CommandLine::paramStr(const StringBuf& name, const StringBuf& value)
{
    param(name, value);
}

void CommandLine::removeParamStr(const StringBuf& name)
{
    removeParam(name);
}

bool CommandLine::hasParamStr(const StringBuf& name) const
{
    return hasParam(name);
}

StringBuf CommandLine::singleValueStr(const StringBuf& name) const
{
    return singleValue(name);
}

int CommandLine::singleValueIntStr(const StringBuf& name, int defaultValue) const
{
    return singleValueInt(name, defaultValue);
}

bool CommandLine::singleValueBoolStr(const StringBuf& name, bool defaultValue) const
{
    return singleValueBool(name, defaultValue);
}

float CommandLine::singleValueFloatStr(const StringBuf& name, float defaultValue) const
{
    return singleValueFloat(name, defaultValue);
}

Array<StringBuf> CommandLine::allValuesStr(const StringBuf& param) const
{
    return allValues(param);
}

//--

bool CommandLine::hasParam(const char* param) const
{
    return hasParam(StringView(param));
}

const char* CommandLine::singleValueAnsiStr(const char *param) const
{
    return singleValue(StringView(param)).c_str();
}

int CommandLine::singleValueInt(const char* param, int defaultValue /*= 0*/) const
{
    return singleValueInt(StringView(param), defaultValue);
}

bool CommandLine::singleValueBool(const char* param, bool defaultValue /*= false*/) const
{
    return singleValueBool(StringView(param), defaultValue);
}

float CommandLine::singleValueFloat(const char* param, float defaultValue /*= 0.0f*/) const
{
    return singleValueFloat(StringView(param), defaultValue);
}

//--

CommandLineUnpackedAnsi::CommandLineUnpackedAnsi(const CommandLine& cmdLine)
    : m_mem(POOL_TEMP)
{
    // count arguments
    uint32_t numArgs = cmdLine.params().size() + cmdLine.commands().size() + 1;

    // get path to executable
    auto executablePath = SystemPath(PathCategory::ExecutableFile);
    if (!executablePath.empty())
        numArgs += 1;

    // allocate structures
    m_argc = numArgs - 1;
    m_argv = (char**) m_mem.alloc(sizeof(const char*) * numArgs, 1);
    memset(m_argv, 0, sizeof(const char*) * numArgs);

    // copy program path
    uint32_t paramIndex = 0;
    if (!executablePath.empty())
        m_argv[paramIndex++] = m_mem.strcpy(executablePath.c_str());

    // copy commands
    for (auto& command : cmdLine.commands())
        m_argv[paramIndex++] = m_mem.strcpy(command.c_str());

    // params
    for (auto& param : cmdLine.params())
    {
        if (param.value.empty())
            m_argv[paramIndex++] = m_mem.strcpy(TempString("-{}", param.name).c_str());
        else
            m_argv[paramIndex++] = m_mem.strcpy(TempString("-{}={}", param.name, param.value).c_str());
    }
}

CommandLineUnpackedAnsi::~CommandLineUnpackedAnsi()
{}

//--

END_BOOMER_NAMESPACE()
