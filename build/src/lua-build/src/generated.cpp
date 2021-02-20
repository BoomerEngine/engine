#include "common.h"
#include "project.h"
#include "generated.h"

CodeGenerator::CodeGenerator(const Configuration& config)
    : config(config)
{
    useStaticLinking = (config.build == BuildType::Standalone);

    rootGroup = new GeneratedGroup;
    rootGroup->name = "BoomerEngine";
    rootGroup->mergedName = "BoomerEngine";
    rootGroup->assignedVSGuid = GuidFromText(rootGroup->mergedName);

    sharedGlueFolder = config.solutionPath / "generated" / "_shared";
}

struct OrderedGraphBuilder
{
    unordered_map<CodeGenerator::GeneratedProject*, int> depthMap;
    

    bool insertProject(CodeGenerator::GeneratedProject* p, int depth, vector<CodeGenerator::GeneratedProject*>& stack)
    {
        if (find(stack.begin(), stack.end(), p) != stack.end())
        {
            cout << "Recursive project dependencies found when project '" << p->mergedName << "' was encountered second time\n";
            for (const auto* proj : stack)
                cout << "  Reachable from '" << proj->mergedName << "'\n";
            return false;
        }

        auto currentDepth = depthMap[p];
        if (depth > currentDepth)
        {
            depthMap[p] = depth;

            stack.push_back(p);

            for (auto* dep : p->directDependencies)
                if (!insertProject(dep, depth + 1, stack))
                    return false;

            stack.pop_back();
        }

        return true;
    }

    void extractOrderedList(vector<CodeGenerator::GeneratedProject*>& outList) const
    {
        vector<pair<CodeGenerator::GeneratedProject*, int>> pairs;
        copy(depthMap.begin(), depthMap.end(), std::back_inserter(pairs));
        sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) -> bool { 
            if (a.second != b.second)
                return a.second > b.second;
            return a.first->mergedName < b.first->mergedName;
            });
        
        for (const auto& pair : pairs)
            outList.push_back(pair.first);
    }
};

CodeGenerator::GeneratedGroup* CodeGenerator::findOrCreateGroup(string_view name, GeneratedGroup* parent)
{
    for (auto* group : parent->children)
        if (group->name == name)
            return group;

    auto* group = new GeneratedGroup;
    group->name = name;
    group->mergedName = parent->mergedName + "." + string(name);
    group->parent = parent;
    group->assignedVSGuid = GuidFromText(group->mergedName);
    parent->children.push_back(group);
    return group;
}

CodeGenerator::GeneratedGroup* CodeGenerator::createGroup(string_view name)
{
    vector<string_view> parts;
    SplitString(name, ".", parts);

    auto* cur = rootGroup;
    for (const auto& part : parts)
        cur = findOrCreateGroup(part, cur);

    return cur;
}

bool CodeGenerator::shouldUseFile(const ProjectStructure::FileInfo* file) const
{
    if (file->flagExcluded)
        return false;

    switch (file->filter)
    {
    case ProjectFilePlatformFilter::Any:
        return true;
    case ProjectFilePlatformFilter::WinApi:
        return (config.platform == PlatformType::Windows) || (config.platform == PlatformType::UWP);
    case ProjectFilePlatformFilter::POSIX:
        return (config.platform == PlatformType::Linux);
    case ProjectFilePlatformFilter::Windows:
        return (config.platform == PlatformType::Windows);
    case ProjectFilePlatformFilter::Linux:
        return (config.platform == PlatformType::Linux);
    }

    return false;
}

