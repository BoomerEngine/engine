#include "common.h"
#include "project.h"
#include "toolMake.h"
#include "toolReflection.h"

static std::string MergeCommandline(int argc, char** argv)
{
    std::string ret;

    for (int i = 1; i < argc; ++i) {
        if (!ret.empty())
            ret += " ";

        ret += argv[i];
    }

    return ret;
}

bool RunTool(const Configuration& config)
{
    if (config.tool == ToolType::SolutionGenerator)
    {
        ToolMake tool;
        return tool.run(config);
    }
    else if (config.tool == ToolType::ReflectionGenerator)
    {
        ToolReflection tool;
        return tool.run(config);
    }    

    return false;
}

int main(int argc, char** argv)
{
    std::cout << "BoomerEngine BuildTool v1.0\n";

    Commandline cmdLine;
    if (!cmdLine.parse(MergeCommandline(argc, argv))) {
        std::cout << "Invalid command line string\n";
        return -1;
    }

    Configuration config;
    if (!config.parse(argv[0], cmdLine)) {
        std::cout << "Invalid/incomplete configuration\n";
        return -1;
    }

    if (!RunTool(config))
    {
        std::cout << "There were errors running selected tool\n";
        return -1;
    }

    return 0;
}
