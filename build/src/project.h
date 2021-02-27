#pragma once

#include "common.h"
#include "utils.h"

//--

enum class ProjectFileType : uint8_t
{
    Unknown,
    CppHeader,
    CppSource,
    Bison,
    WindowsResources,
    BuildScript,
};

enum class ProjectFilePlatformFilter : uint8_t
{
    Any,
    WinApi, // platform with WinAPI: windows, UWP
    POSIX, // platforms with POSIX functions
    Windows,
    Linux,
    Invalid,
};

enum class ProjectGroupType : uint8_t
{
    Engine,
    User,
};

enum class ProjectType : uint8_t
{
    Disabled, // we have a project but it is not compiling to anything
    LocalApplication,
    LocalLibrary,
    RttiGenerator,
    ExternalLibrary,
};

struct Configuration;

struct ProjectStructure
{
    struct FileInfo
    {
        ProjectFileType type = ProjectFileType::Unknown;
        ProjectFilePlatformFilter filter = ProjectFilePlatformFilter::Any;

        std::string name; // "test.cpp"

        bool flagUsePch = true;
        bool flagWarn3 = false;
        bool flagExcluded = false;

        std::string projectRelativePath; // path in project "src/vector3.cpp"
        std::string rootRelativePath; // path relative to the source root, ie. "base/math/src/vector3.cpp"
        fs::path absolutePath; // full path to file on disk "Z:\\BoomerEngine\\src\\base\\math\\src\\vector3.cpp"

        bool toggleFlag(std::string_view name, bool value);

        bool checkFilter(PlatformType platform) const;
    };

    struct ProjectInfo;

    struct ProjectGroup
    {
        ProjectGroupType type;
        fs::path rootPath; // where did we start scanning
        std::vector<ProjectInfo*> projects; // in this group only
    };

    struct DeployInfo
    {
        fs::path sourcePath; // where is the source file to deploy
        std::string deployTarget; // the local path in the bin folder, usually just the file name
    };

    struct ToolInfo
    {
        std::string name;
        fs::path executablePath; // where is the executable located
    };

    struct ProjectInfo
    {
        std::string name; // "math" - directory name
        std::string mergedName; // base_math

        bool hasScriptErrors = false;
        bool hasTests = false;
        bool hasMissingDependencies = false;

        bool flagNoInit = false;
        bool flagWarn3 = false;
        bool flagUsePCH = true;
        bool flagConsole = false;
        bool flagDevOnly = false;
        bool flagGenerateMain = false;
        bool flagNoSymbols = false;
        bool flagForceSharedLibrary = false;
        bool flagForceStaticLibrary = false;

        ProjectType type;
        ProjectGroup* group;

        fs::path rootPath; // directory with "build.lua"    

        std::vector<std::string> dependencies;
        std::vector<std::string> optionalDependencies;
        std::vector<ProjectInfo*> resolvedDependencies;

        std::vector<std::string> localDefines; // local defines to add when compiling this project alone
        std::vector<std::string> globalDefines; // global defines to add for the whole solution (usually stuff like HAS_EDITOR, HAS_PHYSX4, etc)

        std::vector<fs::path> libraryInlcudePaths; // include paths to use
        std::vector<fs::path> libraryLinkFile; // additional files to link with

        std::vector<DeployInfo> deployList; // list of additional files to deploy to binary directory
        
        std::vector<ToolInfo> tools;

        //--

        std::vector<FileInfo*> files;
        std::unordered_map<std::string, FileInfo*> filesMapByName;
        std::unordered_map<std::string, FileInfo*> filesMapByRelativePath;

        //--

        ~ProjectInfo();

        bool scanContent(); // scan for actual files

        bool setupProject(const Configuration& config); // runs lua to discover content of the project, NOTE: result may depend on the configuration

        bool toggleFlag(std::string_view name, bool value);

        void addProjectDependency(std::string_view name, bool optional=false);
        void addLocalDefine(std::string_view name);
        void addGlobalDefine(std::string_view name);
        
        FileInfo* findFileByRelativePath(std::string_view name) const; // src/test/crap.cpp

        const ToolInfo* findToolByName(std::string_view name) const;

    private:
        static int ExportAtPanic(lua_State* L);
        static int ExportAtError(lua_State* L);
        static int ExportProjectOption(lua_State* L); // string, bool [opt]
        static int ExportProjectType(lua_State* L); // string
        static int ExportLinkProject(lua_State* L); // string
        static int ExportLinkOptionalProject(lua_State* L); // string
        static int ExportLocalDefine(lua_State* L); // string
        static int ExportGlobalDefine(lua_State* L); // string
        static int ExportFileOption(lua_State* L); // string, string, bool [opt]
        static int ExportFileFilter(lua_State* L); // string, string
        static int ExportDeploy(lua_State* L); // string, string [opt]
        static int ExportLibraryInclude(lua_State* L); // string
        static int ExportLibraryLink(lua_State* L); // string
        static int ExportTool(lua_State* L); // string, string

        bool internalAddStringOnce(std::vector<std::string>& deps, std::string_view name);

        void internalRegisterFunctions(lua_State* L);
        void internalRegisterFunction(lua_State* L, const char* name, lua_CFunction ptr);

        void internalExportConfiguration(lua_State* L, const Configuration& config);

        void scanFilesAtDir(const fs::path& directoryPath, bool headersOnly);
        bool internalTryAddFileFromPath(const fs::path& absolutePath, bool headersOnly);

        static ProjectFileType FileTypeForExtension(std::string_view ext);
        static ProjectFilePlatformFilter FilterTypeByName(std::string_view ext);
    };

    std::vector<ProjectGroup*> groups;
    std::vector<ProjectInfo*> projects;
    std::unordered_map<std::string, ProjectInfo*> projectsMap;

    ~ProjectStructure();

    void scanProjects(ProjectGroupType group, fs::path rootScanPath);

    bool setupProjects(const Configuration& config);
    bool scanContent(uint32_t& outTotalFiles);

    ProjectInfo* findProject(std::string_view name);

    bool resolveProjectDependencies(const Configuration& config);

    bool deployFiles(const Configuration& config);

private:
    void scanProjectsAtDir(ProjectGroup* group, std::vector<std::string_view>& directoryNames, fs::path directoryPath);
    bool resolveProjectDependency(std::string_view name, std::vector<ProjectInfo*>& outProjects);
    void addProjectDependency(ProjectInfo* project, std::vector<ProjectInfo*>& outProjects);
};

//--

struct Configuration
{
    ToolType tool;
    BuildType build;
    PlatformType platform;
    GeneratorType generator;
    LibraryType libs;
    ConfigurationType configuration;

    bool force = false; // usually means force write all files

    fs::path builderExecutablePath;
    fs::path builderEnvPath;

    fs::path engineRootPath;
    fs::path projectRootPath;

    fs::path engineSourcesPath;
    fs::path projectSourcesPath; // may be empty

    fs::path solutionPath; // build folder
    fs::path deployPath; // "bin" folder when all crap is written

    // windows.vs2019.standalone.final
    // linux.cmake.dev.release
    std::string mergedName() const;

    bool parseOptions(const char* executable, const Commandline& cmd);
    bool parsePaths(const char* executable, const Commandline& cmd);

    bool save(const fs::path& path) const;
    bool load(const fs::path& path);
};