bool CodeGenerator::extractProjects(const ProjectStructure& structure)
{
    // create projects
    for (const auto* proj : structure.projects)
    {
        if (proj->type == ProjectType::LocalApplication || proj->type == ProjectType::LocalLibrary || proj->type == ProjectType::RttiGenerator)
        {
            // do not include dev-only project in standalone builds
            if (config.build != BuildType::Development)
            {
                if (proj->flagDevOnly)
                    continue;
            }

            // create wrapper
            auto* generatorProject = new GeneratedProject;
            generatorProject->mergedName = proj->mergedName;
            generatorProject->originalProject = proj;
            generatorProject->generatedPath = config.solutionPath / "generated" / proj->mergedName;
            generatorProject->projectPath = config.solutionPath / "projects" / proj->mergedName;
            generatorProject->outputPath = config.solutionPath / "output" / proj->mergedName;

            projects.push_back(generatorProject);
            projectsMap[proj] = generatorProject;

            // create file wrappers
            for (const auto* file : proj->files)
            {
                auto* info = new GeneratedProjectFile;
                info->absolutePath = file->absolutePath;
                info->filterPath = filesystem::relative(file->absolutePath.parent_path(), proj->rootPath).u8string();                
                if (info->filterPath == ".")
                    info->filterPath.clear();
                info->name = file->name;
                info->originalFile = file;
                info->type = file->type;
                info->useInCurrentBuild = shouldUseFile(file);
                generatorProject->files.push_back(info);
            }

            // add project to group
            generatorProject->group = createGroup(proj->type == ProjectType::RttiGenerator ? "" : PartBefore(proj->mergedName, "_"));
            generatorProject->group->projects.push_back(generatorProject);

            // determine project guid
            generatorProject->assignedVSGuid = GuidFromText(generatorProject->mergedName);
        }
    }

    // map dependencies
    for (auto* proj : projects)
    {
        for (const auto* dep : proj->originalProject->resolvedDependencies)
        {
            auto it = projectsMap.find(dep);
            if (it != projectsMap.end())
                proj->directDependencies.push_back(it->second);
        }
    }

    // build merged dependencies
    bool validDeps = true;
    for (auto* proj : projects)
    {
        vector<CodeGenerator::GeneratedProject*> stack;
        stack.push_back(proj);

        OrderedGraphBuilder graph;
        for (auto* dep : proj->directDependencies)
            validDeps &= graph.insertProject(dep, 1, stack);
        graph.extractOrderedList(proj->allDependencies);
    }

    // build final project list
    {
        auto temp = std::move(projects);

        vector<CodeGenerator::GeneratedProject*> stack;

        OrderedGraphBuilder graph;
        for (auto* proj : temp)
            validDeps &= graph.insertProject(proj, 1, stack);

        projects.clear();
        graph.extractOrderedList(projects);
    }

    // determine what should be generated
    for (auto* proj : projects)
    {
        proj->localGlueHeader = sharedGlueFolder / proj->mergedName;
        proj->localGlueHeader += "_glue.inl";

        const auto publicHeader = proj->originalProject->rootPath / "include/public.h";
        if (filesystem::is_regular_file(publicHeader))
            proj->localPublicHeader = publicHeader;
    }

    // extract base include directories (source code roots)
    for (const auto* group : structure.groups)
        sourceRoots.push_back(group->rootPath);

    return validDeps;
}

//--

CodeGenerator::GeneratedFile* CodeGenerator::createFile(const filesystem::path& path)
{
    auto file = new GeneratedFile(path);
    files.push_back(file);
    return file;
}

bool CodeGenerator::saveFiles()
{
    bool valid = true;

    uint32_t numSavedFiles = 0;
    for (const auto* file : files)
        valid &= SaveFileFromString(file->absolutePath, file->content.str(), false, &numSavedFiles);

    cout << "Saved " << numSavedFiles << " files\n";

    if (!valid)
    {
        cout << "Failed to save some output files, generated solution may not be valid\n";
        return false;
    }

    return true;
}

//--

bool CodeGenerator::generateAutomaticCode()
{
    bool valid = true;

    for (auto* proj : projects)
        valid &= generateAutomaticCodeForProject(proj);

    return valid;
}

bool CodeGenerator::generateExtraCode()
{
    bool valid = true;

    for (auto* proj : projects)
        valid &= generateExtraCodeForProject(proj);

    return valid;    
}

