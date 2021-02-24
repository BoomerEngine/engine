#include "common.h"
#include "toolReflection.h"
#include "generated.h"

//--

ProjectReflection::~ProjectReflection()
{

}

static bool ProjectsNeedsReflectionUpdate(const filesystem::path& reflectionFile, const vector<ProjectReflection::RefelctionFile*>& files)
{
    try
    {
        if (!filesystem::is_regular_file(reflectionFile))
            return true;

        const auto reflectionFileTimestamp = filesystem::last_write_time(reflectionFile);

        for (const auto* file : files)
        {
            const auto sourceFileTimestamp = filesystem::last_write_time(file->file->absolutePath);
            if (sourceFileTimestamp > reflectionFileTimestamp)
            {
                cout << "File " << file->file->absolutePath << " detected as changed, reflection will have to be refreshed\n";
                return true;
            }
        }

        return false;
    }
    catch (...)
    {
        return true;
    }
}

bool ProjectReflection::extract(const ProjectStructure& structure, const Configuration& config)
{
    for (auto* proj : structure.projects)
    {
        //if (proj->type == ProjectType::LocalApplication || proj->type == ProjectType::LocalLibrary)
        {
            vector<ProjectReflection::RefelctionFile*> localFiles;
            for (const auto* file : proj->files)
            {
                if (file->type == ProjectFileType::CppSource)
                {
                    if (file->name != "build.cpp" && file->name != "main.cpp" && !EndsWith(file->name, ".c"))
                    {
                        auto* fileWrapper = new RefelctionFile();
                        fileWrapper->file = file;
                        fileWrapper->tokenized.contextPath = file->absolutePath;
                        localFiles.push_back(fileWrapper);
                    }
                }
            }

            if (!localFiles.empty())
            {
                const auto reflectionFilePath = config.solutionPath / "generated" / proj->mergedName / "reflection.inl";

                if (config.force || ProjectsNeedsReflectionUpdate(reflectionFilePath, localFiles))
                {
                    // if we are sure that project will have to be refreshed than run the project configurator
                    // this is need to set the filter flags on files (ie. we may want to have different classes on different platforms)
                    if (!proj->setupProject(config))
                        return false;

                    auto oldFiles = std::move(localFiles);
                    for (auto* file : oldFiles)
                    {
                        if (file->file->checkFilter(config.platform))
                        {
                            localFiles.push_back(file);
                            files.push_back(file);
                        }
                    }

                    if (!localFiles.empty())
                    {
                        auto* projectWrapper = new RefelctionProject();
                        projectWrapper->mergedName = proj->mergedName;
                        projectWrapper->reflectionFilePath = reflectionFilePath;
                        projectWrapper->files = std::move(localFiles);
                        projects.push_back(projectWrapper);
                    }
                }
            }
        }
    }

    cout << "Found " << projects.size() << " projects with " << files.size() << " files for reflection\n";
    return true;
}

bool ProjectReflection::tokenizeFiles()
{
    bool valid = true;

    for (auto* file : files)
    {
        string content;
        if (LoadFileToString(file->file->absolutePath, content))
        {
            valid &= file->tokenized.tokenize(content);
        }
        else
        {
            cout << "Failed to load content of file " << file->file->absolutePath << "\n";
            valid = false;
        }
    }

    return valid;
}

bool ProjectReflection::parseDeclarations()
{
    bool valid = true;

    for (auto* file : files)
        valid &= file->tokenized.process();

    uint32_t totalDeclarations = 0;
    for (auto* file : files)
        totalDeclarations += (uint32_t)file->tokenized.declarations.size();

    cout << "Discovered " << totalDeclarations << " declarations\n";

    return valid;
}

bool ProjectReflection::generateReflection(CodeGenerator& gen) const
{
    bool valid = true;

    for (const auto* p : projects)
    {
        auto file = gen.createFile(p->reflectionFilePath);
        valid &= generateReflectionForProject(*p, file->content);
    }

    return valid;
}

struct ExportedDeclaration
{
    const CodeTokenizer::Declaration* declaration = nullptr;
    int priority = 0;
};

static void ExtractDeclarations(const ProjectReflection::RefelctionProject& p, vector<ExportedDeclaration>& outList)
{
    for (const auto* file : p.files)
    {
        for (const auto& decl : file->tokenized.declarations)
        {
            ExportedDeclaration info;
            info.declaration = &decl;

            if (decl.type == CodeTokenizer::DeclarationType::CUSTOM_TYPE)
                info.priority = 10;
            else if (decl.type == CodeTokenizer::DeclarationType::ENUM)
                info.priority = 20;
            else if (decl.type == CodeTokenizer::DeclarationType::BITFIELD)
                info.priority = 30;
            else if (decl.type == CodeTokenizer::DeclarationType::CLASS)
            {
                if (EndsWith(decl.name, "Metadata"))
                    info.priority = 40;
                else
                    info.priority = 41;
            }
            else if (decl.type == CodeTokenizer::DeclarationType::GLOBAL_FUNC)
                info.priority = 50;

            outList.push_back(info);
        }
    }

    sort(outList.begin(), outList.end(), [](const ExportedDeclaration& a, const ExportedDeclaration& b)
        {
            if (a.priority != b.priority)
                return a.priority < b.priority;

            return a.declaration->name < b.declaration->name;
        });
}

