#include "common.h"
#include "project.h"
#include "generated.h"
#include "generatorVS.h"

static const char* VS_PROJECT_VERSION = "16.0";
static const char* VS_TOOLSET_VERSION = "v142";

SolutionGeneratorVS::SolutionGeneratorVS(const Configuration& config, CodeGenerator& gen)
    : m_config(config)
    , m_gen(gen)
{
    m_visualStudioScriptsPath = config.builderEnvPath / "vs";
    m_buildWithLibs = (config.libs == LibraryType::Static);
}

void SolutionGeneratorVS::printSolutionDeclarations(std::stringstream& f, const CodeGenerator::GeneratedGroup* g)
{
    writelnf(f, "Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"%s\", \"%s\", \"%s\"", g->name.c_str(), g->name.c_str(), g->assignedVSGuid.c_str());
    writeln(f, "EndProject");

    for (const auto* child : g->children)
        printSolutionDeclarations(f, child);

    for (const auto* p : g->projects)
    {
        auto projectFilePath = p->projectPath / p->mergedName;
        projectFilePath += ".vcxproj";

        writelnf(f, "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%s\", \"%s\"", p->mergedName.c_str(), projectFilePath.u8string().c_str(), p->assignedVSGuid.c_str());
        writeln(f, "EndProject");
    }
}

void SolutionGeneratorVS::printSolutionParentLinks(std::stringstream& f, const CodeGenerator::GeneratedGroup* g)
{
    for (const auto* child : g->children)
    {
        // {808CE59B-D2F0-45B3-90A4-C63953B525F5} = {943E2949-809F-4411-A11F-51D51E9E579B}
        writelnf(f, "		%s = %s", child->assignedVSGuid.c_str(), g->assignedVSGuid.c_str());
        printSolutionParentLinks(f, child);
    }

    for (const auto* child : g->projects)
    {
        // {808CE59B-D2F0-45B3-90A4-C63953B525F5} = {943E2949-809F-4411-A11F-51D51E9E579B}
        writelnf(f, "		%s = %s", child->assignedVSGuid.c_str(), g->assignedVSGuid.c_str());
    }
}

static const char* NameVisualStudioConfiguration(ConfigurationType config)
{    
    switch (config)
    {
        case ConfigurationType::Checked: return "Checked";
        case ConfigurationType::Release: return "Release";
        case ConfigurationType::Debug: return "Debug";
        case ConfigurationType::Final: return "Final";
    }

    return "Release";
}

bool SolutionGeneratorVS::generateSolution()
{
    const auto solutionFileName = std::string("boomer.") + m_config.mergedName() + ".sln";

    auto* file = m_gen.createFile(m_config.solutionPath / solutionFileName);
    auto& f = file->content;

    std::cout << "-------------------------------------------------------------------------------------------\n";
    std::cout << "-- SOLUTION FILE: " << file->absolutePath << "\n";
    std::cout << "-------------------------------------------------------------------------------------------\n";

    writeln(f, "Microsoft Visual Studio Solution File, Format Version 12.00");
    /*if (toolset.equals("v140")) {
        writeln(f, "# Visual Studio 14");
        writeln(f, "# Generated file, please do not modify");
        writeln(f, "VisualStudioVersion = 14.0.25420.1");
        writeln(f, "MinimumVisualStudioVersion = 10.0.40219.1");
    }
    else*/
    {
        writeln(f, "# Visual Studio 15");
        writeln(f, "# Generated file, please do not modify");
        writeln(f, "VisualStudioVersion = 15.0.26403.7");
        writeln(f, "MinimumVisualStudioVersion = 10.0.40219.1");
    }

    writeln(f, "Global");

    printSolutionDeclarations(f, m_gen.rootGroup);

    {
        const auto c = NameVisualStudioConfiguration(m_config.configuration);
        const auto p = "x64";

        writeln(f, "	GlobalSection(SolutionConfigurationPlatforms) = preSolution");
        writelnf(f, "		%s|%s = %s|%s", c, p, c, p);
        writeln(f, "	EndGlobalSection");

        // project configs
        writeln(f, "	GlobalSection(ProjectConfigurationPlatforms) = postSolution");

        for (const auto* px : m_gen.projects)
        {
            writelnf(f, "		%s.%s|%s.ActiveCfg = %s|%s", px->assignedVSGuid.c_str(), c, p, c, p);
            writelnf(f, "		%s.%s|%s.Build.0 = %s|%s", px->assignedVSGuid.c_str(), c, p, c, p);
        }

        writeln(f, "	EndGlobalSection");
    }

    {
        writeln(f, "	GlobalSection(SolutionProperties) = preSolution");
        writeln(f, "		HideSolutionNode = FALSE");
        writeln(f, "	EndGlobalSection");
    }

    {
        writeln(f, "	GlobalSection(NestedProjects) = preSolution");
        printSolutionParentLinks(f, m_gen.rootGroup);
        writeln(f, "	EndGlobalSection");
    }

    writeln(f, "EndGlobal");
    return true;
}

