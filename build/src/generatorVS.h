#pragma once

#include "utils.h"
#include "project.h"
#include "generated.h"

//--

struct SolutionGeneratorVS
{
    SolutionGeneratorVS(const Configuration& config, CodeGenerator& gen);

    bool generateSolution();
    bool generateProjects();

private:
    const Configuration& m_config;
    CodeGenerator& m_gen;

    filesystem::path m_visualStudioScriptsPath;

    bool generateSourcesProjectFile(const CodeGenerator::GeneratedProject* project, stringstream& outContent) const;
    bool generateSourcesProjectFilters(const CodeGenerator::GeneratedProject* project, stringstream& outContent) const;
    bool generateSourcesProjectFileEntry(const CodeGenerator::GeneratedProject* project, const CodeGenerator::GeneratedProjectFile* file, stringstream& f) const;

    bool generateRTTIGenProjectFile(const CodeGenerator::GeneratedProject* project, stringstream& outContent) const;

    void extractSourceRoots(const CodeGenerator::GeneratedProject* project, vector<filesystem::path>& outPaths) const;

    void printSolutionDeclarations(stringstream& f, const CodeGenerator::GeneratedGroup* g);    
    void printSolutionParentLinks(stringstream& f, const CodeGenerator::GeneratedGroup* g);
};

//--