bool ProjectReflection::generateReflectionForProject(const RefelctionProject& p, stringstream& f) const
{
    writeln(f, "/// Boomer Engine v4 by Tomasz \"RexDex\" Jonarski");
    writeln(f, "/// RTTI Glue Code Generator is under MIT License");
    writeln(f, "");
    writeln(f, "/// AUTOGENERATED FILE - ALL EDITS WILL BE LOST");
    writeln(f, "");
    writeln(f, "// --------------------------------------------------------------------------------");
    writeln(f, "");

    vector<ExportedDeclaration> declarations;
    ExtractDeclarations(p, declarations);

    for (const auto& d : declarations)
    {
        if (d.declaration->type == CodeTokenizer::DeclarationType::GLOBAL_FUNC)
        {
            writelnf(f, "namespace %s { extern void RegisterGlobalFunc_%s(); }", d.declaration->scope.c_str(), d.declaration->name.c_str());
        }
        else
        {
            writelnf(f, "namespace %s { extern void CreateType_%s(const char* name); }", d.declaration->scope.c_str(), d.declaration->name.c_str());
            writelnf(f, "namespace %s { extern void InitType_%s(); }", d.declaration->scope.c_str(), d.declaration->name.c_str());
            //writelnf(f, "namespace %s { extern void RegisterType_%s(const char* name); }", d.declaration->scope.c_str(), d.declaration->name.c_str());
        }
    }

    writeln(f, "");
    writeln(f, "// --------------------------------------------------------------------------------");
    writeln(f, "");

    writelnf(f, "void InitializeReflection_%s", p.mergedName.c_str());
    writeln(f, "{");
    for (const auto& d : declarations)
    {
        if (d.declaration->type != CodeTokenizer::DeclarationType::GLOBAL_FUNC)
        {
            writelnf(f, "%s::CreateType_%s(\"%s::%s\"); }",
                d.declaration->scope.c_str(), d.declaration->name.c_str(),
                d.declaration->scope.c_str(), d.declaration->name.c_str());
        }
    }

    for (const auto& d : declarations)
    {
        if (d.declaration->type == CodeTokenizer::DeclarationType::GLOBAL_FUNC)
        {
            writelnf(f, "%s::RegisterGlobalFunc_%s(); }",
                d.declaration->scope.c_str(), d.declaration->name.c_str());
        }
        else
        {
            /*writelnf(f, "%s::RegisterType_%s(\"%s::%s\"); }",
                d.declaration->scope.c_str(), d.declaration->name.c_str(),
                d.declaration->scope.c_str(), d.declaration->name.c_str());*/
            writelnf(f, "%s::InitType_%s(); }",
                d.declaration->scope.c_str(), d.declaration->name.c_str());
        }
    }
    writeln(f, "}");

    writeln(f, "");
    writeln(f, "// --------------------------------------------------------------------------------");
    writeln(f, "");

    writelnf(f, "void InitializeTests_%s()", p.mergedName.c_str());
    writeln(f, "{");
    writeln(f, "}");

    writeln(f, "// --------------------------------------------------------------------------------");
    writeln(f, "");

    writeln(f, "#ifdef DECLARE_MODULE");
    writeln(f, "#undef DECLARE_MODULE");
    writeln(f, "#endif");
    writeln(f, "#define DECLARE_MODULE(_projectName) DECLARE_MODULE_WITH_REFLECTION_IMPL(_projectName)");

    return true;
}

//--

ToolReflection::ToolReflection()
{}


bool ToolReflection::run(const Configuration& config)
{
    ProjectStructure structure;

    if (!config.engineSourcesPath.empty())
        structure.scanProjects(ProjectGroupType::Engine, config.engineSourcesPath);

    if (!config.projectSourcesPath.empty())
        structure.scanProjects(ProjectGroupType::User, config.projectSourcesPath);

    uint32_t totalFiles = 0;
    if (!structure.scanContent(totalFiles))
        return false;

    cout << "Found " << totalFiles << " total files across " << structure.projects.size() << " projects\n";

    ProjectReflection reflection;
    if (!reflection.extract(structure, config))
        return false;

    if (!reflection.tokenizeFiles())
        return false;

    if (!reflection.parseDeclarations())
        return false;

    //--

    CodeGenerator codeGenerator(config);

    cout << "Generating reflection files...\n";

    if (!reflection.generateReflection(codeGenerator))
        return false;

    if (!codeGenerator.saveFiles())
        return false;

    return true;
}

//--