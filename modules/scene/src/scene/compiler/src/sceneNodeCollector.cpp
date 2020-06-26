/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\build #]
***/

#include "build.h"
#include "sceneNodeCollector.h"

#include "scene/common/include/scenePrefab.h"
#include "scene/common/include/sceneLayer.h"
#include "scene/common/include/sceneWorld.h"
#include "scene/common/include/sceneEntity.h"
#include "scene/common/include/sceneElement.h"
#include "scene/common/include/sceneComponent.h"
#include "scene/common/include/sceneNodeTemplate.h"
#include "scene/common/include/sceneNodeContainer.h"

#include "base/containers/include/inplaceArray.h"
#include "base/resources/include/resourceLoader.h"
#include "base/depot/include/depotStructure.h"
#include "base/app/include/localServiceContainer.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/absolutePath.h"
#include "base/io/include/absolutePathBuilder.h"

namespace scene
{

    //---

    /// helper class to extract all layers from world
    static void CollectLayersFromDirectory(base::depot::DepotStructure& loader, const base::StringBuf& directoryPath, base::Array<base::res::ResourcePath>& outLayers)
    {
        PC_SCOPE_LVL0(CollectLayersFromDirectory);

        // get the sub directories
        base::InplaceArray<base::depot::DepotStructure::DirectoryInfo, 16> subDirInfos;
        loader.enumDirectoriesAtPath(directoryPath, subDirInfos);

        // scan sub directories, each on separate job
        for (auto& subDir : subDirInfos)
        {
            base::StringBuf fullPath = base::TempString("{}{}/", directoryPath, subDir.name);
            CollectLayersFromDirectory(loader, fullPath, outLayers);
        }

        // get the layers
        base::InplaceArray<base::depot::DepotStructure::FileInfo, 16> fileInfos;
        loader.enumFilesAtPath(directoryPath, fileInfos);

        // scan the layers
        for (auto& fileInfo : fileInfos)
        {
            base::StringBuf fullPath = base::TempString("{}{}", directoryPath, fileInfo.name);

            base::io::TimeStamp timestamp;
            if (loader.queryFileInfo(fullPath, nullptr, nullptr, &timestamp))
            {
                auto layerDepotPath = base::res::ResourcePath(fullPath.c_str());
                outLayers.pushBack(layerDepotPath);
            }
            else
            {
                TRACE_ERROR("Reported layer file '{}' does not seem to be valid", fullPath);
            }
        }
    }

    NodePath CreateNodePathFromLayerPath(const base::res::ResourcePath& path)
    {
        NodePath layerPath = NodePath()["world"_id];

        auto layerPathStr = path.path().afterFirst("layers/").beforeLast(".");
        if (layerPathStr.empty())
        {
            TRACE_WARNING("Layer '{}' is not a world layer", path);
        }
        else
        {
            base::InplaceArray<base::StringView<char>, 10> layerPathParts;
            layerPathStr.slice("/", false, layerPathParts);

            for (auto& pathPart : layerPathParts)
                layerPath = layerPath[base::StringID(pathPart)];
        }

        return layerPath;
    }

    //--

    /// helper class to process content of a layer
    NodeCollector::NodeCollector()
    {}

    NodeCollector::~NodeCollector()
    {
        m_allNodes.clearPtr();
        m_rootNodes.clear();
    }

    void NodeCollector::extractNodesFromLayer(const base::res::ResourcePath& path)
    {
        PC_SCOPE_LVL0(ExtractLayer);

        // load the layer
        scene::LayerPtr layer;
        {
            PC_SCOPE_LVL0(LoadLayer);
            layer = base::LoadResource<scene::Layer>(path);
        }

        // no layer loaded
        if (!layer)
        {
            TRACE_WARNING("Failed to load layer '{}' for node extraction", path);
            return;
        }

        // keep layer alive
        m_loadedLayers.pushBack(layer);

        // no layer content
        auto layerData = layer->nodeContainer();
        if (!layerData || layerData->rootNodes().empty())
        {
            TRACE_WARNING("Layer '{}' has no content to process", path);
            return;
        }

        // create the path element for layer
        auto layerPath = CreateNodePathFromLayerPath(path);
        TRACE_INFO("Layer '{}' node path: '{}'", path, layerPath);

        // compile all nodes
        auto flattenedNodes = base::CreateSharedPtr<NodeTemplateContainer>();
        for (auto rootIndex : layerData->rootNodes())
        {
            auto& rootNode = layerData->nodes()[rootIndex];
            auto& placement = rootNode.m_data->placement();

            layerData->compile(rootIndex, placement.toAbsoluteTransform(), flattenedNodes, -1);
        }

        TRACE_INFO("Compiled {} layer nodes into {} flat nodes", layerData->nodes().size(), flattenedNodes->nodes().size());

        // process the extracted nodes
        for (auto rootIndex : flattenedNodes->rootNodes())
            extractNode(flattenedNodes, rootIndex, nullptr, layerPath);

        TRACE_INFO("Extarcted {} final flat entnties from {} nodes", m_allNodes.size(), flattenedNodes->nodes().size());
    }

