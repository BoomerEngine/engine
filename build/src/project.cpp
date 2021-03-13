#include "common.h"
#include "project.h"

//--

std::string Configuration::mergedName() const
{
    std::string ret;
    ret.reserve(100);
    ret += NameEnumOption(platform);
    ret += ".";
    ret += NameEnumOption(generator);
    ret += ".";
    ret += NameEnumOption(build);
    ret += ".";
    ret += NameEnumOption(libs);
    ret += ".";
    ret += NameEnumOption(configuration);
    return ret;
}

bool Configuration::save(const fs::path& path) const
{
    return SaveFileFromString(path, mergedName());
}

bool Configuration::load(const fs::path& path)
{
    std::string str;
    if (!LoadFileToString(path, str))
        return false;

    std::vector<std::string_view> parts;
    SplitString(str, ".", parts);
    if (parts.size() != 5)
        return false;

    bool valid = ParsePlatformType(parts[0], platform);
    valid &= ParseGeneratorType(parts[1], generator);
    valid &= ParseBuildType(parts[2], build);
    valid &= ParseLibraryType(parts[3], libs);
    valid &= ParseConfigurationType(parts[4], configuration);
    return valid;
}

bool Configuration::parseOptions(const char* path, const Commandline& cmd)
{
    builderExecutablePath = fs::absolute(path);
    if (!fs::is_regular_file(builderExecutablePath))
    {
        std::cout << "Invalid local executable name: " << builderExecutablePath << "\n";
        return false;
    }

    builderEnvPath = builderExecutablePath.parent_path().parent_path();
    std::cout << "EnvPath: " << builderEnvPath << "\n";

    if (!fs::is_directory(builderEnvPath / "vs"))
    {
        std::cout << "Lua-build is run from invalid directory and does not have required local files (vs folder is missing)\n";
        return false;
    }

    if (!fs::is_directory(builderEnvPath / "cmake"))
    {
        std::cout << "Lua-build is run from invalid directory and does not have required local files (cmake folder is missing)\n";
        return false;
    }

    {
        const auto& str = cmd.get("tool");
        if (str.empty())
            this->tool = ToolType::SolutionGenerator;
        else if (str == "make")
            this->tool = ToolType::SolutionGenerator;
        else if (str == "reflection")
            this->tool = ToolType::ReflectionGenerator;
        else
        {
            std::cout << "Invalid tool specified\n";
            return false;
        }
    }

    {
        const auto& str = cmd.get("build");
        if (str.empty())
        {
            this->build = BuildType::Development;
        }
        else if (!ParseBuildType(str, this->build))
        {
            std::cout << "Invalid build type '" << str << "'specified\n";
            return false;
        }
    }

    {
        const auto& str = cmd.get("platform");
        if (str.empty())
        {
            this->platform = PlatformType::Windows;
        }
        else if (!ParsePlatformType(str, this->platform))
        {
            std::cout << "Invalid platform type '" << str << "'specified\n";
            return false;
        }
    }

    {
        const auto& str = cmd.get("generator");
        if (str.empty())
        {
            if (this->platform == PlatformType::Windows || this->platform == PlatformType::UWP)
                this->generator = GeneratorType::VisualStudio;
            else
                this->generator = GeneratorType::CMake;
        }
        else if (!ParseGeneratorType(str, this->generator))
        {
            std::cout << "Invalid generator type '" << str << "'specified\n";
            return false;
        }
    }

    {
        const auto& str = cmd.get("libs");
        if (str.empty())
        {
            if (this->build == BuildType::Development)
                this->libs = LibraryType::Shared;
            else
                this->libs = LibraryType::Static;
        }
        else if (!ParseLibraryType(str, this->libs))
        {
            std::cout << "Invalid library type '" << str << "'specified\n";
            return false;
        }
    }

    {
        const auto& str = cmd.get("config");
        if (str.empty())
        {
            if (this->build == BuildType::Development)
                this->configuration = ConfigurationType::Debug;
            else
                this->configuration = ConfigurationType::Final;
        }
        else if (!ParseConfigurationType(str, this->configuration))
        {
            std::cout << "Invalid configuration type '" << str << "'specified\n";
            return false;
        }
    }

    force = cmd.has("force");

    return true;
}

