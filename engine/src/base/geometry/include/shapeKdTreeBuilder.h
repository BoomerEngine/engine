/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#pragma once

#include "shape.h"

namespace base
{
    namespace shape
    {
        namespace kd
        {

            /// source triangle
            struct TreeTriangle
            {
                Vector3 v0;
                Vector3 v1;
                Vector3 v2;
                uint16_t chunk = 0;

                Vector3 center; // computed
            };

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
            class BASE_GEOMETRY_API TreeBuilder
            {
            public:
                TreeBuilder();

                ///---

                /// get the final triangles (ordered)
                INLINE const TreeTriangle* triangles() const { return m_triangles.typedData(); }

                /// get the number of nodes
                INLINE uint32_t numTriangles() const { return m_triangles.size(); }

                /// get the final nodes
                INLINE const TreeNode* nodes() const { return m_nodes.typedData(); }

                /// get the number of nodes
                INLINE uint32_t numNodes() const { return m_nodes.size(); }

                ///---

                /// add triangles to builder, data is filled by the caller
                TreeTriangle* allocTriangles(uint32_t count);

                ///---

                /// build the tree from current triangles
                void build(const TreeBuildingSetup& config);

            private:
                Array<TreeTriangle> m_triangles;
                Array<TreeNode> m_nodes;

                // allocate a node
                int allocNodeIndex();

                // build tree for given triangle range
                void buildTree(int nodeIndex, uint32_t firstTriangle, uint32_t lastTriangle, const TreeBuildingSetup& config);

                // find a best split axis for triangles
                void findSplitAxis(uint32_t firstTriangle, uint32_t lastTriangle, uint32_t& outSplitIndex);
            };

        } // kd
    } // shape
} // base