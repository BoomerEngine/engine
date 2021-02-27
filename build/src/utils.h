#pragma once

//--

class Parser
{
public:
    Parser(std::string_view txt);

    //--

    inline std::string_view fullView() const { return std::string_view(m_start, m_end - m_start); }
    inline std::string_view currentView() const { return std::string_view(m_cur, m_end - m_cur); }
    inline uint32_t line() const { return m_line; }

    //--

    bool testKeyword(std::string_view keyword) const;

    void push();
    void pop();

    bool parseWhitespaces();
    bool parseTillTheEndOfTheLine(std::string_view* outIdent = nullptr);
    bool parseIdentifier(std::string_view& outIdent);
    bool parseString(std::string_view& outValue, const char* additionalDelims = "");
    bool parseLine(std::string_view& outValue, const char* additionalDelims = "", bool eatLeadingWhitespaces = true);
    bool parseKeyword(std::string_view keyword);
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

    std::vector<State> m_stateStack;
};

//--

class Commandline
{
public:
    struct Arg
    {
        std::string key;
        std::string value; // last
        std::vector<std::string> values; // all
    };

    std::vector<Arg> args;
    std::vector<std::string> commands;

    const std::string& get(std::string_view name) const;
    const std::vector<std::string>& getAll(std::string_view name) const;

    bool has(std::string_view name) const;

    bool parse(std::string_view text);
};

//--

extern bool LoadFileToString(const fs::path& path, std::string& outText);

extern bool SaveFileFromString(const fs::path& path, std::string_view txt, bool force = false, uint32_t* outCounter=nullptr, fs::file_time_type customTime = fs::file_time_type());

//--

extern bool EndsWith(std::string_view txt, std::string_view end);

extern bool BeginsWith(std::string_view txt, std::string_view end);

extern std::string_view PartBefore(std::string_view txt, std::string_view end);

extern std::string_view PartAfter(std::string_view txt, std::string_view end);

extern std::string MakeGenericPathEx(const fs::path& path);

extern std::string MakeGenericPath(std::string_view txt);

extern std::string ToUpper(std::string_view txt);

extern void writeln(std::stringstream& s, std::string_view txt);

extern void writelnf(std::stringstream& s, const char* txt, ...);

extern void SplitString(std::string_view txt, std::string_view delim, std::vector<std::string_view>& outParts);

extern std::string GuidFromText(std::string_view txt);

extern bool IsFileSourceNewer(const fs::path& source, const fs::path& target);

extern bool CopyNewerFile(const fs::path& source, const fs::path& target);

//--

extern std::string_view NameEnumOption(ConfigurationType type);
extern std::string_view NameEnumOption(BuildType type);
extern std::string_view NameEnumOption(LibraryType type);
extern std::string_view NameEnumOption(PlatformType type);
extern std::string_view NameEnumOption(GeneratorType type);

extern bool ParseConfigurationType(std::string_view txt, ConfigurationType& outType);
extern bool ParseBuildType(std::string_view txt, BuildType& outType);
extern bool ParseLibraryType(std::string_view txt, LibraryType& outType);
extern bool ParsePlatformType(std::string_view txt, PlatformType& outType);
extern bool ParseGeneratorType(std::string_view txt, GeneratorType& outType);

//--
   