bool Configuration::parsePaths(const char* executable, const Commandline& cmd)
{
    //--

    fs::path rootPath;

    {
        const auto& str = cmd.get("engineDir");
        if (str.empty())
        {
            //std::cout << "No engine path specified, using current directory\n";
            engineRootPath = fs::current_path();
        }
        else
        {
            engineRootPath = fs::path(str);

            std::error_code ec;
            if (!fs::is_directory(engineRootPath))
            {
                std::cout << "Specified engine directory does not exist\n";
                return false;
            }
        }

        rootPath = engineRootPath;

        this->engineSourcesPath = engineRootPath / "src";
        if (!fs::is_directory(engineSourcesPath))
        {
            std::cout << "Specified engine directory has no source directory\n";
            return false;
        }
    }

    {
        const auto& str = cmd.get("deployDir");
        if (str.empty())
        {
            //std::cout << "No deploy directory specified, using default one\n";

            std::string solutionPartialPath = ".bin/";
            solutionPartialPath += mergedName();

            this->deployPath = rootPath / solutionPartialPath;
        }
        else
        {
            this->deployPath = str;
        }

        std::error_code ec;
        if (!fs::is_directory(deployPath, ec))
        {
            if (!fs::create_directories(deployPath, ec))
            {
                std::cout << "Failed to create deploy directory " << deployPath << "\n";
                return false;
            }
        }
    }

    {
        const auto& str = cmd.get("outDir");
        if (str.empty())
        {
            //std::cout << "No output directory specified, using default one\n";

            std::string solutionPartialPath = ".temp/";
            solutionPartialPath += mergedName();

            this->solutionPath = rootPath / solutionPartialPath;
        }
        else
        {
            this->solutionPath = str;
        }

        std::error_code ec;
        if (!fs::is_directory(solutionPath, ec))
        {
            if (!fs::create_directories(solutionPath, ec))
            {
                std::cout << "Failed to create solution directory " << solutionPath << "\n";
                return false;
            }
        }
    }

    engineSourcesPath = engineSourcesPath.make_preferred();
    projectSourcesPath = projectSourcesPath.make_preferred();
    solutionPath = solutionPath.make_preferred();
    deployPath = deployPath.make_preferred();

    return true;
}

//--

ProjectStructure::ProjectInfo::~ProjectInfo()
{
    for (auto* file : files)
        delete file;
}

void ProjectStructure::ProjectInfo::internalRegisterFunction(lua_State* L, const char* name, lua_CFunction ptr)
{
    lua_pushcfunction(L, ptr);
    lua_setglobal(L, name);
}

void ProjectStructure::ProjectInfo::internalRegisterFunctions(lua_State* L)
{
    lua_atpanic(L, &ExportAtPanic);

    internalRegisterFunction(L, "ProjectOption", &ExportProjectOption);
    internalRegisterFunction(L, "FileOption", &ExportFileOption);
    internalRegisterFunction(L, "FileFilter", &ExportFileFilter);
    internalRegisterFunction(L, "ProjectType", &ExportProjectType);
    internalRegisterFunction(L, "Dependency", &ExportLinkProject);
    internalRegisterFunction(L, "DependencyOptional", &ExportLinkOptionalProject);
    internalRegisterFunction(L, "LocalDefine", &ExportLocalDefine);
    internalRegisterFunction(L, "GlobalDefine", &ExportGlobalDefine);
    internalRegisterFunction(L, "Tool", &ExportTool);
    internalRegisterFunction(L, "Deploy", &ExportDeploy);
    internalRegisterFunction(L, "LibraryLink", &ExportLibraryLink);
    internalRegisterFunction(L, "LibraryInclude", &ExportLibraryInclude);    
}

template< typename T >
static void ExportEnumToLUA(lua_State* L, const char* name, T val)
{
    lua_pushstring(L, std::string(NameEnumOption(val)).c_str());
    lua_setglobal(L, name);
}

void ProjectStructure::ProjectInfo::internalExportConfiguration(lua_State* L, const Configuration& config)
{
    ExportEnumToLUA(L, "GeneratorName", config.generator);
    ExportEnumToLUA(L, "PlatformName", config.platform);
    ExportEnumToLUA(L, "BuildName", config.build);
    ExportEnumToLUA(L, "ConfigurationName", config.configuration);
    
    lua_pushboolean(L, config.libs == LibraryType::Static);
    lua_setglobal(L, "UseStaticLibs");
}

