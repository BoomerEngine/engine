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

        string name; // "test.cpp"

        bool flagUsePch = true;
        bool flagWarn3 = false;
        bool flagExcluded = false;

        string projectRelativePath; // path in project "src/vector3.cpp"
        string rootRelativePath; // path relative to the source root, ie. "base/math/src/vector3.cpp"
        filesystem::path absolutePath; // full path to file on disk "Z:\\BoomerEngine\\src\\base\\math\\src\\vector3.cpp"

        bool toggleFlag(string_view name, bool value);

        bool checkFilter(PlatformType platform) const;
    };

    struct ProjectInfo;

    struct ProjectGroup
    {
        ProjectGroupType type;
        filesystem::path rootPath; // where did we start scanning
        vector<ProjectInfo*> projects; // in this group only
    };

    struct DeployInfo
    {
        filesystem::path sourcePath; // where is the source file to deploy
        string deployTarget; // the local path in the bin folder, usually just the file name
    };

    struct ToolInfo
    {
        string name;
        filesystem::path executablePath; // where is the executable located
    };

    struct ProjectInfo
    {
        string name; // "math" - directory name
        string mergedName; // base_math

        bool hasScriptErrors = false;
        bool hasTests = false;
        bool hasMissingDependencies = false;

        bool flagNoInit = false;
        bool flagWarn3 = false;
        bool flagUsePCH = true;
        bool flagConsole = false;
        bool flagDevOnly = false;
        bool flagGenerateMain = false;

        ProjectType type;
        ProjectGroup* group;

        filesystem::path rootPath; // directory with "build.lua"    

        vector<string> dependencies;
        vector<string> optionalDependencies;
        vector<ProjectInfo*> resolvedDependencies;

        vector<string> localDefines; // local defines to add when compiling this project alone
        vector<string> globalDefines; // global defines to add for the whole solution (usually stuff like HAS_EDITOR, HAS_PHYSX4, etc)

        vector<filesystem::path> libraryInlcudePaths; // include paths to use
        vector<filesystem::path> libraryLinkFile; // additional files to link with

        vector<DeployInfo> deployList; // list of additional files to deploy to binary directory
        
        vector<ToolInfo> tools;

        //--

        vector<FileInfo*> files;
        unordered_map<string, FileInfo*> filesMapByName;
        unordered_map<string, FileInfo*> filesMapByRelativePath;

        //--

        ~ProjectInfo();

        bool scanContent(); // scan for actual files

        bool setupProject(const Configuration& config); // runs lua to discover content of the project, NOTE: result may depend on the configuration

        bool toggleFlag(string_view name, bool value);

        void addProjectDependency(string_view name, bool optional=false);
        void addLocalDefine(string_view name);
        void addGlobalDefine(string_view name);
        
        FileInfo* findFileByRelativePath(string_view name) const; // src/test/crap.cpp

        const ToolInfo* findToolByName(string_view name) const;

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

        bool internalAddStringOnce(vector<string>& deps, string_view name);

        void internalRegisterFunctions(lua_State* L);
        void internalRegisterFunction(lua_State* L, const char* name, lua_CFunction ptr);

        void internalExportConfiguration(lua_State* L, const Configuration& config);

        void scanFilesAtDir(const filesystem::path& directoryPath, bool headersOnly);
        bool internalTryAddFileFromPath(const filesystem::path& absolutePath, bool headersOnly);

        static ProjectFileType FileTypeForExtension(string_view ext);
        static ProjectFilePlatformFilter FilterTypeByName(string_view ext);
    };

    vector<ProjectGroup*> groups;
    vector<ProjectInfo*> projects;
    unordered_map<string, ProjectInfo*> projectsMap;

    ~ProjectStructure();

    void scanProjects(ProjectGroupType group, filesystem::path rootScanPath);

    bool setupProjects(const Configuration& config);
    bool scanContent(uint32_t& outTotalFiles);

    ProjectInfo* findProject(string_view name);

    bool resolveProjectDependencies();

    bool deployFiles(const Configuration& config);

private:
    void scanProjectsAtDir(ProjectGroup* group, vector<string_view>& directoryNames, filesystem::path directoryPath);
    bool resolveProjectDependency(string_view name, vector<ProjectInfo*>& outProjects);
    void addProjectDependency(ProjectInfo* project, vector<ProjectInfo*>& outProjects);
};

//--

struct Configuration
{
    ToolType tool;
    BuildType build;
    PlatformType platform;
    GeneratorType generator;
    ConfigurationType configuration;

    filesystem::path builderExecutablePath;
    filesystem::path builderEnvPath;

    filesystem::path engineRootPath;
    filesystem::path projectRootPath;

    filesystem::path engineSourcesPath;
    filesystem::path projectSourcesPath; // may be empty

    filesystem::path solutionPath; // build folder
    filesystem::path deployPath; // "bin" folder when all crap is written

    // windows.vs2019.standalone.final
    // linux.cmake.dev.release
    string mergedName() const;

    bool parse(const char* executable, const Commandline& cmd);    
};