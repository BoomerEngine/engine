#include "common.h"
#include "project.h"
#include "toolMake.h"
#include "toolReflection.h"

#ifdef _WIN32
    #include <Windows.h>
    #include <conio.h>
#endif

static std::string MergeCommandline(int argc, char** argv)
{
    std::string ret;

    for (int i = 1; i < argc; ++i) {
        if (!ret.empty())
            ret += " ";

        ret += argv[i];
    }

    return ret;
}

bool RunTool(const Configuration& config)
{
    if (config.tool == ToolType::SolutionGenerator)
    {
        ToolMake tool;
        return tool.run(config);
    }
    else if (config.tool == ToolType::ReflectionGenerator)
    {
        ToolReflection tool;
        return tool.run(config);
    }    

    return false;
}

#ifdef _WIN32
static void ClearConsole()
{
    COORD topLeft = { 0, 0 };
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;

    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
    FillConsoleOutputAttribute(console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE, screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
    SetConsoleCursorPosition(console, topLeft);
}
#else
static void ClearConsole()
{
    std::std::cout << "\x1B[2J\x1B[H";
}
#endif

template< typename T >
bool ConfigEnum(T& option, const char* title)
{
    ClearConsole();

    int maxOption = (int)T::MAX;

    std::cout << title << "\n";
    std::cout << "\n";
    std::cout << "Options:\n";

    for (int i = 0; i < maxOption; ++i)
    {
        std::cout << "  " << (i+1) << "): ";
        std::cout << NameEnumOption((T)i);

        if (option == (T)i)
            std::cout << " (current)";
        std::cout << "\n";
    }
    std::cout << "\n";
    std::cout << "\n";

    std::cout << "Press (1-" << maxOption << ") to select option\n";
    std::cout << "Press (ENTER) to use current option (" << NameEnumOption(option) << ")\n";
    std::cout << "Press (ESC) to exit\n";

    for (;;)
    {
        auto ch = _getch();
        if (ch == 13)
            return true;
        if (ch == 27)
            return false;

        if (ch >= '1' && ch < ('1' + maxOption))
        {
            option = (T)(ch - '1');
            return true;
        }
    }
}

static void PrintConfig(const Configuration& cfg)
{
    std::cout << "  Platform  : " << NameEnumOption(cfg.platform) << "\n";
    std::cout << "  Generator : " << NameEnumOption(cfg.generator) << "\n";
    std::cout << "  Build     : " << NameEnumOption(cfg.build) << "\n";
    std::cout << "  Libraries : " << NameEnumOption(cfg.libs) << "\n";
    std::cout << "  Config    : " << NameEnumOption(cfg.configuration) << "\n";
}

bool RunInteractiveConfig(Configuration& cfg)
{
    const auto configPath = cfg.builderEnvPath / ".buildConfig";

    if (cfg.load(configPath))
    {
        std::cout << "\n";
        std::cout << "Loaded existing configuration:\n";
        PrintConfig(cfg);
        std::cout << "\n";
        std::cout << "\n";
        std::cout << "Press (ENTER) to use\n";
        std::cout << "Press (ESC) to edit\n";

        for (;;)
        {
            auto ch = _getch();
            if (ch == 13)
                return true;
            if (ch == 27)
                break;
        }
    }

    ClearConsole();
    if (!ConfigEnum(cfg.platform, "Select build platform:"))
        return false;

    if (cfg.platform == PlatformType::Windows || cfg.platform == PlatformType::UWP)
    {
        ClearConsole();
        if (!ConfigEnum(cfg.generator, "Select solution generator:"))
            return false;
    }
    else 
    {
        cfg.generator = GeneratorType::CMake;
    }

    ClearConsole();
    if (!ConfigEnum(cfg.build, "Select build type:"))
        return false;

    ClearConsole();
    if (!ConfigEnum(cfg.libs, "Select libraries type:"))
        return false;

    ClearConsole();
    if (!ConfigEnum(cfg.configuration, "Select configuration type:"))
        return false;

    ClearConsole();

    if (!cfg.save(configPath))
        std::cout << "Failed to save configuration!\n";

    std::cout << "Running with config:\n";
    PrintConfig(cfg);
    
    return true;
}

int main(int argc, char** argv)
{
    std::cout << "BoomerEngine BuildTool v1.0\n";

    Commandline cmdLine;
    if (!cmdLine.parse(MergeCommandline(argc, argv))) {
        std::cout << "Invalid command line std::string\n";
        return -1;
    }

    Configuration config;
    if (!config.parseOptions(argv[0], cmdLine)) {
        std::cout << "Invalid/incomplete configuration\n";
        return -1;
    }

    if (cmdLine.has("interactive"))
        if (!RunInteractiveConfig(config))
            return false;

    if (!config.parsePaths(argv[0], cmdLine)) {
        std::cout << "Invalid/incomplete configuration\n";
        return -1;
    }

    if (!RunTool(config))
    {
        std::cout << "There were errors running selected tool\n";
        return -1;
    }

    return 0;
}