bool ProjectStructure::ProjectInfo::setupProject(const Configuration& config)
{
    lua_State* L = luaL_newstate();  /* create state */
    if (L == NULL)
    {
        std::cout << "Cannot create state: not enough memory\n";
        return false;
    }

    L->selfPtr = this;

    luaL_openlibs(L);

    internalRegisterFunctions(L);
    internalExportConfiguration(L, config);

    auto scriptFilePath = rootPath / "build.lua";
    std::string code;
    if (!LoadFileToString(scriptFilePath, code))
    {
        std::cout << "Cannot create state: not enough memory\n";
        return false;
    }

    int ret = luaL_loadstring(L, code.c_str());
    if (LUA_OK != ret)
    {
        std::string_view text = luaL_checkstring(L, 1);

        std::cout << "Failed to parse build script at " << scriptFilePath << "\n";
        std::cout << "LUA error: " << text << "\n";

        lua_close(L);
        return false;
    }

    ret = lua_pcall(L, 0, 0, 0);
    if (LUA_OK != ret)
    {
        std::string_view text = luaL_checkstring(L, 1);

        std::cout << "Failed to run loaded script at " << scriptFilePath << "\n";
        std::cout << "LUA error: " << text << "\n";
        lua_close(L);
        return false;
    }

    return !hasScriptErrors;
}

ProjectFileType ProjectStructure::ProjectInfo::FileTypeForExtension(std::string_view ext)
{
    if (ext == ".h" || ext == ".hpp" || ext == ".hxx" || ext == ".inl")
        return ProjectFileType::CppHeader;
    if (ext == ".c" || ext == ".cpp" || ext == ".cxx")
        return ProjectFileType::CppSource;
    if (ext == ".bison")
        return ProjectFileType::Bison;
    if (ext == ".lua")
        return ProjectFileType::BuildScript;
    if (ext == ".rc")
        return ProjectFileType::WindowsResources;

    return ProjectFileType::Unknown;
}

ProjectFilePlatformFilter ProjectStructure::ProjectInfo::FilterTypeByName(std::string_view ext)
{
    if (ext == "windows")
        return ProjectFilePlatformFilter::Windows;
    else if (ext == "linux")
        return ProjectFilePlatformFilter::Linux;
    else if (ext == "winapi")
        return ProjectFilePlatformFilter::WinApi;
    else if (ext == "posix")
        return ProjectFilePlatformFilter::POSIX;

    return ProjectFilePlatformFilter::Invalid;
}

bool ProjectStructure::ProjectInfo::internalTryAddFileFromPath(const fs::path& absolutePath, bool headersOnly)
{
    const auto ext = absolutePath.extension().u8string();

    const auto type = FileTypeForExtension(ext);

    if (type == ProjectFileType::Unknown)
        return false;

    if (headersOnly && type != ProjectFileType::CppHeader)
        return false;

    const auto shortName = absolutePath.filename().u8string();

    auto* file = new FileInfo;
    file->type = type;
    file->absolutePath = absolutePath;
    file->name = shortName;
    file->projectRelativePath = MakeGenericPathEx(fs::relative(absolutePath, rootPath));
    file->rootRelativePath = MakeGenericPathEx(fs::relative(absolutePath, group->rootPath));

    if (EndsWith(file->name, "_test.cpp") || EndsWith(file->name, "_tests.cpp"))
    {
        if (!hasTests)
        {
            //std::cout << "Project '" << mergedName << "' determined to have tests\n";
            hasTests = true;
        }
    }

    filesMapByName[shortName] = file;
    filesMapByRelativePath[file->projectRelativePath] = file;

    files.push_back(file);
    return true;    
}

void ProjectStructure::ProjectInfo::scanFilesAtDir(const fs::path& directoryPath, bool headersOnly)
{
    try
    {
        if (fs::is_directory(directoryPath))
        {
            for (const auto& entry : fs::directory_iterator(directoryPath))
            {
                const auto name = entry.path().filename().u8string();

                if (entry.is_directory())
                    scanFilesAtDir(entry.path(), headersOnly);
                else if (entry.is_regular_file())
                    internalTryAddFileFromPath(entry.path(), headersOnly);
            }
        }
    }
    catch (fs::filesystem_error& e)
    {
        std::cout << "Filesystem Error: " << e.what() << "\n";
    }
}

