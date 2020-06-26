/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\build #]
***/

#include "build.h"

#include "base/app/include/localServiceContainer.h"
#include "base/app/include/commandline.h"
#include "base/app/include/localServiceContainer.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/absolutePath.h"
#include "base/io/include/absolutePathBuilder.h"
//#include "base/depot/include/depotStructure.h"

#include "scene/common/include/sceneWorld.h"
#include "scene/common/include/sceneLayer.h"

#include "sceneNodeCollector.h"

namespace scene
{

#if 0
    //--

    class WorldCompiler : public base::app::ICommand
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldCompiler, base::app::ICommand);

    public:
        WorldCompiler()
        {
            name = "WorldCompiler"_id;
        }

        virtual void printHelp() const override final
        {
            TRACE_INFO("Command: WorldCompiler");
            TRACE_INFO("Version: 0.1");
            TRACE_INFO("Arguments:");
            TRACE_INFO("  -worldPath=<depot path to world file to compile>");
        }

        virtual int execute(const base::app::CommandLine& commandLine) override final
        {
            base::ScopeTimer timer;

            // get the path to the world
            auto worldDepotPath = commandLine.singleValue("worldPath");
            if (worldDepotPath.empty())
            {
                TRACE_ERROR("Missing path to world file (-worldPath)");
                return -2;
            }

            // load the world
            auto world = base::LoadResource<scene::World>(base::res::ResourcePath(worldDepotPath));
            if (!world)
            {
                TRACE_ERROR("Unable to load world from '{}'", worldDepotPath);
                return -4;
            }

            // print world name
            TRACE_INFO("Started to process world '{}'", worldDepotPath);

            base::depot::DepotStructure loader;
            if (!loader.initialize(commandLine))
            { 
                TRACE_ERROR("Cannot initialize depot at given depot path");
                return -5;
            }

            // list all layers
            base::Array<base::res::ResourcePath> worldLayerPaths;
            world->collectLayerPaths(loader, worldLayerPaths);
            TRACE_INFO("Found {} layers in world '{}", worldLayerPaths.size(), worldDepotPath);

            // extract nodes
            NodeCollector nodes;
            {
                base::ScopeTimer timer;

                for (auto& path : worldLayerPaths)
                    nodes.extractNodesFromLayer(path);

                TRACE_INFO("Extracted {} nodes in {}", nodes.nodeCount(), TimeInterval(timer.timeElapsed()));
            }

            // compile sectors
            base::Array<WorldSectorDesc> cookedSectors;
            {
                base::ScopeTimer timer;
                nodes.pack(cookedSectors);
                TRACE_INFO("Packed {} nodes into {} sectors in {}", nodes.nodeCount(), cookedSectors.size(), TimeInterval(timer.timeElapsed()));
            }

            // get the path to cooked data for the world
            auto worldCookedPath = base::StringBuf(base::TempString("{}/cooked/", worldDepotPath.stringBeforeLast("/")));
            TRACE_INFO("Output path for cooked world: '{}'", worldCookedPath);

            // create the cooked world resource
            auto cookedWorld = base::CreateSharedPtr<CompiledWorld>();
            cookedWorld->content(world->parameters(), cookedSectors);

            // save cooked world
            if (!cookedWorld->save(loader, worldCookedPath))
            {
                TRACE_ERROR("Failed to save content of cooked world");
                return -4;
            }

            // done
            TRACE_INFO("All processing done in {}", TimeInterval(timer.timeElapsed()));
            return 0;
        }
    };

#endif

} // scene
