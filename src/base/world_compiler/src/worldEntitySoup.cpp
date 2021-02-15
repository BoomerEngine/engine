/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*
***/

#include "build.h"
#include "worldEntitySoup.h"

#include "base/world/include/worldRawLayer.h"
#include "base/world/include/worldNodePath.h"
#include "base/world/include/worldNodeTemplate.h"
#include "base/world/include/worldPrefab.h"

#include "base/resource/include/objectIndirectTemplateCompiler.h"
#include "base/resource/include/objectIndirectTemplate.h"
#include "base/resource/include/depotService.h"

namespace base
{
    namespace world
    {

        //---

        struct SourceLayer
        {
            StringBuf path;
            SourceEntitySoup layerSoup;
            bool valid = false;
        };

        void ExtractSourceLayersAtDirectory(StringView directoryPath, Array<SourceLayer*>& outLayers)
        {
            // scan the layers (they must come first so they can be loaded first)
            GetService<DepotService>()->enumFilesAtPath(directoryPath, [&outLayers, directoryPath](const DepotService::FileInfo& file)
                {
                    static const auto rawLayerExtensions = res::IResource::GetResourceExtensionForClass(RawLayer::GetStaticClass());
                    if (file.name.endsWith(rawLayerExtensions))
                    {
                        auto* entry = new SourceLayer();
                        entry->path = StringBuf(TempString("{}{}", directoryPath, file.name));
                        outLayers.pushBack(entry);
                    }
                    return false;
                });

            // scan the depot directories at path
            GetService<DepotService>()->enumDirectoriesAtPath(directoryPath, [&outLayers, directoryPath](const DepotService::DirectoryInfo& dir)
                {
                    ExtractSourceLayersAtDirectory(TempString("{}{}/", directoryPath, dir.name), outLayers);
                    return false;
                });
        }

        bool ExtractLayerSoup(SourceLayer& layer)
        {
            // load the layer
            auto data = LoadResource<RawLayer>(layer.path).acquire();
            if (!data)
            {
                TRACE_ERROR("Failed to load layer '{}', no soup will be extracted", layer.path);
                return false;
            }

            // the node path starts with the layer
            NodePathBuilder currentPath(layer.path.view());

            // extract the content from the nodes 
            // TODO: each node can be extracted in parallel as well
            for (const auto& node : data->nodes())
            {
                currentPath.pushSingle(node->m_name);

                if (auto hierarchy = CompileEntityHierarchy(currentPath, node, nullptr, nullptr))
                {
                    layer.layerSoup.rootEntities.pushBack(hierarchy);
                    layer.layerSoup.totalEntityCount += hierarchy->countTotalEntities();
                }

                currentPath.pop();
            }

            // extracted
            layer.valid = true;
            return true;
        }

        void ExtractSourceEntities(const res::ResourcePath& worldFilePath, SourceEntitySoup& outSoup)
        {
            InplaceArray<SourceLayer*, 128> collectedLayers;

            // scan layers in the world
            const auto worldDir = worldFilePath.basePath();
            ExtractSourceLayersAtDirectory(TempString("{}layers/", worldDir), collectedLayers);
            TRACE_INFO("Found {} layers in scene {}", collectedLayers.size(), worldFilePath);

            // generate per-layer soup
            uint32_t numRootEntities = 0;
            {
                ScopeTimer timer;
                uint32_t numFailedLayers = 0;
                uint32_t numTotalEntities = 0;
                for (auto* layer : collectedLayers)
                    ExtractLayerSoup(*layer);

                uint32_t numInvalidLayers = 0;
                for (const auto* layer : collectedLayers)
                {
                    numTotalEntities += layer->layerSoup.totalEntityCount;
                    numRootEntities += layer->layerSoup.rootEntities.size();
                    if (!layer->valid)
                        numInvalidLayers += 1;
                }

                TRACE_INFO("Generated entity soup ({} total entities, {} roots) in {}", numTotalEntities, numRootEntities, timer);

                if (numInvalidLayers)
                {
                    TRACE_ERROR("There were {} layer(s) that failed to extract:", numInvalidLayers);
                    for (const auto* layer : collectedLayers)
                    {
                        if (!layer->valid)
                        {
                            TRACE_ERROR("Invalid layer: '{}'", layer->path);
                        }
                    }
                }
            }

            // merge soup
            outSoup.rootEntities.reserve(numRootEntities);
            for (const auto* layer : collectedLayers)
            {
                outSoup.totalEntityCount += layer->layerSoup.totalEntityCount;
                for (auto& rootEntity : layer->layerSoup.rootEntities)
                    outSoup.rootEntities.emplaceBack(std::move(rootEntity));
            }
        }

        //---

    } // world
} // base



