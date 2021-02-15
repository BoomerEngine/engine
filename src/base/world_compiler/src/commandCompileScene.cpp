/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: command #]
*
***/

#include "build.h"
#include "worldEntitySoup.h"
#include "worldEntityIslandGeneration.h"
#include "worldEntityStreamingGrid.h"

#include "base/app/include/command.h"
#include "base/app/include/commandline.h"
#include "base/resource/include/resourceLoadingService.h"
#include "base/resource/include/resourceLoader.h"
#include "base/world/include/worldRawScene.h"
#include "base/world/include/worldCompiledScene.h"
#include "base/resource/include/resourceFileSaver.h"
#include "base/io/include/ioFileHandle.h"
#include "base/world/include/worldStreamingSector.h"
#include "base/resource/include/depotService.h"

namespace base
{
    namespace world
    {

        //---

        class CommandCompileScene : public app::ICommand
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CommandCompileScene, app::ICommand);

        public:
            virtual bool run(IProgressTracker* progress, const app::CommandLine& commandline) override final;
        };

        //---

        RTTI_BEGIN_TYPE_CLASS(CommandCompileScene);
            RTTI_METADATA(app::CommandNameMetadata).name("compileScene");
        RTTI_END_TYPE();

        static void DistributeIslandsIntoGrid(const SourceIslands& islands, SourceStreamingGrid& outGrid)
        {
            for (const auto& island : islands.rootIslands)
                InsertIslandIntoGrid(island, outGrid);
        }

        static bool SaveFileToDepot(StringView path, IObject* data)
        {
            ScopeTimer timer;

            auto file = GetService<DepotService>()->createFileWriter(path);
            if (!file)
            {
                TRACE_ERROR("Unable to open '{}' for saving", path);
                return false;
            }

            res::FileSavingContext context;
            context.rootObject.pushBack(AddRef(data));
            if (!SaveFile(file, context))
            {
                TRACE_ERROR("Unable to save '{}'", path);
                file->discardContent();
                return false;
            }

            TRACE_INFO("Saved '{}' in {}", path, timer);
            return true;
        }

        static StringBuf BuildCellSavePath(StringView outputDirectoryPath, const SourceStreamingGridCell& cell)
        {
            const auto extension = res::IResource::GetResourceExtensionForClass(StreamingSector::GetStaticClass());
            return StringBuf(TempString("{}sectors/sector_{}_({}x{}).{}",
                outputDirectoryPath,
                cell.level, cell.cellX, cell.cellY,
                extension));
        }

        static StringBuf BuildCompiledSceneSavePath(StringView outputDirectoryPath)
        {
            const auto extension = res::IResource::GetResourceExtensionForClass(CompiledScene::GetStaticClass());
            return StringBuf(TempString("{}scene.{}", outputDirectoryPath, extension));
        }

        bool CommandCompileScene::run(IProgressTracker* progress, const app::CommandLine& commandline)
        {
            ScopeTimer timer;

            auto loadingService = GetService<res::LoadingService>();
            if (!loadingService || !loadingService->loader())
            {
                TRACE_ERROR("Resource loading service not started properly (incorrect depot mapping?), cooking won't be possible.");
                return false;
            }

            const auto depotPath = commandline.singleValue("sceneDepotPath");
            if (depotPath.empty())
            {
                TRACE_ERROR("Missing '-sceneDepotPath' parameter pointing to the raw scene");
                return false;
            }

            StringBuf outputDirectoryPath = commandline.singleValue("outputDirectory");
            if (outputDirectoryPath.empty())
            {
                const auto loadPath = res::ResourcePath(depotPath);
                outputDirectoryPath = TempString("{}compiled/", loadPath.basePath());
                TRACE_INFO("No location for cooked scene specified, using the default '{}'", outputDirectoryPath);
            }
            else
            {
                TRACE_INFO("Using specified cooked scene location: {}", outputDirectoryPath);

                if (!ValidateDepotPath(outputDirectoryPath, DepotPathClass::AbsoluteDirectoryPath))
                {
                    TRACE_ERROR("Invalid output path: '{}'", outputDirectoryPath);
                    return false;
                }
            }

            TRACE_INFO("Loading scene '{}'", depotPath);

            auto rawScene = LoadResource<RawScene>(depotPath);
            if (!rawScene)
            {
                TRACE_ERROR("Unable to load '{}'", depotPath);
                return false;
            }

            //--

            // extract entity source
            SourceEntitySoup soup;
            ExtractSourceEntities(res::ResourcePath(depotPath), soup);

            // build entity islands
            SourceIslands islands;
            ExtractSourceIslands(soup, islands);

            // build streaming grid
            SourceStreamingGrid grid;
            InitializeGrid(islands.totalStreamingArea, 16.0f, islands.largestStreamingDistance, grid);

            // distribute islands
            DistributeIslandsIntoGrid(islands, grid);

            // dump grid info
            DumpGrid(grid);

            // collect cells that are not empty
            Array<const SourceStreamingGridCell*> finalCells;
            CollectFinalCells(grid, finalCells);
            TRACE_INFO("Collected {} final cells", finalCells.size());

            // save sectors
            CompiledScene::Setup setup;
            for (const auto* cell : finalCells)
            {
                if (const auto cellData = BuildSectorFromCell(*cell))
                {
                    const auto savePath = BuildCellSavePath(outputDirectoryPath, *cell);
                    TRACE_INFO("Saving cell to '{}'...", savePath);

                    if (!SaveFileToDepot(savePath, cellData))
                    {
                        TRACE_ERROR("Failed to save compiled scene file");
                        return false;
                    }

                    auto& cellInfo = setup.cells.emplaceBack();
                    cellInfo.streamingBox = cellData->streamingBounds();
                    cellInfo.data = StreamingSectorAsyncRef(res::ResourcePath(savePath));
                }
            }

            // prepare compiled scene object
            {
                const auto savePath = BuildCompiledSceneSavePath(outputDirectoryPath);

                auto compiledScene = RefNew<CompiledScene>(setup);
                if (!SaveFileToDepot(savePath, compiledScene))
                {
                    TRACE_ERROR("Failed to save compiled scene file");
                    return false;
                }
            }

            // done
            return true;
        }

        //---

    } // world
} // base