bool CodeGenerator::generateAutomaticCodeForProject(GeneratedProject* project)
{
    bool valid = true;

    if (!project->localGlueHeader.empty())
    {
        auto* info = new GeneratedProjectFile;
        info->absolutePath = project->localGlueHeader;
        info->type = ProjectFileType::CppHeader;
        info->filterPath = "_generated";
        info->name = project->mergedName + "_glue.inl";
        info->generatedFile = createFile(info->absolutePath);
        project->files.push_back(info);

        valid &= generateProjectGlueFile(project, info->generatedFile->content);
    }

    if (project->originalProject->type == ProjectType::LocalApplication && project->originalProject->flagGenerateMain)
    {
        auto* info = new GeneratedProjectFile;
        info->absolutePath = project->generatedPath / "main.cpp";
        info->type = ProjectFileType::CppSource;
        info->filterPath = "_generated";
        info->name = "main.cpp";
        info->generatedFile = createFile(info->absolutePath);
        project->files.push_back(info);

        valid &= generateProjectAutoMainFile(project, info->generatedFile->content);
    }

    if (project->originalProject->type == ProjectType::LocalApplication || project->originalProject->type == ProjectType::LocalLibrary)
    {
        auto* info = new GeneratedProjectFile;
        info->absolutePath = project->generatedPath / "static_init.inl";
        info->type = ProjectFileType::CppHeader;
        info->filterPath = "_generated";
        info->name = "main.cpp";
        info->generatedFile = createFile(info->absolutePath);
        project->files.push_back(info);

        valid &= generateProjectStaticInitFile(project, info->generatedFile->content);
    }

    if (project->originalProject->type == ProjectType::LocalApplication || project->originalProject->type == ProjectType::LocalLibrary)
    {
        auto* info = new GeneratedProjectFile;
        info->absolutePath = project->generatedPath / "reflection.inl";
        info->type = ProjectFileType::CppHeader;
        info->filterPath = "_generated";
        info->name = "main.cpp";
        info->generatedFile = createFile(info->absolutePath);
        project->files.push_back(info);

        valid &= generateProjectDefaultReflection(project, info->generatedFile->content);
    }        

    return valid;
}

bool CodeGenerator::generateExtraCodeForProject(GeneratedProject* project)
{
    bool valid = true;

    for (size_t i = 0; i < project->files.size(); ++i)
    {
        const auto* file = project->files[i];

        if (file->type == ProjectFileType::Bison)
            valid &= processBisonFile(project, file);
    }

    return valid;
}

bool CodeGenerator::generateProjectGlueFile(const GeneratedProject* project, stringstream& f)
{
    auto macroName = ToUpper(project->mergedName) + "_GLUE";
    auto apiName = ToUpper(project->mergedName) + "_API";
    auto exportsMacroName = ToUpper(project->mergedName) + "_EXPORTS";

    writeln(f, "/***");
    writeln(f, "* Boomer Engine Glue Code");
    writeln(f, "* Build system source code licensed under MIP license");
    writeln(f, "* Auto generated, do not modify");
    writeln(f, "***/");
    writeln(f, "");    writeln(f, "");
    writeln(f, "#ifndef " + macroName);
    writeln(f, "#define " + macroName);
    writeln(f, "");

    if (useStaticLinking)
    {
        writeln(f, "#define " + apiName);
    }
    else
    {
        writeln(f, "  #ifdef " + exportsMacroName);
        writelnf(f, "    #define %s __declspec( dllexport )", apiName.c_str());
        writeln(f, "  #else");
        writelnf(f, "    #define %s __declspec( dllimport )", apiName.c_str());
        writeln(f, "  #endif");
    }

    if (!project->directDependencies.empty())
    {
        writeln(f, "");
        writeln(f, "// Public header from project dependencies:");

        for (const auto* dep : project->directDependencies)
            if (!dep->localPublicHeader.empty())
                writeln(f, "#include \"" + dep->localPublicHeader.u8string() + "\"");
    }

    writeln(f, "#endif");

    return true;
}

bool CodeGenerator::generateProjectDefaultReflection(const GeneratedProject* project, stringstream& f)
{
    writeln(f, "/// Boomer Engine v4 by Tomasz \"RexDex\" Jonarski");
    writeln(f, "/// RTTI Glue Code Generator is under MIT License");
    writeln(f, "");
    writeln(f, "/// AUTOGENERATED FILE - ALL EDITS WILL BE LOST");
    return true;
}
        
bool CodeGenerator::projectRequiresStaticInit(const GeneratedProject* project) const
{
    if (project->originalProject->flagNoInit)
        return false;

    if (project->originalProject->type != ProjectType::LocalLibrary)
        return false;

    for (const auto* dep : project->allDependencies)
        if (dep->mergedName == "base_system")
            return true;

    return project->mergedName == "base_system";
}

