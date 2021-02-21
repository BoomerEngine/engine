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

                if (ProjectsNeedsReflectionUpdate(reflectionFilePath, localFiles))
                {
                    // if we are sure that project will have to be refreshed than run the project configurator
                    // this is need to set the filter flags on files (ie. we may want to have different classes on different platforms)
                    if (!proj->setupProject(config))
                        return false;

                    for (auto* file : std::move(localFiles))
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

    return valid;
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

    //if (!reflection.parseDeclarations())
      //  return false;

    //--

    CodeGenerator codeGenerator(config);

    cout << "Generating reflection files...\n";

    if (!codeGenerator.saveFiles())
        return false;

    return true;
}

//--