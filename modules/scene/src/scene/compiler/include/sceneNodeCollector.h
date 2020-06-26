/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\build #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "scene/common/include/sceneNodePath.h"
#include "scene/common/include/sceneNodeTemplate.h"
#include "scene/common/include/sceneWorld.h"

namespace scene
{

    //--

    /// extracted scene entity
    struct SCENE_COMPILER_API ExtractedNode : public base::NoCopy
    {
        // streaming model
        StreamingModel m_streamingModel = StreamingModel::Auto;
        float m_streamingOverrideDistance = 0.0f;

        // data for the node, not owned by anybody
        // NOTE: this is always defined
        EntityPtr m_entity;

        // parent node
        ExtractedNode* m_parent = nullptr;

        // node path
        NodePath m_path;

        // child nodes
        base::Array<ExtractedNode*> m_children;

        // linked nodes
        base::Array<ExtractedNode*> m_links;
    };

    //--

    /// helper class to aid in node extraction
    class SCENE_COMPILER_API NodeCollector : public base::NoCopy
    {
    public:
        NodeCollector();
        ~NodeCollector();

        INLINE uint32_t nodeCount() const { return m_allNodes.size(); }

        // extract content
        void extractNodesFromLayer(const base::res::ResourcePath& path);

        // pack nodes into sectors
        void pack(base::Array<WorldSectorDesc>& outPackedSectors);

    private:
        base::Array<scene::PrefabPtr> m_loadedPrefabs;
        base::Array<scene::LayerPtr> m_loadedLayers;

        base::Array<ExtractedNode*> m_allNodes;
        base::Array<ExtractedNode*> m_rootNodes;

        //--

        void extractNode(const NodeTemplateContainerPtr& ptr, int nodeIndex, ExtractedNode* parentNode, const NodePath& parentPath);

        uint8_t gridLevelForStreamingDistance(float distance) const;
        base::Point gridCooridinateForPosition(const base::AbsolutePosition& pos, uint8_t gridIndex) const;
        base::Box streamingBox(const base::Point& grid, uint8_t gridIndex) const;

        WorldSectorDesc* alwaysLoadedSector(base::Array<WorldSectorDesc>& outPackedSectors);
        WorldSectorDesc* gridSector(base::Array<WorldSectorDesc>& outPackedSectors, uint8_t gridIndex, const base::Point& grid);

        void packNode(const ExtractedNode* node, base::Array<WorldSectorDesc>& outPackedSectors);
        void packNodeIntoSector(const ExtractedNode* node, WorldSectorDesc* sector);
    };

    //--

} // scene