bool ProjectStructure::ProjectInfo::scanContent()
{
    const auto buildPath = rootPath / "build.lua";
    if (fs::is_regular_file(buildPath))
        internalTryAddFileFromPath(buildPath, false);

    const auto sourcesPath = rootPath / "src";
    if (fs::is_directory(sourcesPath))
    {
        scanFilesAtDir(rootPath / "include", true);
        scanFilesAtDir(rootPath / "res", false);
        scanFilesAtDir(rootPath / "natvis", false);
        scanFilesAtDir(rootPath / "src", false);
    }

    sort(files.begin(), files.end(), [](const auto* a, const auto* b) -> bool {
        return a->name < b->name;
        });

    return true;
}

int ProjectStructure::ProjectInfo::ExportAtPanic(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    std::cout << "Project '" << self->mergedName << "' encountered critical script error: " << name << "\n";
    self->hasScriptErrors = true;
    return 0;
}

int ProjectStructure::ProjectInfo::ExportAtError(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    std::cout << "Project '" << self->mergedName << "' encountered script error: " << name << "\n";
    self->hasScriptErrors = true;
    return 0;
}

int ProjectStructure::ProjectInfo::ExportProjectOption(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    bool value = lua_isnoneornil(L, 2) ? true : lua_toboolean(L, 3);

    if (!self->toggleFlag(name, value))
    {
        std::cout << "Project '" << self->mergedName << "' uses invalid flag: '" << name << "'\n";
        self->hasScriptErrors = true;
    }

    return 0;
}

int ProjectStructure::ProjectInfo::ExportFileOption(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    std::string_view flag = luaL_checkstring(L, 2);
    bool value = lua_isnoneornil(L, 3) ? true : lua_toboolean(L, 3);

    if (auto* file = self->findFileByRelativePath(name))
    {
        file->toggleFlag(flag, value);
    }
    else
    {
        std::cout << "Unknown file '" << name << "'\n";
        self->hasScriptErrors = true;
    }

    return 0;
}

int ProjectStructure::ProjectInfo::ExportFileFilter(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    std::string_view flag = luaL_checkstring(L, 2);

    auto filter = FilterTypeByName(flag);
    if (filter == ProjectFilePlatformFilter::Invalid)
    {
        std::cout << "File'" << self->mergedName << "' has invalid platform filter: '" << name << "'\n";
        self->hasScriptErrors = true;
    }
    else
    {
        if (auto* file = self->findFileByRelativePath(name))
        {
            file->filter = filter;
        }
        else
        {
            std::cout << "Unknown file '" << name << "'\n";
            self->hasScriptErrors = true;
        }
    }

    return 0;
}

int ProjectStructure::ProjectInfo::ExportDeploy(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view path = luaL_checkstring(L, 1);

    auto fullPath = self->rootPath / path;
    std::error_code ec;
    if (!fs::exists(fullPath, ec))
    {
        std::cout << "Referenced deployment file '" << path << "' does not exist in the library folder\n";
        self->hasScriptErrors = true;
    }
    else if (!fs::is_regular_file(fullPath, ec))
    {
        std::cout << "Referenced deployment object '" << path << "' is not a file\n";
        self->hasScriptErrors = true;
    }
    else
    {
        DeployInfo info;
        info.sourcePath = fullPath;

        if (lua_isnoneornil(L, 2))
            info.deployTarget = fullPath.filename().u8string();
        else
            info.deployTarget = luaL_checkstring(L, 2);

        self->deployList.push_back(info);
    }

    return 0;
}