    void NodeCollector::extractNode(const NodeTemplateContainerPtr& ptr, int nodeIndex, ExtractedNode* parentNode, const NodePath& parentPath)
    {
        auto& sourceNode = ptr->nodes()[nodeIndex];

        if (auto sourceData = sourceNode.m_data)
        {
            if (auto compiledEntity = sourceNode.m_data->compile())
            {
                // TODO: fix
                compiledEntity->relativePosition(sourceData->placement().T);
                compiledEntity->relativeRotation(sourceData->placement().R.toQuat());

                auto retNode  = MemNew(ExtractedNode);
                retNode->m_parent = parentNode;
                retNode->m_entity = compiledEntity;
                retNode->m_path = parentPath[sourceNode.m_data->name()];
                m_allNodes.pushBack(retNode);

                if (parentNode)
                    parentNode->m_children.pushBack(retNode);
                else
                    m_rootNodes.pushBack(retNode);

                // recurse
                for (auto childNodeIndex : sourceNode.m_children)
                    extractNode(ptr, childNodeIndex, retNode, retNode->m_path);
            }
        }
    }

    //--

    static const uint32_t TOP_LEVEL_GRID_SIZE = 1024;
    static const uint8_t MAX_GRID_LEVELS = 4;

    uint8_t NodeCollector::gridLevelForStreamingDistance(float distance) const
    {
        uint32_t size = TOP_LEVEL_GRID_SIZE;

        for (uint8_t level = 0; level < MAX_GRID_LEVELS; ++level)
        {
            if (distance >= size)
                return level;
            size /= 2;
        }

        return MAX_GRID_LEVELS;
    }

    base::Box NodeCollector::streamingBox(const base::Point& grid, uint8_t gridIndex) const
    {
        auto gridCellSize = (float)(TOP_LEVEL_GRID_SIZE >> gridIndex);
        auto halfGridCellSize = gridCellSize / 2.0f;

        auto x = gridCellSize * grid.x;
        auto y = gridCellSize * grid.y;

        base::Box ret;
        ret.min.x = x - halfGridCellSize;
        ret.min.y = y - halfGridCellSize;
        ret.min.z = -10000.0f;
        ret.max.x = x + halfGridCellSize;
        ret.max.y = y + halfGridCellSize;
        ret.max.z = 10000.0f;

        return ret;
    }

    base::Point NodeCollector::gridCooridinateForPosition(const base::AbsolutePosition& pos, uint8_t gridIndex) const
    {
        auto gridCellSize = (float)(TOP_LEVEL_GRID_SIZE >> gridIndex);

        auto x = (int)std::round(pos.approximate().x / gridCellSize);
        auto y = (int)std::round(pos.approximate().y / gridCellSize);

        return base::Point(x, y);
    }

    WorldSectorDesc* NodeCollector::alwaysLoadedSector(base::Array<WorldSectorDesc>& outPackedSectors)
    {
        for (auto& entry : outPackedSectors)
            if (entry.m_alwaysLoaded)
                return &entry;

        auto& sector = outPackedSectors.emplaceBack();
        sector.m_alwaysLoaded = true;
        sector.m_name = "root";
        sector.m_streamingBox = base::Box(base::Vector3(-100000.0f, -100000.0f, -100000.0f), base::Vector3(100000.0f, 100000.0f, 100000.0f));
        sector.m_unsavedSectorData = base::CreateSharedPtr<WorldSector>();
        
        return &sector;
    }

    WorldSectorDesc* NodeCollector::gridSector(base::Array<WorldSectorDesc>& outPackedSectors, uint8_t gridIndex, const base::Point& grid)
    {
        base::TempString name;
        name.appendf("grid{}_{}_{}", gridIndex, grid.x, grid.y);

        for (auto& entry : outPackedSectors)
            if (entry.m_name == name)
                return &entry;

        auto& sector = outPackedSectors.emplaceBack();
        sector.m_alwaysLoaded = false;
        sector.m_name = name;
        sector.m_streamingBox = streamingBox(grid, gridIndex);
        sector.m_unsavedSectorData = base::CreateSharedPtr<WorldSector>();

        return &sector;
    }

    void NodeCollector::packNodeIntoSector(const ExtractedNode* node, WorldSectorDesc* sector)
    {
        if (node->m_entity)
        {
            sector->m_unsavedSectorData->m_entities.pushBack(node->m_entity);
            node->m_entity->parent(sector->m_unsavedSectorData);
        }
    }

    void NodeCollector::packNode(const ExtractedNode* node, base::Array<WorldSectorDesc>& outPackedSectors)
    {
        // skip discarded nodes
        if (node->m_streamingModel == StreamingModel::Discard)
            return;

        // get target sector
        WorldSectorDesc* targetSector = nullptr;
        if (node->m_streamingModel == StreamingModel::AlwaysLoaded)
        {
            // put all "always loaded" stuff into the same sector
            targetSector = alwaysLoadedSector(outPackedSectors);
        }
        else
        {
            // calculate the streaming distance
            float streamingDistance = node->m_streamingOverrideDistance;
            if (streamingDistance <= 0.0f)
                streamingDistance = node->m_entity->calculateRequiredStreamingDistance();
            streamingDistance = std::max<float>(streamingDistance, 5.0f);

            // get grid sector
            auto gridIndex = gridLevelForStreamingDistance(streamingDistance);
            auto grid = gridCooridinateForPosition(node->m_entity->relativePosition(), gridIndex);

            targetSector = gridSector(outPackedSectors, gridIndex, grid);
        }

        // add node to sector
        packNodeIntoSector(node, targetSector);
    }

    void NodeCollector::pack(base::Array<WorldSectorDesc>& outPackedSectors)
    {
        // TODO: better hierarchical approach

        for (auto node  : m_allNodes)
            packNode(node, outPackedSectors);
    }

    //--

} // scene
