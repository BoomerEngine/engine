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

    fs::path m_visualStudioScriptsPath;
    bool m_buildWithLibs = false;

    bool generateSourcesProjectFile(const CodeGenerator::GeneratedProject* project, std::stringstream& outContent) const;
    bool generateSourcesProjectFilters(const CodeGenerator::GeneratedProject* project, std::stringstream& outContent) const;
    bool generateSourcesProjectFileEntry(const CodeGenerator::GeneratedProject* project, const CodeGenerator::GeneratedProjectFile* file, std::stringstream& f) const;

    bool generateRTTIGenProjectFile(const CodeGenerator::GeneratedProject* project, std::stringstream& outContent) const;

    void extractSourceRoots(const CodeGenerator::GeneratedProject* project, std::vector<fs::path>& outPaths) const;

    void printSolutionDeclarations(std::stringstream& f, const CodeGenerator::GeneratedGroup* g);    
    void printSolutionParentLinks(std::stringstream& f, const CodeGenerator::GeneratedGroup* g);
};

//--