#pragma once

#include "utils.h"
#include "project.h"
#include "generated.h"

//--

struct SolutionGeneratorCMAKE
{
    SolutionGeneratorCMAKE(const Configuration& config, CodeGenerator& gen);

    bool generateSolution();
    bool generateProjects();

private:
    const Configuration& m_config;
    CodeGenerator& m_gen;

    fs::path m_cmakeScriptsPath;
    bool m_buildWithLibs = false;

    bool generateProjectFile(const CodeGenerator::GeneratedProject* project, std::stringstream& outContent) const;

    void extractSourceRoots(const CodeGenerator::GeneratedProject* project, std::vector<fs::path>& outPaths) const;

    void printSolutionDeclarations(std::stringstream& f, const CodeGenerator::GeneratedGroup* g);    
    void printSolutionParentLinks(std::stringstream& f, const CodeGenerator::GeneratedGroup* g);

    bool shouldStaticLinkProject(const CodeGenerator::GeneratedProject* project) const;
};

//--