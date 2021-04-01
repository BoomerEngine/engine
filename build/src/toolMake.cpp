#include "common.h"
#include "toolMake.h"
#include "toolReflection.h"
#include "projectGenerator.h"
#include "solutionGeneratorVS.h"
#include "solutionGeneratorCMAKE.h"

#ifdef _WIN32
#include <Windows.h>
#include <conio.h>
#endif

//--

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
        std::cout << "  " << (i + 1) << "): ";
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

//--

ToolMake::ToolMake()
{}

int ToolMake::run(const char* argv0, const Commandline& cmdline)
{
    //--

    Configuration config;
    if (!config.parseOptions(argv0, cmdline)) {
        std::cout << "Invalid/incomplete configuration\n";
        return -1;
    }

    if (cmdline.has("interactive"))
        if (!RunInteractiveConfig(config))
            return false;

    if (!config.parsePaths(argv0, cmdline)) {
        std::cout << "Invalid/incomplete configuration\n";
        return -1;
    }

    //--

    ProjectStructure structure;

    if (!config.engineSourcesPath.empty())
        structure.scanProjects(ProjectGroupType::Engine, config.engineSourcesPath);

    if (!config.engineScriptPath.empty())
        structure.scanScriptProjects(ProjectGroupType::MonoScripts, config.engineScriptPath / "src");

    if (!config.projectSourcesPath.empty())
        structure.scanProjects(ProjectGroupType::User, config.projectSourcesPath);

    uint32_t totalFiles = 0;
    if (!structure.scanContent(totalFiles))
        return -1;

    if (!structure.setupProjects(config))
        return -1;

    if (!structure.resolveProjectDependencies(config))
        return -1;

    std::cout << "Found " << totalFiles << " total files across " << structure.projects.size() << " projects\n";

    if (!structure.deployFiles(config))
        return -1;

    ProjectGenerator codeGenerator(config);
    if (!codeGenerator.extractProjects(structure))
        return -1;

    if (!codeGenerator.generateAutomaticCode())
        return -1;

    if (!codeGenerator.generateExtraCode())
        return -1;

    if (config.generator == GeneratorType::VisualStudio)
    {
        SolutionGeneratorVS gen(config, codeGenerator);
        if (!gen.generateSolution())
            return -1;
        if (!gen.generateProjects())
            return -1;
    }
    else if (config.generator == GeneratorType::CMake)
    {
        if (!GenerateInlinedReflection(config, structure, codeGenerator))
            return -1;

        SolutionGeneratorCMAKE gen(config, codeGenerator);
        if (!gen.generateSolution())
            return -1;
        if (!gen.generateProjects())
            return -1;
    }

    if (!codeGenerator.saveFiles())
        return -1;

    return 0;
}

//--