bool SolutionGeneratorVS::generateProjects()
{
    bool valid = true;

    for (const auto* p : m_gen.projects)
    {
        if (p->originalProject->type == ProjectType::LocalLibrary || p->originalProject->type == ProjectType::LocalApplication)
        {
            {
                auto projectFilePath = p->projectPath / p->mergedName;
                projectFilePath += ".vcxproj";

                auto* file = m_gen.createFile(projectFilePath);
                valid &= generateSourcesProjectFile(p, file->content);
            }

            {
                auto projectFilePath = p->projectPath / p->mergedName;
                projectFilePath += ".vcxproj.filters";

                auto* file = m_gen.createFile(projectFilePath);
                valid &= generateSourcesProjectFilters(p, file->content);
            }
        }
        else if (p->originalProject->type == ProjectType::RttiGenerator)
        {
            {
                auto projectFilePath = p->projectPath / p->mergedName;
                projectFilePath += ".vcxproj";

                auto* file = m_gen.createFile(projectFilePath);
                valid &= generateRTTIGenProjectFile(p, file->content);
            }
        }
    }

    return true;
}

void SolutionGeneratorVS::extractSourceRoots(const CodeGenerator::GeneratedProject* project, std::vector<fs::path>& outPaths) const
{
    for (const auto& sourceRoot : m_gen.sourceRoots)
        outPaths.push_back(sourceRoot);

    outPaths.push_back(project->originalProject->rootPath / "src");
    outPaths.push_back(project->originalProject->rootPath / "include");

    outPaths.push_back(m_config.solutionPath / "generated/_shared");
    outPaths.push_back(project->generatedPath);
}