int ProjectStructure::ProjectInfo::ExportTool(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    std::string_view path = luaL_checkstring(L, 2);

    auto fullPath = self->rootPath / path;
    fullPath = fullPath.make_preferred();

    std::error_code ec;
    if (!fs::exists(fullPath, ec))
    {
        std::cout << "Referenced binary file '" << path << "' does not exist\n";
        self->hasScriptErrors = true;
    }
    else if (!fs::is_regular_file(fullPath, ec))
    {
        std::cout << "Referenced binary file '" << path << "' is not a file\n";
        self->hasScriptErrors = true;
    }
    else
    {
        ToolInfo info;
        info.name = name;
        info.executablePath = fullPath;
        self->tools.push_back(info);
    }

    return 0;
}

int ProjectStructure::ProjectInfo::ExportLibraryInclude(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view path = luaL_checkstring(L, 1);

    auto fullPath = self->rootPath / path;
    std::error_code ec;
    if (!fs::exists(fullPath, ec))
    {
        std::cout << "Referenced include path '" << path << "' does not exist in the library folder\n";
        self->hasScriptErrors = true;
    }
    else if (!fs::is_directory(fullPath, ec))
    {
        std::cout << "Referenced include path '" << path << "' is not a directory\n";
        self->hasScriptErrors = true;
    }
    else
    {
        fullPath = fullPath.make_preferred();
        self->libraryInlcudePaths.push_back(fullPath);
    }

    return 0;
}

int ProjectStructure::ProjectInfo::ExportLibraryLink(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view path = luaL_checkstring(L, 1);

    auto fullPath = self->rootPath / path;
    fullPath = fullPath.make_preferred();

    std::error_code ec;
    if (!fs::exists(fullPath, ec))
    {
        std::cout << "Referenced file '" << path << "' does not exist in the library folder\n";
        self->hasScriptErrors = true;
    }
    else if (!fs::is_regular_file(fullPath, ec))
    {
        std::cout << "Referenced object '" << path << "' is not a file\n";
        self->hasScriptErrors = true;
    }
    else
    {
        fullPath = fullPath.make_preferred();
        self->libraryLinkFile.push_back(fullPath);
    }

    return 0;
}

int ProjectStructure::ProjectInfo::ExportProjectType(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    if (name == "library")
        self->type = ProjectType::LocalLibrary;
    else if (name == "app")
        self->type = ProjectType::LocalApplication;
    else if (name == "external")
        self->type = ProjectType::ExternalLibrary;
    else
    {
        std::cout << "Project '" << self->mergedName << "' has invalid type: '" << name << "'\n";
    }

    return 0;
}

bool ProjectStructure::ProjectInfo::toggleFlag(std::string_view name, bool value)
{
    if (name == "warn3")
    {
        flagWarn3 = value;
        return true;
    }
    else if (name == "noinit")
    {
        flagNoInit = value;
        return true;
    }
    else if (name == "pch")
    {
        flagUsePCH = value;
        return true;
    }
    else if (name == "console")
    {
        flagConsole = value;
        return true;
    }
    else if (name == "devonly")
    {
        flagDevOnly = value;
        return true;
    }
    else if (name == "main")
    {
        flagGenerateMain = value;
        return true;
    }
    else if (name == "nosymbols")
    {
        flagNoSymbols = !value;
        return true;
    }
    else if (name == "symbols")
    {
        flagNoSymbols = !value;
        return true;
    }
    else if (name == "forceShared")
    {
        flagForceSharedLibrary = value;
        return true;
    }
    else if (name == "forceStatic")
    {
        flagForceStaticLibrary = value;
        return true;
    }

    return false;
}

void ProjectStructure::ProjectInfo::addProjectDependency(std::string_view name, bool optional /*= false*/)
{
    internalAddStringOnce(optional ? optionalDependencies : dependencies, name);   
}

bool ProjectStructure::ProjectInfo::internalAddStringOnce(std::vector<std::string>& deps, std::string_view name)
{
    for (const auto& str : deps)
        if (str == name)
            return false;

    deps.push_back(std::string(name));
    return true;
}

void ProjectStructure::ProjectInfo::addLocalDefine(std::string_view name)
{
    internalAddStringOnce(localDefines, name);
}

void ProjectStructure::ProjectInfo::addGlobalDefine(std::string_view name)
{
    internalAddStringOnce(globalDefines, name);
}

ProjectStructure::FileInfo* ProjectStructure::ProjectInfo::findFileByRelativePath(std::string_view name) const
{
    auto it = filesMapByRelativePath.find(std::string(name));
    if (it != filesMapByRelativePath.end())
        return it->second;
    return nullptr;
}

