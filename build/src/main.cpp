#include "common.h"
#include "project.h"
#include "toolMake.h"
#include "toolReflection.h"
#include "toolScriptMake.h"

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




int main(int argc, char** argv)
{
    std::cout << "BoomerEngine BuildTool v1.0\n";

    Commandline cmdLine;
    if (!cmdLine.parse(MergeCommandline(argc, argv))) {
        std::cout << "Invalid command line std::string\n";
        return -1;
    }

    const auto& tool = cmdLine.get("tool");
    if (tool == "make" || tool.empty())
    {
        ToolMake tool;
        return tool.run(argv[0], cmdLine);
    }
    else if (tool == "reflection")
    {
        ToolReflection tool;
        return tool.run(argv[0], cmdLine);
    }
    else if (tool == "scriptMake")
    {
        ToolScriptMake tool;
        return tool.run(argv[0], cmdLine);
    }
    else
    {
        std::cout << "Unknown tool specified\n";
        return -1;
    }

    return 0;
}