bool SolutionGeneratorVS::generateSourcesProjectFile(const CodeGenerator::GeneratedProject* project, std::stringstream& f) const
{
    writeln(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    writeln(f, "<!-- Auto generated file, please do not edit -->");

    writelnf(f, "<Project DefaultTargets=\"Build\" ToolsVersion=\"%s\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">", VS_PROJECT_VERSION);
    writelnf(f, "<Import Project=\"%s\\SharedConfigurationSetup.props\"/>", m_visualStudioScriptsPath.u8string().c_str());

    writeln(f, "<PropertyGroup>");
    writelnf(f, "  <PlatformToolset>%s</PlatformToolset>", VS_TOOLSET_VERSION);
    writelnf(f, "  <VCProjectVersion>%s</VCProjectVersion>", VS_PROJECT_VERSION);
    writeln(f, "  <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>");

    {
        f << "  <SourcesRoot>";
        std::vector<fs::path> sourceRoots;
        extractSourceRoots(project, sourceRoots);

        for (auto& root : sourceRoots)
            f << root.make_preferred().u8string() << "\\;";

        f << "</SourcesRoot>\n";
    }

    {
        f << "  <LibraryIncludePath>";
        for (const auto* lib : project->originalProject->resolvedDependencies)
            if (lib->type == ProjectType::ExternalLibrary)
                for (const auto& path : lib->libraryInlcudePaths)
                    f << path.u8string() << "\\;";
        f << "</LibraryIncludePath>\n";
    }

    const auto relativeSourceDir = fs::relative(project->originalProject->rootPath, project->originalProject->group->rootPath);

    writelnf(f, " 	<ProjectOutputPath>%s\\</ProjectOutputPath>", project->outputPath.u8string().c_str());
    writelnf(f, " 	<ProjectGeneratedPath>%s\\</ProjectGeneratedPath>", project->generatedPath.u8string().c_str());
    writelnf(f, " 	<ProjectPublishPath>%s\\</ProjectPublishPath>", m_config.deployPath.u8string().c_str());
    writelnf(f, " 	<ProjectSourceRoot>%s\\</ProjectSourceRoot>", project->originalProject->rootPath.u8string().c_str());
    //writelnf(f, " 	<ProjectPathName>%s</ProjectPathName>", relativeSourceDir.u8string().c_str());

    if (project->originalProject->flagWarn3)
        writeln(f, "     <ProjectWarningLevel>Level3</ProjectWarningLevel>");

    if (m_buildWithLibs)
        writeln(f, " <SolutionType>StaticLibraries</SolutionType>");
    else
        writeln(f, " <SolutionType>SharedLibraries</SolutionType>");

    if (project->originalProject->type == ProjectType::LocalApplication)
    {
        if (project->originalProject->flagConsole)
            writeln(f, " 	<ModuleType>ConsoleApp</ModuleType>");
        else
            writeln(f, " 	<ModuleType>App</ModuleType>");
    }
    else if (project->originalProject->type == ProjectType::LocalLibrary)
    {
        if (project->originalProject->flagForceSharedLibrary)
            writeln(f, " 	<ModuleType>DynamicLibrary</ModuleType>");
        else if (project->originalProject->flagForceStaticLibrary)
            writeln(f, " 	<ModuleType>StaticLibrary</ModuleType>");
        else
        {
            if (m_buildWithLibs)
                writeln(f, " 	<ModuleType>StaticLibrary</ModuleType>");
            else
                writeln(f, " 	<ModuleType>DynamicLibrary</ModuleType>");
        }
    }

    f << " 	<ProjectPreprocessorDefines>$(ProjectPreprocessorDefines);";

    f << ToUpper(project->mergedName) << "_EXPORTS;";
    f << "PROJECT_NAME=" << project->mergedName << ";";

    if (project->originalProject->flagConsole)
        f  << "CONSOLE;";

    if (project->hasReflection)
        f << "BUILD_WITH_REFLECTION;";

    for (const auto* dep : project->allDependencies)
        f << "HAS_" << ToUpper(dep->mergedName) << ";";

    f << "</ProjectPreprocessorDefines>\n";

    writelnf(f, " 	<ProjectGuid>%s</ProjectGuid>", project->assignedVSGuid.c_str());
    writeln(f, "</PropertyGroup>");

    writelnf(f, "<Import Project=\"%s\\SharedItemGroups.props\"/>", m_visualStudioScriptsPath.u8string().c_str());

    /*long numAssemblyFiles = this.files.stream().filter(pf->pf.type == FileType.ASSEMBLY).count();
    if (numAssemblyFiles > 0) {
        writeln(f, "<ImportGroup Label=\"ExtensionSettings\" >");
        writeln(f, "  <Import Project=\"$(VCTargetsPath)\\BuildCustomizations\\masm.props\" />");
        writeln(f, "</ImportGroup>");
    }*/

    writeln(f, "<ItemGroup>");
    for (const auto* pf : project->files)
        generateSourcesProjectFileEntry(project, pf, f);
    writeln(f, "</ItemGroup>");

    writeln(f, "<ItemGroup>");
    if (m_buildWithLibs && (project->originalProject->type == ProjectType::LocalApplication))
    {
        for (const auto* dep : project->allDependencies)
        {
            auto projectFilePath = dep->projectPath / dep->mergedName;
            projectFilePath += ".vcxproj";

            writelnf(f, " <ProjectReference Include=\"%s\">", projectFilePath.u8string().c_str());
            writelnf(f, "   <Project>%s</Project>", dep->assignedVSGuid.c_str());
            writeln(f, " </ProjectReference>");
        }
    }
    else
    {
        for (const auto* dep : project->directDependencies)
        {
            // when building static libs they are linked together at the end, we don't have to keep track of local dependencies
            if (m_buildWithLibs && dep->originalProject->type != ProjectType::RttiGenerator)
                continue;

            auto projectFilePath = dep->projectPath / dep->mergedName;
            projectFilePath += ".vcxproj";

            writelnf(f, " <ProjectReference Include=\"%s\">", projectFilePath.u8string().c_str());
            writelnf(f, "   <Project>%s</Project>", dep->assignedVSGuid.c_str());
            writeln(f, " </ProjectReference>");
        }
    }
    writeln(f, "</ItemGroup>");

    writelnf(f, " <Import Project=\"%s\\Shared.targets\"/>", m_visualStudioScriptsPath.u8string().c_str());
    writeln(f, " <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\"/>");

    /*if (numAssemblyFiles > 0) {
        writeln(f, "<ImportGroup Label=\"ExtensionTargets\">");
        writeln(f, "  <Import Project=\"$(VCTargetsPath)\\BuildCustomizations\\masm.targets\" />");
        writeln(f, "</ImportGroup>");
    }*/

    writeln(f, "</Project>");

    return true;
}

bool SolutionGeneratorVS::generateSourcesProjectFileEntry(const CodeGenerator::GeneratedProject* project, const CodeGenerator::GeneratedProjectFile* file, std::stringstream& f) const
{
    switch (file->type)
    {
        case ProjectFileType::CppHeader:
        {
            writelnf(f, "   <ClInclude Include=\"%s\">", file->absolutePath.u8string().c_str());
            if (!file->useInCurrentBuild)
                writeln(f, "   <ExcludedFromBuild>true</ExcludedFromBuild>");
            writeln(f, "   </ClInclude>");
            break;
        }

        case ProjectFileType::CppSource:
        {
            writelnf(f, "   <ClCompile Include=\"%s\">", file->absolutePath.u8string().c_str());

            if (file->name == "build.cpp")
                writeln(f, "      <PrecompiledHeader>Create</PrecompiledHeader>");
            else if (!file->originalFile || !file->originalFile->flagUsePch)
                writeln(f, "      <PrecompiledHeader>NotUsing</PrecompiledHeader>");

            if (!file->useInCurrentBuild)
                writeln(f, "   <ExcludedFromBuild>true</ExcludedFromBuild>");

            writeln(f, "   </ClCompile>");
            break;
        }

        case ProjectFileType::Bison:
        {
            writelnf(f, "   <None Include=\"%s\">", file->absolutePath.u8string().c_str());
            writelnf(f, "      <SubType>Bison</SubType>");
            writeln(f, "   </None>");
            break;
        }

        case ProjectFileType::WindowsResources:
        {
            writelnf(f, "   <ResourceCompile Include=\"%s\"/>", file->absolutePath.u8string().c_str());
            break;
        }

        case ProjectFileType::BuildScript:
        {
            writelnf(f, "   <None Include=\"%s\"/>", file->absolutePath.u8string().c_str());
            break;
        }

        /*case NATVIS: {
            f.writelnf("   <Content Include=\"%s\">", pf.absolutePath);
            f.writelnf("      <IncludeInVSIX>true</IncludeInVSIX>");
            f.writelnf("   </Content>");
            break;
        }

        case VSIXMANIFEST:
        {
            f.writelnf("   <None Include=\"%s\">", pf.absolutePath);
            f.writelnf("      <SubType>Designer</SubType>");
            f.writelnf("   </None>");
            break;
        }*/

    }

    return true;
}

static void AddFilterSection(std::string_view path, std::vector<std::string>& outFilterSections)
{
    if (outFilterSections.end() == find(outFilterSections.begin(), outFilterSections.end(), path))
        outFilterSections.push_back(std::string(path));
}

static void AddFilterSectionRecursive(std::string_view path, std::vector<std::string>& outFilterSections)
{
    auto pos = path.find_last_of('\\');
    if (pos != -1)
    {
        auto left = path.substr(0, pos);
        AddFilterSectionRecursive(left, outFilterSections);
    }

    AddFilterSection(path, outFilterSections);
    
}

bool SolutionGeneratorVS::generateSourcesProjectFilters(const CodeGenerator::GeneratedProject* project, std::stringstream& f) const
{
    writeln(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    writeln(f, "<!-- Auto generated file, please do not edit -->");
    writeln(f, "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">");

    std::vector<std::string> filterSections;

    // file entries
    {
        writeln(f, "<ItemGroup>");
        for (const auto* file : project->files)
        {
            const char* filterType = "None";

            switch (file->type)
            {
                case ProjectFileType::CppHeader:
                    filterType = "ClInclude";
                    break;
                case ProjectFileType::CppSource:
                    filterType = "ClCompile";
                    break;
            }

            if (file->filterPath.empty())
            {
                writelnf(f, "  <%s Include=\"%s\"/>", filterType, file->absolutePath.u8string().c_str());
            }
            else
            {
                writelnf(f, "  <%s Include=\"%s\">", filterType, file->absolutePath.u8string().c_str());
                writelnf(f, "    <Filter>%s</Filter>", file->filterPath.c_str());
                writelnf(f, "  </%s>", filterType);

                AddFilterSectionRecursive(file->filterPath, filterSections);
            }
        }           

        writeln(f, "</ItemGroup>");
    }

    // filter section
    {
        writeln(f, "<ItemGroup>");

        for (const auto& section : filterSections)
        {
            const auto guid = GuidFromText(section);

            writelnf(f, "  <Filter Include=\"%s\">", section.c_str());
            writelnf(f, "    <UniqueIdentifier>%s</UniqueIdentifier>", guid.c_str());
            writelnf(f, "  </Filter>");
        }

        writeln(f, "</ItemGroup>");
    }

    writeln(f, "</Project>");

    return true;
}

bool SolutionGeneratorVS::generateRTTIGenProjectFile(const CodeGenerator::GeneratedProject* project, std::stringstream& f) const
{
    writeln(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    writeln(f, "<!-- Auto generated file, please do not edit -->");

    writelnf(f, "<Project DefaultTargets=\"Build\" ToolsVersion=\"%s\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">", VS_PROJECT_VERSION);

    writelnf(f, "<Import Project=\"%s\\SharedConfigurationSetup.props\"/>", m_visualStudioScriptsPath.u8string().c_str());

    writeln(f,  "<PropertyGroup>");
    writelnf(f, "  <PlatformToolset>%s</PlatformToolset>", VS_TOOLSET_VERSION);
    writelnf(f, "  <VCProjectVersion>%s</VCProjectVersion>", VS_PROJECT_VERSION);
    writeln(f,  "  <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>");
    writeln(f,  "  <ModuleType>Empty</ModuleType>");
    writeln(f,   " <SolutionType>SharedLibraries</SolutionType>");
    writelnf(f, "  <ProjectGuid>%s</ProjectGuid>", project->assignedVSGuid.c_str());
    writeln(f,  "  <DisableFastUpToDateCheck>true</DisableFastUpToDateCheck>");
    writeln(f,  "</PropertyGroup>");
    writeln(f,  "  <PropertyGroup>");
    writeln(f,  "    <PreBuildEventUseInBuild>true</PreBuildEventUseInBuild>");
    writeln(f,  "  </PropertyGroup>");
    writeln(f,  "  <ItemDefinitionGroup>");
    writeln(f,  "    <PreBuildEvent>");

    {
        std::stringstream cmd;
        f << "      <Command>";
        f << m_config.builderExecutablePath.u8string() << " ";
        f << "-tool=reflection ";
        if (!m_config.engineRootPath.empty())
            f << "-engineDir=" << m_config.engineRootPath.u8string() << " ";
        if (!m_config.projectRootPath.empty())
            f << "-projectDir=" << m_config.projectRootPath.u8string() << " ";
        f << "-build=" << NameEnumOption(m_config.build) << " ";
        f << "-config=" << NameEnumOption(m_config.configuration) << " ";
        f << "-platform=" << NameEnumOption(m_config.platform) << " ";
        f << "-libs=" << NameEnumOption(m_config.libs) << " ";
        f << "</Command>\n";
    }

    writeln(f, "    </PreBuildEvent>");
    writeln(f, "  </ItemDefinitionGroup>");
    writelnf(f, "<Import Project=\"%s\\SharedItemGroups.props\"/>", m_visualStudioScriptsPath.u8string().c_str());
    writeln(f, "  <ItemGroup>");
    writeln(f, "  </ItemGroup>");
    writeln(f, "  <ItemGroup>");
    writeln(f, "  </ItemGroup>");
    writelnf(f, " <Import Project=\"%s\\Shared.targets\"/>", m_visualStudioScriptsPath.u8string().c_str());
    writeln(f, " <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\"/>");
    writeln(f, "</Project>");

    return true;
}

#if 0
// copy files
for (Config.DeployFile path : setup.getDeployFiles()) {
    if (Files.exists(path.sourcePath)) {
        Path publishPath = solutionSetup.rootPublishPath.resolve(String.format("%s", configName.toLowerCase()));
        Path targetPath = publishPath.resolve(path.targetPath);
        if (GeneratedFile.ShouldCopyFile(path.sourcePath, targetPath)) {
            System.out.printf("Copying file \"%s\" to \"%s\"\n", path.sourcePath, targetPath);
            try {
                Files.createDirectories(targetPath.getParent());
                Files.copy(path.sourcePath, targetPath, StandardCopyOption.REPLACE_EXISTING);
                Files.setLastModifiedTime(targetPath, Files.getLastModifiedTime(path.sourcePath));
            }
            catch (IOException e) {
                System.err.printf("Failed to copy '%s' referenced by library '%s' into '%s'\n", path.sourcePath, lib.name, targetPath);
            }
        }
    }
    else {
        System.err.printf("Deployed file '%s' referenced by library '%s' manifest does not exist\n", path.sourcePath, lib.name);
    }
#endif

#if 0
private void generateVSIXManifestFile(GeneratedFile f) {
    writeln(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    writeln(f, "<PackageManifest Version=\"2.0.0\" xmlns=\"http://schemas.microsoft.com/developer/vsx-schema/2011\" xmlns:d=\"http://schemas.microsoft.com/developer/vsx-schema-design/2011\">");
    writeln(f, "<Metadata>");
    writeln(f, "<Identity Id=\"NatvisSample..e72aa83c-bb6f-4910-8e9f-793052cd112a\" Version=\"1.0\" Language=\"en-US\" Publisher=\"MS\" />");
    writeln(f, "<DisplayName>NatvisSample</DisplayName>");
    writeln(f, "<Description>Sample project that demonstrates how to package type visualizers in a VSIX package</Description>");
    writeln(f, "</Metadata>");
    writeln(f, "<Installation>");
    writeln(f, "<InstallationTarget Id=\"Microsoft.VisualStudio.Pro\" Version=\"11.0\" />");
    writeln(f, "</Installation>");
    writeln(f, "<Dependencies>");
    writeln(f, "<Dependency Id=\"Microsoft.Framework.NDP\" DisplayName=\"Microsoft .NET Framework\" d:Source=\"Manual\" Version=\"4.5\"/>");
    writeln(f, "</Dependencies>");
    writeln(f, "<Assets>");

    this.files.stream()
        .filter(pf->pf.type == FileType.NATVIS)
        .forEach(pf->f.writelnf("<Asset Type=\"NativeVisualizer\" Path=\"%s\"/>", pf.absolutePath));

    writeln(f, "</Assets>");
    writeln(f, "</PackageManifest>");
}

private void generateProjectMainFile(GeneratedFilesCollection files) {
    GeneratedFile f = files.createFile(localProjectFilePath);


}


private static Stream<Path> decompilePathIntoSubPaths(Path path) {
    if (path == null) {
        return Stream.empty();
    }
    else {
        return Stream.concat(decompilePathIntoSubPaths(path.getParent()), Stream.of(path));
    }
}

private List<Path> generateUniqueFilterPaths(List<File> files)
{
    // get all (unique) filter paths nicely in a set
    return new ArrayList<>(files.stream()
        .map(file->file.attributes.value("filter")) // we are only interested in files with the filter attribute
        .map(attr->Paths.get(attr)) // convert the std::string to a relative path
        .flatMap(path->decompilePathIntoSubPaths(path)) // convert a single path into pieces
        .sorted(Comparator.comparingInt(a->a.getNameCount())) // sort all path pieces so the smaller paths comes first
        .collect(Collectors.toCollection(() -> new LinkedHashSet<>())));
}


@Override
protected void generateProjectLibrariesLinkage(boolean hasStaticInitialization, GeneratedFile f) {
    // get all public and internal libraries that this project is using
    // NOTE: this does not include private libraries from another projects (they are needed only if we build something that must be self-contained)
    List<Library> libs = collectLibraries("internal");
    if (!libs.isEmpty()) {
        writeln(f, "#ifndef BUILD_AS_LIBS");

        // generate the libraries linkage
        // this adds the required #pragma comment lib
        generateProjectLibrariesLinkage(libs, f);

        writeln(f, "#endif");
        writeln(f, "");
    }

    // app only linkage
    if (hasStaticInitialization)
    {
        // we are trying to build self-contained module, need to initialize the private libraries
        // from all our dependencies as well
        List<Library> allLibs = collectLibraries("all");
        allLibs.removeAll(libs);

        if (!allLibs.isEmpty()) {
            writeln(f, "#ifdef BUILD_AS_LIBS");

            // do not link libs that were already linked
            generateProjectLibrariesLinkage(allLibs, f);

            writeln(f, "#endif");
            writeln(f, "");
        }
    }
}

private void generateProjectLibrariesLinkage(List<Library> libs, GeneratedFile f) {
    for (String config : solutionSetup.allConfigs)
        for (String platform : solutionSetup.allPlatforms)
            for (Library l : libs)
                generateProjectLibraryLinkage(l, platform, config, f);
}

private void generateProjectLibraryLinkage(Library lib, String platformName, String configName, GeneratedFile f) {
    // get the library setup for given platform and config
    Config.MergedState setup = lib.getSetup(platformName, configName);

    // do the #pragma lib for all the library paths
    // NOTE: in normal makefile we would have to end until the link phase with this
    if (!setup.getLibPaths().isEmpty())
    {
        f.writelnf("// linkage from library %s", lib.name);
        f.writelnf("#if defined(BUILD_%s) && defined(PLATFORM_%s)", configName.toUpperCase(), platformName.toUpperCase());

        for (Path path : setup.getLibPaths())
            f.writelnf("    #pragma comment( lib, \"%s\")", path.toString().replaceAll("\\\\", "\\\\\\\\"));

        f.writelnf("#endif");
    }
}

private void generateProjectLibrariesIncludes(GeneratedFile f) {
    VsSolutionSetup solution = (VsSolutionSetup)solutionSetup;

    // for every platform/config generate the list of libraries to include
    for (String configName : solutionSetup.allConfigs) {
        for (String platformName : solutionSetup.allPlatforms) {
            for (Library lib : collectLibraries("internal")) {

                // get the library setup for given platform and config
                Config.MergedState setup = lib.getSetup(platformName, configName);

                // emit the library include paths into the project
                for (Path path : setup.getIncludePaths()) {
                    f.writelnf("	<LibraryIncludePath Condition=\"'$(Platform)' == '%s' and '$(Configuration)'=='%s'\">$(LibraryIncludePath);%s\\</LibraryIncludePath>", platformName, configName, path);
                }

                // copy files
                for (Config.DeployFile path : setup.getDeployFiles()) {
                    if (Files.exists(path.sourcePath)) {
                        Path publishPath = solutionSetup.rootPublishPath.resolve(String.format("%s", configName.toLowerCase()));
                        Path targetPath = publishPath.resolve(path.targetPath);
                        if (GeneratedFile.ShouldCopyFile(path.sourcePath, targetPath)) {
                            System.out.printf("Copying file \"%s\" to \"%s\"\n", path.sourcePath, targetPath);
                            try {
                                Files.createDirectories(targetPath.getParent());
                                Files.copy(path.sourcePath, targetPath, StandardCopyOption.REPLACE_EXISTING);
                                Files.setLastModifiedTime(targetPath, Files.getLastModifiedTime(path.sourcePath));
                            }
                            catch (IOException e) {
                                System.err.printf("Failed to copy '%s' referenced by library '%s' into '%s'\n", path.sourcePath, lib.name, targetPath);
                            }
                        }
                    }
                    else {
                        System.err.printf("Deployed file '%s' referenced by library '%s' manifest does not exist\n", path.sourcePath, lib.name);
                    }
                }
            }
        }
    }
}

#endif