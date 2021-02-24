/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/reflection/include/reflectionClassBuilder.h"
#include "base/reflection/include/reflectionMacros.h"
#include "base/containers/include/stringBuf.h"
#include "base/containers/include/array.h"
#include "base/containers/include/stringBuf.h"
#include "base/system/include/commandline.h"
#include "base/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE(base::app)

/// Command line parameter
struct BASE_APP_API CommandLineParam
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(CommandLineParam);

public:
    StringBuf name;
    StringBuf value;

    INLINE bool operator==(const CommandLineParam& other) const
    {
        return (name == other.name) && (value == other.value);
    }

    INLINE bool operator!=(const CommandLineParam& other) const
    {
        return !operator==(other);
    }
};

/// Command line arguments holder
/// NOTE: we have very specific arguments format for all of the tools:
/// First words can be without the trailing -, they are assumed to be ORDERED commands
/// We can have flags: -foo -bar
/// We can have values: -file=XXX -bar="X:\path"
/// We do not support "stray" arguments
/// Command line parameters are not enumerable - you can only ask if we have a specific one and what's the value(s)
/// Example: XXX.exe compile -file=X:\data\x.fx -outdir="X:\out dir\"
/// Example: XXX.exe help compile
/// Example: XXX.exe -dump=X:\crap.txt
/// NOTE: commandline is unicode (so we can have proper paths specified)
class BASE_APP_API CommandLine final : public IBaseCommandLine
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(CommandLine);

public:
    CommandLine();
    CommandLine(const CommandLine& other);
    CommandLine(CommandLine&& other);
    CommandLine& operator=(const CommandLine& other);
    CommandLine& operator=(CommandLine&& other);

    //--

    //! check if the command line is empty
    INLINE bool empty() const { return m_commands.empty() && m_params.empty(); }

    //! get all of the command words
    INLINE const Array<StringBuf>& commands() const { return m_commands; }

    //! get the single command word, may be empty, returns the first word from the command list
    INLINE const StringBuf& command() const { return m_commands.empty() ? StringBuf::EMPTY() : m_commands[0]; }

    //! get all parameters
    INLINE const Array<CommandLineParam>& params() const { return m_params; }

    //--

    //! parse the commandline from specified text (UTF8) that conforms to the commandline formatting, returns false if the commandline is invalid
    bool parse(StringView txt, bool skipProgramPath);

    //! parse the commandline from specified text (UTF16) that conforms to the commandline formatting, returns false if the commandline is invalid
    bool parse(const BaseStringView<wchar_t>& txt, bool skipProgramPath);

    //! parse the command line from a "classic" C argc/argv combo
    bool parse(int argc, wchar_t **argv);

    //! parse the command line from a "classic" C argc/argv combo
    bool parse(int argc, char **argv);

    //--

    //! set value of a parameter
    void param(StringView name, StringView value);

    //! remove parameter
    void removeParam(StringView name);

    //-----

    //! check if params is defined
    bool hasParam(StringView param) const;

    //! get single value (last defined value) of a parameter
    const StringBuf& singleValue(StringView param) const;

    //! get single value (last defined value) of parameter and parse it as an integer
    int singleValueInt(StringView param, int defaultValue = 0) const;

    //! get single value (last defined value) of parameter and parse it as a boolean
    bool singleValueBool(StringView param, bool defaultValue = false) const;

    //! get single value (last defined value) of parameter and parse it as a floating point value
    float singleValueFloat(StringView param, float defaultValue = 0.0f) const;

    //--

    //! add command word
    void addCommand(StringView command);

    //! remove command word
    void removeCommand(StringView command);

    //! do we have command word ?
    bool hasCommand(StringView command) const;

    //---

    //! gets all values for given parameter
    Array<StringBuf> allValues(StringView param) const;

    //---

    //! parse commandline from given string, after first error parsing stops but something is returned
    //! NOTE: this function does not guarantee safety in case of mallformated commandline
    static CommandLine Parse(const StringBuf& buf);

    //--

    //! convert commandline to an UTF8 string that can be passed forward
    StringBuf toString() const;

    //! convert commandline to an UTF16 string that can be passed forward
    UTF16StringVector toUTF16String() const;

    //! unpack as a list of arguments
    Array<UTF16StringVector> toUTF16StringArray() const;

    //--

    // IBaseCommandLine
    virtual bool hasParam(const char* param) const override final;
    virtual const char* singleValueAnsiStr(const char* param) const override final;
    virtual int singleValueInt(const char* param, int defaultValue = 0) const override final;
    virtual bool singleValueBool(const char* param, bool defaultValue = false) const override final;
    virtual float singleValueFloat(const char* param, float defaultValue = 0.0f) const override final;

private:
    Array<StringBuf> m_commands; // command words (non-params) ordered as in the original commandline
    Array<CommandLineParam> m_params; // key-value parameters

    // script interface wrappers (Scripts don't see StringView<> yet)
    void paramStr(const StringBuf& name, const StringBuf& value);
    void removeParamStr(const StringBuf& name);
    bool hasParamStr(const StringBuf& name) const;

    StringBuf singleValueStr(const StringBuf& name) const;
    int singleValueIntStr(const StringBuf& name, int defaultValue) const;
    bool singleValueBoolStr(const StringBuf& name, bool defaultValue) const;
    float singleValueFloatStr(const StringBuf& name, float defaultValue) const;

    Array<StringBuf> allValuesStr(const StringBuf& param) const;
};

//---

/// Unpack commandline into classic argc/argv pairs
class BASE_APP_API CommandLineUnpackedAnsi : public NoCopy
{
public:
    int m_argc;
    char** m_argv;

    CommandLineUnpackedAnsi(const CommandLine& cmdLine);
    ~CommandLineUnpackedAnsi();

private:
    mem::LinearAllocator m_mem;
};

//---

END_BOOMER_NAMESPACE()