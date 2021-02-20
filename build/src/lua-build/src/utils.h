#pragma once

//--

class Parser
{
public:
    Parser(string_view txt);

    //--

    inline string_view fullView() const { return string_view(m_start, m_end - m_start); }
    inline string_view currentView() const { return string_view(m_cur, m_end - m_cur); }
    inline uint32_t line() const { return m_line; }

    //--

    bool testKeyword(string_view keyword) const;

    void push();
    void pop();

    bool parseWhitespaces();
    bool parseTillTheEndOfTheLine(string_view* outIdent = nullptr);
    bool parseIdentifier(string_view& outIdent);
    bool parseString(string_view& outValue, const char* additionalDelims = "");
    bool parseLine(string_view& outValue, const char* additionalDelims = "", bool eatLeadingWhitespaces = true);
    bool parseKeyword(string_view keyword);
    bool parseChar(uint32_t& outChar);
    bool parseFloat(float& outValue);
    bool parseDouble(double& outValue);
    bool parseBoolean(bool& outValue);
    bool parseHex(uint64_t& outValue, uint32_t maxLength = 0, uint32_t* outValueLength = nullptr);
    bool parseInt8(char& outValue);
    bool parseInt16(short& outValue);
    bool parseInt32(int& outValue);
    bool parseInt64(int64_t& outValue);
    bool parseUint8(uint8_t& outValue);
    bool parseUint16(uint16_t& outValue);
    bool parseUint32(uint32_t& outValue);
    bool parseUint64(uint64_t& outValue);

private:
    const char* m_start;
    const char* m_cur;
    const char* m_end;
    uint32_t m_line;

    struct State
    {
        const char* cur = nullptr;
        uint32_t line = 0;
    };

    vector<State> m_stateStack;
};

//--

class Commandline
{
public:
    struct Arg
    {
        string key;
        string value; // last
        vector<string> values; // all
    };

    vector<Arg> args;
    vector<string> commands;

    const string& get(string_view name) const;
    const vector<string>& getAll(string_view name) const;

    bool has(string_view name) const;

    bool parse(string_view text);
};

//--

extern bool LoadFileToString(const filesystem::path& path, string& outText);

extern bool SaveFileFromString(const filesystem::path& path, string_view txt, bool force = false, uint32_t* outCounter=nullptr);

//--

extern bool EndsWith(string_view txt, string_view end);

extern bool BeginsWith(string_view txt, string_view end);

extern string_view PartBefore(string_view txt, string_view end);

extern string MakeGenericPathEx(const filesystem::path& path);

extern string MakeGenericPath(string_view txt);

extern string ToUpper(string_view txt);

extern void writeln(stringstream& s, string_view txt);

extern void writelnf(stringstream& s, const char* txt, ...);

extern void SplitString(string_view txt, string_view delim, vector<string_view>& outParts);

extern string GuidFromText(string_view txt);

extern bool IsFileSourceNewer(const filesystem::path& source, const filesystem::path& target);

extern bool CopyNewerFile(const filesystem::path& source, const filesystem::path& target);

//--

extern string_view NameConfigurationType(ConfigurationType type);

extern string_view NameBuildType(BuildType type);

extern string_view NamePlatformType(PlatformType type);

extern string_view NameGeneratorType(GeneratorType type);

//--
   