const ProjectStructure::ToolInfo* ProjectStructure::ProjectInfo::findToolByName(std::string_view name) const
{
    for (auto& tool : tools)
        if (tool.name == name)
            return &tool;

    for (const auto* dep : resolvedDependencies)
        for (auto& tool : dep->tools)
            if (tool.name == name)
                return &tool;

    return nullptr;
}

bool ProjectStructure::FileInfo::checkFilter(PlatformType platform) const
{
    if (flagExcluded)
        return false;

    switch (filter)
    {
    case ProjectFilePlatformFilter::WinApi:
        return (platform == PlatformType::Windows) || (platform == PlatformType::UWP);
    case ProjectFilePlatformFilter::POSIX:
        return (platform == PlatformType::Linux);
    case ProjectFilePlatformFilter::Windows:
        return (platform == PlatformType::Windows);
    case ProjectFilePlatformFilter::Linux:
        return (platform == PlatformType::Linux);
    }

    return true;
}

bool ProjectStructure::FileInfo::toggleFlag(std::string_view name, bool value)
{
    if (name == "exclude")
    {
        flagExcluded = value;
        return true;
    }
    else if (name == "pch")
    {
        flagUsePch = value;
        return true;
    }
    else if (name == "nopch")
    {
        flagUsePch = !value;
        return true;
    }
    else if (name == "warn3")
    {
        flagWarn3 = value;
        return true;
    }

    std::cout << "Unknown file option '" << name << "' used on file " << absolutePath << "\n";
    return false;
}

int ProjectStructure::ProjectInfo::ExportLinkProject(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    self->addProjectDependency(name);
    return 0;
}

int ProjectStructure::ProjectInfo::ExportLinkOptionalProject(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    self->addProjectDependency(name, true);
    return 0;
}

int ProjectStructure::ProjectInfo::ExportLocalDefine(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    self->addLocalDefine(name);
    return 0;
}

int ProjectStructure::ProjectInfo::ExportGlobalDefine(lua_State* L)
{
    auto* self = (ProjectInfo*)L->selfPtr;
    std::string_view name = luaL_checkstring(L, 1);
    self->addGlobalDefine(name);
    return 0;
}

ProjectStructure::~ProjectStructure()
{
    for (auto* proj : projects)
        delete proj;
    for (auto* group : groups)
        delete group;
}

static std::string BuildMergedProjectName(const std::vector<std::string_view>& parts)
{
    std::string ret;

    for (const auto& name : parts)
    {
        if (!ret.empty())
            ret += "_";
        ret += name;
    }

    return ret;
}

void ProjectStructure::scanProjectsAtDir(ProjectGroup* group, std::vector<std::string_view>& directoryNames, fs::path directoryPath)
{
    bool hasBuildLua = false;

    try
    {
        for (const auto& entry : fs::directory_iterator(directoryPath))
        {
            const auto name = entry.path().filename().u8string();

            if (entry.is_directory())
            {
                directoryNames.push_back(name);
                scanProjectsAtDir(group, directoryNames, entry.path());
                directoryNames.pop_back();
            }
            else if (entry.is_regular_file())
            {
                if (name == "build.lua")
                {
                    hasBuildLua = true;
                }
            }
        }
    }
    catch (fs::filesystem_error& e)
    {
        std::cout << "Filesystem Error: " << e.what() << "\n";
    }

    if (hasBuildLua)
    {
        auto* project = new ProjectInfo();
        project->type = ProjectType::Disabled;
        project->group = group;
        project->mergedName = BuildMergedProjectName(directoryNames);
        project->name = std::string(directoryNames.back());
        project->rootPath = directoryPath;

        group->projects.push_back(project);
        projects.push_back(project);

        projectsMap[project->mergedName] = project;

        //std::cout << "Found project '" << project->mergedName << "' at " << project->rootPath << "\n";
    }
}

ProjectStructure::ProjectInfo* ProjectStructure::findProject(std::string_view name)
{
    auto it = projectsMap.find(std::string(name));
    if (it != projectsMap.end())
        return it->second;
    return nullptr;
}

