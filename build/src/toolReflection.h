#pragma once

#include "utils.h"
#include "project.h"
#include "codeParser.h"

//--

struct CodeGenerator;

struct ProjectReflection
{
    struct RefelctionFile
    {
        const ProjectStructure::FileInfo* file = nullptr;

        CodeTokenizer tokenized;
    };

    struct RefelctionProject
    {
        std::string mergedName;
        std::vector<RefelctionFile*> files;
        fs::path reflectionFilePath;
        fs::file_time_type reflectionFileTimstamp;
    };

    std::vector<RefelctionFile*> files;
    std::vector<RefelctionProject*> projects;

    ~ProjectReflection();

    bool extract(const ProjectStructure& structure, const Configuration& config);
    bool tokenizeFiles();
    bool parseDeclarations();
    bool generateReflection(CodeGenerator& gen) const;

private:
    bool generateReflectionForProject(const RefelctionProject& p, std::stringstream& f) const;
};

//--

class ToolReflection
{
public:
    ToolReflection();

    bool run(const Configuration& config);

private:
};

extern bool GenerateInlinedReflection(const Configuration& config, ProjectStructure& structure, CodeGenerator& codeGenerator);

//--