bool CodeGenerator::generateProjectStaticInitFile(const GeneratedProject* project, stringstream& f)
{
    writeln(f, "/***");
    writeln(f, "* Boomer Engine Static Lib Initialization Code");
    writeln(f, "* Auto generated, do not modify");
    writeln(f, "* Build system source code licensed under MIP license");
    writeln(f, "***/");
    writeln(f, "");

    // determine if project requires static initialization (the apps and console apps require that)
    // then pull in the library linkage, for apps we pull much more crap
    if (config.generator == GeneratorType::VisualStudio)
    {
        if (!useStaticLinking)
        {
            for (const auto* dep : project->originalProject->resolvedDependencies)
            {
                if (dep->type == ProjectType::ExternalLibrary)
                {
                    for (const auto& linkPath : dep->libraryLinkFile)
                    {
                        stringstream ss;
                        ss << linkPath;
                        writelnf(f, "#pragma comment( lib, %s )", ss.str().c_str());
                    }
                }
            }
        }
        else if (project->originalProject->type == ProjectType::LocalApplication)
        {
            unordered_set<const ProjectStructure::ProjectInfo*> exportedLibs;

            for (const auto* proj : project->allDependencies)
            {
                for (const auto* dep : proj->originalProject->resolvedDependencies)
                {
                    if (dep->type == ProjectType::ExternalLibrary)
                    {
                        if (exportedLibs.insert(dep).second)
                        {
                            for (const auto& linkPath : dep->libraryLinkFile)
                            {
                                stringstream ss;
                                ss << linkPath;
                                writelnf(f, "#pragma comment( lib, %s)", ss.str().c_str());
                            }
                        }
                    }
                }
            }
        }
    }

    // static initialization part is only generated for apps
    if (project->originalProject->type == ProjectType::LocalApplication)
    {
        writeln(f, "void InitializeStaticDependencies()");
        writeln(f, "{");

        if (!project->allDependencies.empty())
        {
            if (useStaticLinking)
            {
                // if we build based on static libraries we need to "touch" the initialization code from other modules
                for (const auto* dep : project->allDependencies)
                {
                    if (projectRequiresStaticInit(dep))
                    {
                        writelnf(f, "    extern void InitModule_%s();", dep->mergedName.c_str());
                        writelnf(f, "    InitModule_%s();", dep->mergedName.c_str());
                    }
                };
            }
            else
            {
                // if we are build based on dynamic libs (dlls)
                // make sure that they are loaded on time
                // THIS IS TEMPORARY UNTIL WE DO A PROPER CLASS LOADER
                for (const auto* dep : project->allDependencies)
                {
                    if (dep->originalProject->type == ProjectType::LocalLibrary)
                    {
                        writelnf(f, "    base::modules::LoadDynamicModule(\"%s\");", dep->mergedName.c_str());
                    }
                }
            }
        }

        // initialize ourselves
        writelnf(f, "    extern void InitModule_%s();", project->mergedName.c_str());
        writelnf(f, "    InitModule_%s();", project->mergedName.c_str());

        writeln(f, "}");
    }

    return true;
}