void ProjectStructure::addProjectDependency(ProjectInfo* project, std::vector<ProjectInfo*>& outProjects)
{
    auto it = std::find(outProjects.begin(), outProjects.end(), project);
    if (it == outProjects.end())
        outProjects.push_back(project);
}

bool ProjectStructure::resolveProjectDependency(std::string_view name, std::vector<ProjectInfo*>& outProjects)
{
    if (name == "*all_tests*")
    {
        for (auto* proj : projects)
            if (proj->hasTests)
                addProjectDependency(proj, outProjects);

        return true;
    }
    else if (EndsWith(name, "_*"))
    {
        const auto pattern = name.substr(0, name.length() - 1);
        for (auto* proj : projects)
            if (BeginsWith(proj->mergedName, pattern) && proj->type == ProjectType::LocalLibrary)
                addProjectDependency(proj, outProjects);

        return true;
    }
    else
    {
        auto* proj = findProject(name);
        if (proj)
        {
            if (proj->type == ProjectType::LocalApplication)
            {
                std::cout << "Project '" << proj->mergedName << "' is an application and can't be a dependency\n";
                return true;
            }

            addProjectDependency(proj, outProjects);
            return true;
        }
    }

    return false;
}

bool ProjectStructure::resolveProjectDependencies(const Configuration& config)
{
    bool hasValidDeps = true;

    // create the special rtti generator project
    ProjectInfo* rttiGenerator = nullptr;
    if (config.generator == GeneratorType::VisualStudio)
    {
        rttiGenerator = new ProjectInfo();
        rttiGenerator->name = "_rtti_gen";
        rttiGenerator->mergedName = "_rtti_gen";
        rttiGenerator->type = ProjectType::RttiGenerator;
        projects.push_back(rttiGenerator);
    }

    // check and resolve dependencies
    for (auto* proj : projects)
    {
        proj->resolvedDependencies.clear();

        if (proj->type == ProjectType::LocalApplication || proj->type == ProjectType::LocalLibrary)
        {
            if (rttiGenerator)
                proj->resolvedDependencies.push_back(rttiGenerator);

            for (const auto& dep : proj->dependencies)
            {
                if (!resolveProjectDependency(dep, proj->resolvedDependencies))
                {
                    std::cout << "Project '" << proj->mergedName << "' has dependency '" << dep << "' that can't be properly resolved!\n";
                    proj->hasMissingDependencies = true;
                    hasValidDeps = false;
                }
            }

            for (const auto& dep : proj->optionalDependencies)
                resolveProjectDependency(dep, proj->resolvedDependencies);
        }
        else if (!proj->dependencies.empty())
        {
            std::cout << "Project '" << proj->mergedName << "' has dependencies even though it's not a source-code based project\n";
        }
    }

    if (!hasValidDeps)
    {
        std::cout << "There are project with invalid dependencies, can't continue solution generation\n";
        return false;
    }

    return true;
}

void ProjectStructure::scanProjects(ProjectGroupType groupType, fs::path rootScanPath)
{
    std::cout << "Scanning for projects at " << rootScanPath << "\n";

    auto* group = new ProjectGroup;
    group->type = groupType;
    group->rootPath = rootScanPath;
    groups.push_back(group);

    std::vector<std::string_view> directoryNames;
    scanProjectsAtDir(group, directoryNames, rootScanPath);

    std::cout << "Discovered " << group->projects.size() << " project(s)\n";
}

bool ProjectStructure::setupProjects(const Configuration& config)
{
    bool valid = true;

    for (auto* project : projects)
        valid &= project->setupProject(config);

    return valid;
}

bool ProjectStructure::scanContent(uint32_t& outTotalFiles)
{
    bool valid = true;

    outTotalFiles = 0;

    //#pragma omp parallel num_threads(16)
    for (auto* project : projects)
    {
        valid &= project->scanContent();
        outTotalFiles += (uint32_t)project->files.size();
    }

    return valid;
}

bool ProjectStructure::deployFiles(const Configuration& config)
{
    bool valid = true;

    for (const auto* proj : projects)
    {
        for (const auto& deploy : proj->deployList)
        {
            fs::path targetPath = config.deployPath / deploy.deployTarget;
            valid &= CopyNewerFile(deploy.sourcePath, targetPath);
        }
    }

    return valid;
}
