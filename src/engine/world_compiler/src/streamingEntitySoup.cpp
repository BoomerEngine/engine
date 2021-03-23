/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*
***/

#include "build.h"
#include "streamingEntitySoup.h"

#include "engine/world/include/rawLayer.h"
#include "engine/world/include/rawEntity.h"
#include "engine/world/include/rawPrefab.h"
#include "engine/world/include/worldEntityID.h"

#include "core/resource/include/indirectTemplateCompiler.h"
#include "core/resource/include/indirectTemplate.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE()

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
    GetService<DepotService>()->enumFilesAtPath(directoryPath, [&outLayers, directoryPath](StringView name)
        {
            if (name.endsWith(RawLayer::FILE_EXTENSION))
            {
                auto* entry = new SourceLayer();
                entry->path = StringBuf(TempString("{}{}", directoryPath, name));
                outLayers.pushBack(entry);
            }
            return false;
        });

    // scan the depot directories at path
    GetService<DepotService>()->enumDirectoriesAtPath(directoryPath, [&outLayers, directoryPath](StringView name)
        {
            ExtractSourceLayersAtDirectory(TempString("{}{}/", directoryPath, name), outLayers);
            return false;
        });
}

bool ExtractLayerSoup(SourceLayer& layer)
{
    // load the layer
    auto data = LoadResource<RawLayer>(layer.path);
    if (!data)
    {
        TRACE_ERROR("Failed to load layer '{}', no soup will be extracted", layer.path);
        return false;
    }

    // the node path starts with the layer
    EntityStaticIDBuilder currentPath(layer.path.view());

    // extract the content from the nodes 
    // TODO: each node can be extracted in parallel as well
    for (const auto& node : data->nodes())
    {
        currentPath.pushSingle(node->m_name);

        if (auto hierarchy = CompileEntityHierarchy(currentPath, node, false))
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

void ExtractSourceEntities(const Array<RawEntityPtr>& roots, SourceEntitySoup& outSoup)
{
    outSoup.totalEntityCount += roots.size();

    EntityStaticIDBuilder currentPath;

    for (const auto& rootNode : roots)
    {
        currentPath.pushSingle(rootNode->m_name);

        if (auto hierarchy = CompileEntityHierarchy(currentPath, rootNode, false))
        {
            outSoup.rootEntities.emplaceBack(hierarchy);
            outSoup.totalEntityCount += hierarchy->countTotalEntities();
        }

        currentPath.pop();
    }
}

void ExtractSourceEntities(StringView worldFilePath, SourceEntitySoup& outSoup)
{
    InplaceArray<SourceLayer*, 128> collectedLayers;

    // scan layers in the world
    const auto worldDir = worldFilePath.baseDirectory();
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

END_BOOMER_NAMESPACE()