bool CodeGenerator::generateProjectAutoMainFile(const GeneratedProject* project, stringstream& f)
{
    auto isConsole = project->originalProject->flagConsole;

    writeln(f, "/***");
    writeln(f, "* Boomer Engine Entry Point");
    writeln(f, "* Auto generated, do not modify");
    writeln(f, "* Build system source code licensed under MIP license");
    writeln(f, "***/");

    writeln(f, "");
    writeln(f, "#include \"build.h\"");
    writeln(f, "#include \"static_init.inl\"");
    writeln(f, "#include \"base/app/include/launcherMacros.h\"");
    writeln(f, "#include \"base/app/include/commandline.h\"");
    writeln(f, "#include \"base/app/include/launcherPlatform.h\"");
    writeln(f, "#include \"base/app/include/application.h\"");

    writeln(f, "");
    writeln(f, "void* GCurrentModuleHandle = nullptr;");
    writeln(f, "");

    //if (!noApp) {
        writeln(f, "");
        writeln(f, "extern base::app::IApplication& GetApplicationInstance();");
        writeln(f, "");
    //}

    writeln(f, "#define MACRO_TXT2(x) #x");
    writeln(f, "#define MACRO_TXT(x) MACRO_TXT2(x)");
    writeln(f, "");

    writeln(f, "LAUNCHER_MAIN()");
    writeln(f, "{");
    writeln(f, "      // initialize all static modules used by this application");
    writeln(f, "      GCurrentModuleHandle = LAUNCHER_APP_HANDLE();");
    writeln(f, "      InitializeStaticDependencies();");
    writeln(f, "");
    writeln(f, "      // parse system command line");
    writeln(f, "      base::app::CommandLine cmdline;");
    writeln(f, "      if (!LAUNCHER_PARSE_CMDLINE())");
    writeln(f, "        return -1;");
    writeln(f, "");

    /*if (noApp)
       writeln(f, "      cmdline.param(\"noapp\", nullptr);");*/

    if (isConsole)
        writeln(f, "      cmdline.param(\"console\", nullptr);");

    /*if (isEditor)
        writeln(f, "      cmdline.param(\"editor\", nullptr);");*/

    /*if (attributes.hasKey("noscripts"))
        writeln(f, "      cmdline.param(\"noscripts\", nullptr);");*/

    //writeln(f, "      #ifdef MODULE_NAME");
    //writeln(f, "        cmdline.addParam(\"moduleName\", MACRO_TXT(MODULE_NAME));");
    //writeln(f, "      #endif");
    //writeln(f, "");
    //writeln(f, "      auto& app = base::platform::GetLaunchPlatform();");
    //writeln(f, "      return app.run(cmdline);");

    writeln(f, "      // initialize app");
    writeln(f, "      auto& platform = base::platform::GetLaunchPlatform();");
    /*if (noApp)
        writeln(f, "      if (!platform.platformStart(cmdline, nullptr))");
    else*/
        writeln(f, "      if (!platform.platformStart(cmdline, &GetApplicationInstance()))");
    writeln(f, "        return -1; // failed to initialize");
    writeln(f, "");

    {
        writeln(f, "      // main loop");
        writeln(f, "      while (platform.platformUpdate()) { platform.platformIdle(); };");
        writeln(f, "");
    }

    writeln(f, "      // cleanup");
    writeln(f, "      cmdline = base::app::CommandLine(); // prevent leaks past app.cleanup()");
    writeln(f, "      platform.platformCleanup();");

    writeln(f, "      return 0;");
    writeln(f, "    }");

    return true;
}

//--

bool CodeGenerator::processBisonFile(GeneratedProject* project, const GeneratedProjectFile* file)
{
    auto* tool = project->originalProject->findToolByName("bison");
    if (!tool)
    {
        cout << "BISON library not linked by current project\n";
        return false;
    }

    const auto coreName = PartBefore(file->name, ".");

    string parserFileName = string(coreName) + "_Parser.cpp";
    string symbolsFileName = string(coreName) + "_Symbols.h";
    string reportFileName = string(coreName) + "_Report.txt";

    filesystem::path parserFile = project->generatedPath / parserFileName;
    filesystem::path symbolsFile = project->generatedPath / symbolsFileName;
    filesystem::path reportPath = project->generatedPath / reportFileName;

    if (!IsFileSourceNewer(file->absolutePath, parserFile))
        return true;

    parserFile = parserFile.make_preferred();
    symbolsFile = symbolsFile.make_preferred();
    reportPath = reportPath.make_preferred();

    std::error_code ec;
    if (!filesystem::is_directory(project->generatedPath))
    {
        if (!filesystem::create_directories(project->generatedPath, ec))
        {
            cout << "BISON tool failed because output directory can't be created\n";
            return false;
        }
    }

    stringstream params;
    params << tool->executablePath.u8string() << " ";
    params << "\"" << file->absolutePath.u8string() << "\" ";
    params << "-o\"" << parserFile.u8string() << "\" ";
    params << "--defines=\"" << symbolsFile.u8string() << "\" ";
    params << "--report-file=\"" << reportPath.u8string() << "\" ";
    params << "--verbose";

    const auto activeDir = filesystem::current_path();
    const auto bisonDir = tool->executablePath.parent_path();
    filesystem::current_path(bisonDir);

    auto code = std::system(params.str().c_str());

    filesystem::current_path(activeDir);

    if (code != 0)
    {
        cout << "BISON tool failed with exit code " << code << "\n";
        return false;
    }

    {
        auto* generatedFile = new GeneratedProjectFile();
        generatedFile->absolutePath = parserFile;
        generatedFile->name = parserFileName;
        generatedFile->filterPath = "_generated";
        generatedFile->type = ProjectFileType::CppSource;
        project->files.push_back(generatedFile);
    }

    {
        auto* generatedFile = new GeneratedProjectFile();
        generatedFile->absolutePath = symbolsFile;
        generatedFile->name = symbolsFileName;
        generatedFile->filterPath = "_generated";
        generatedFile->type = ProjectFileType::CppHeader;
        project->files.push_back(generatedFile);
    }

    return true;
}

