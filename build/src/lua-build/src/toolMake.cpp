#include "common.h"
#include "toolMake.h"
#include "generated.h"
#include "generatorVS.h"

//--

ToolMake::ToolMake()
{}

bool ToolMake::run(const Configuration& config)
{
    ProjectStructure structure;

    if (!config.engineSourcesPath.empty())
        structure.scanProjects(ProjectGroupType::Engine, config.engineSourcesPath);

    if (!config.projectSourcesPath.empty())
        structure.scanProjects(ProjectGroupType::User, config.projectSourcesPath);

    uint32_t totalFiles = 0;
    if (!structure.scanContent(totalFiles))
        return false;

    if (!structure.setupProjects(config))
        return false;

    if (!structure.resolveProjectDependencies())
        return false;

    cout << "Found " << totalFiles << " total files across " << structure.projects.size() << " projects\n";

    if (!structure.deployFiles(config))
        return false;

    CodeGenerator codeGenerator(config);
    if (!codeGenerator.extractProjects(structure))
        return false;

    if (!codeGenerator.generateAutomaticCode())
        return false;

    if (!codeGenerator.generateExtraCode())
        return false;

    if (config.generator == GeneratorType::VisualStudio)
    {
        SolutionGeneratorVS gen(config, codeGenerator);
        if (!gen.generateSolution())
            return false;
        if (!gen.generateProjects())
            return false;        
    }

    if (!codeGenerator.saveFiles())
        return false;

    return true;
}

//--