/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\trimesh #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

struct TriMeshBuildTriangle;

namespace kd
{
    /// a node in the tree
    struct TreeNode
    {
        uint32_t firstTriangle = 0;
        uint32_t numTriangles = 0;
        uint8_t splitAxis = 0;
        float splitDist = 0.0f;
        int childIndex = -1;
    };

    /// tree building setup
    struct TreeBuildingSetup
    {
        uint32_t numTrianglesPerNode = 16;
    };

    /// a simple KD tree builder
    class BASE_MATH_API TreeBuilder
    {
    public:
        TreeBuilder(TriMeshBuildTriangle* triangles, uint32_t numTriangles, Array<TreeNode>& nodeTable);

        /// build the tree from current triangles
        void build(const TreeBuildingSetup& config);

    private:
        TriMeshBuildTriangle* m_triangles = nullptr;
        uint32_t m_numTriangles = 0;

        Array<TreeNode>& m_nodes;

        // allocate a node
        int allocNodeIndex();

        // build tree for given triangle range
        void buildTree(int nodeIndex, uint32_t firstTriangle, uint32_t lastTriangle, const TreeBuildingSetup& config);

        // find a best split axis for triangles
        void findSplitAxis(uint32_t firstTriangle, uint32_t lastTriangle, uint32_t& outSplitIndex);
    };

} // kd

END_BOOMER_NAMESPACE(base)