//--

#if 0
private void generateGenericMainFile(GeneratedFile f) {


}
private void generateBisonOutputs(File sourcefile) {
    // get the name of the reflection file
    Path parserFile = localGeneratedPath.resolve(sourcefile.coreName + "_Parser.cpp");
    Path symbolsFile = localGeneratedPath.resolve(sourcefile.coreName + "_Symbols.h");
    Path reportPath = localGeneratedPath.resolve(sourcefile.coreName + "_Report.txt");

    // run the bison
    try {
        Path bisonPath = null;
        Path envPath = null;

        String OS = System.getProperty("os.name", "generic").toLowerCase(Locale.ENGLISH);
        if (OS.indexOf("win") >= 0) {
            bisonPath = solutionSetup.libs.resolveFileInLibraries("bison/win/bin/win_bison.exe");
            envPath = solutionSetup.libs.resolveFileInLibraries("bison/win/share/bison");
        }
        else {
            bisonPath = solutionSetup.libs.resolveFileInLibraries("bison/linux/bin/bison");
            envPath = solutionSetup.libs.resolveFileInLibraries("bison/linux/share/bison");
        }

        if (!Files.exists(bisonPath)) {
            System.err.printf("Unable to find Bison executable at %s, Bison files will NOT be build, projects may not compile\n", bisonPath.toString());
            return;
        }

        Files.createDirectories(parserFile.getParent());
        Files.createDirectories(symbolsFile.getParent());
        Files.createDirectories(reportPath.getParent());

        if (GeneratedFile.ShouldGenerateFile(sourcefile.absolutePath, parserFile)) {
            String commandLine = String.format("%s %s -o%s --defines=%s --verbose --report-file=%s", bisonPath.toString(), sourcefile.absolutePath.toString(), parserFile.toString(), symbolsFile.toString(), reportPath.toString());
            System.out.println(String.format("Running command '%s' with env '%s'", commandLine, envPath));
            Process p = Runtime.getRuntime().exec(commandLine, new String[]{ "BISON_PKGDATADIR=" + envPath.toString() });
            p.waitFor();

            // forward output
            BufferedReader stdOut = new BufferedReader(new InputStreamReader(p.getInputStream()));
            String s = null;
            while ((s = stdOut.readLine()) != null) {
                System.out.println(s);
            }

            // forward errors
            BufferedReader stdErr = new BufferedReader(new InputStreamReader(p.getErrorStream()));
            while ((s = stdErr.readLine()) != null) {
                System.err.println(s);
            }

            if (0 != p.exitValue())
                throw new ProjectException("Unable to process BISON file " + sourcefile.shortName + ", failed with error code: " + p.exitValue());

            // tag the file with the same timestamp as the source file
            // Files.setLastModifiedTime(parserFile, Files.getLastModifiedTime(sourcefile.absolutePath));
            //Files.setLastModifiedTime(symbolsFile, Files.getLastModifiedTime(sourcefile.absolutePath));
        }
    }
    catch (Exception e) {
        e.printStackTrace();
        throw new ProjectException("Unable to start BISON", e);
    }

    // remember about the file
    localAdditionalSourceFiles.add(parserFile);
    localAdditionalHeaderFiles.add(symbolsFile);
}

private void generateGenericProjectStaticInitializationFile(GeneratedFile f) {

}
#endif

//--
