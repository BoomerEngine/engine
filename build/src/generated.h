#pragma once

#include "utils.h"
#include "project.h"

//--

struct CodeGenerator
{
    CodeGenerator(const Configuration& config);

    struct GeneratedFile
    {
        GeneratedFile(const filesystem::path& path)
            : absolutePath(path)
        {}

        filesystem::path absolutePath;
        filesystem::file_time_type customtTime;

        stringstream content; // may be empty
    };

    struct GeneratedProjectFile
    {
        const ProjectStructure::FileInfo* originalFile = nullptr; // may be empty
        GeneratedFile* generatedFile = nullptr; // may be empty

        string name;
        string filterPath; // platform\\win
        ProjectFileType type = ProjectFileType::Unknown;

        bool useInCurrentBuild = true;

        filesystem::path absolutePath; // location
    };

    struct GeneratedProject;
        
    struct GeneratedGroup
    {
        string name;
        string mergedName;

        string assignedVSGuid;

        GeneratedGroup* parent = nullptr;
        vector<GeneratedGroup*> children;
        vector<GeneratedProject*> projects;
    };
    
    struct GeneratedProject
    {
        const ProjectStructure::ProjectInfo* originalProject;

        GeneratedGroup* group = nullptr;

        string mergedName;
        filesystem::path generatedPath; // generated/base_math/
        filesystem::path outputPath; // output/base_math/
        filesystem::path projectPath; // project/base_math/

        filesystem::path localGlueHeader; // _glue.inl file
        filesystem::path localPublicHeader; // include/public.h file

        vector<GeneratedProject*> directDependencies;
        vector<GeneratedProject*> allDependencies;

        vector<GeneratedProjectFile*> files; // may be empty

        string assignedVSGuid;
    };

    bool extractProjects(const ProjectStructure& structure);

    bool generateAutomaticCode();

    bool generateExtraCode(); // tools

    bool saveFiles();

    GeneratedFile* createFile(const filesystem::path& path);

    GeneratedGroup* createGroup(string_view name);

    GeneratedGroup* rootGroup = nullptr;

    vector<GeneratedProject*> projects;

    vector<filesystem::path> sourceRoots;

private:
    //--

    Configuration config;

    bool useStaticLinking = false;

    filesystem::path sharedGlueFolder;

    vector<GeneratedFile*> files; // may be empty

    unordered_map<const ProjectStructure::ProjectInfo*, GeneratedProject*> projectsMap;

    //--


    //-

    bool generateAutomaticCodeForProject(GeneratedProject* project);
    bool generateExtraCodeForProject(GeneratedProject* project);

    bool processBisonFile(GeneratedProject* project, const GeneratedProjectFile* file);

    bool generateProjectGlueFile(const GeneratedProject* project, stringstream& outContent);
    bool generateProjectStaticInitFile(const GeneratedProject* project, stringstream& outContent);
    bool generateProjectAutoMainFile(const GeneratedProject* project, stringstream& outContent);
    bool generateProjectDefaultReflection(const GeneratedProject* project, stringstream& outContent);

    bool projectRequiresStaticInit(const GeneratedProject* project) const;

    bool shouldUseFile(const ProjectStructure::FileInfo* file) const;

    GeneratedGroup* findOrCreateGroup(string_view name, GeneratedGroup* parent);

    //--

};

//--