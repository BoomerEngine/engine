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
        string mergedName;
        vector<RefelctionFile*> files;
        filesystem::path reflectionFilePath;
    };

    vector<RefelctionFile*> files;
    vector<RefelctionProject*> projects;

    ~ProjectReflection();

    bool extract(const ProjectStructure& structure, const Configuration& config);
    bool tokenizeFiles();
    bool parseDeclarations();
    bool generateReflection(CodeGenerator& gen) const;

private:
    bool generateReflectionForProject(const RefelctionProject& p, stringstream& f) const;
};

//--

class ToolReflection
{
public:
    ToolReflection();

    bool run(const Configuration& config);

private:
};


//--