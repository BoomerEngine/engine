/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: command #]
*
***/

#include "build.h"
#include "worldCompiler.h"

#include "core/app/include/command.h"
#include "core/app/include/commandline.h"
#include "core/resource/include/loader.h"
#include "core/resource/include/loader.h"
#include "engine/world/include/rawWorldData.h"
#include "engine/world/include/compiledWorldData.h"
#include "core/resource/include/fileSaver.h"
#include "core/io/include/fileHandle.h"
#include "core/resource/include/depot.h"
#include "core/containers/include/path.h"

BEGIN_BOOMER_NAMESPACE()

//---

class CommandCompileScene : public ICommand
{
    RTTI_DECLARE_VIRTUAL_CLASS(CommandCompileScene, ICommand);

public:
    virtual bool run(IProgressTracker* progress, const CommandLine& commandline) override final;
};

//---

RTTI_BEGIN_TYPE_CLASS(CommandCompileScene);
    RTTI_METADATA(CommandNameMetadata).name("compileScene");
RTTI_END_TYPE();

bool CommandCompileScene::run(IProgressTracker* progress, const CommandLine& commandline)
{
    ScopeTimer timer;

    const auto depotPath = commandline.singleValue("sceneDepotPath");
    if (depotPath.empty())
    {
        TRACE_ERROR("Missing '-sceneDepotPath' parameter pointing to the raw scene");
        return false;
    }

    StringBuf outputDepotPath = commandline.singleValue("outputDepotPath");
    if (outputDepotPath.empty())
    {
        outputDepotPath = TempString("{}.compiled/{}.{}", depotPath.view().baseDirectory(), depotPath.view().fileStem(), IResource::FILE_EXTENSION);
        TRACE_INFO("No location for cooked scene specified, using the default '{}'", outputDepotPath);
    }
    else
    {
        TRACE_INFO("Using specified cooked scene location: {}", outputDepotPath);
        if (!ValidateDepotFilePath(outputDepotPath))
        {
            TRACE_ERROR("Invalid output path: '{}'", outputDepotPath);
            return false;
        }
    }

    TRACE_INFO("Compiling world '{}'", depotPath);

    auto compiledScene = CompileRawWorld(depotPath, *progress);
    if (!compiledScene)
    {
        TRACE_ERROR("Unable to compile '{}'", depotPath);
        return false;
    }

    if (!GetService<DepotService>()->saveFileFromResource(outputDepotPath, compiledScene))
    {
        TRACE_ERROR("Unable to save compiled world to '{}'", outputDepotPath);
        return false;
    }
    
    return true;
}

//---

END_BOOMER_NAMESPACE()
