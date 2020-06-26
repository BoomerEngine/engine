/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#include "build.h"
#include "shapeUtils.h"
#include "shapeKdTreeBuilder.h"

namespace base
{
    namespace shape
    {
        namespace kd
        {
            //---

            TreeBuilder::TreeBuilder()
            {}

            TreeTriangle* TreeBuilder::allocTriangles(uint32_t count)
            {
                return m_triangles.allocateUninitialized(count);
            }

            void TreeBuilder::build(const TreeBuildingSetup& config)
            {
                ScopeTimer timer;

                // preallocate space for the nodes
                auto numLowLevelNodes = m_triangles.size() / config.numTrianglesPerNode;
                auto estimatedNumberOfNodes = 3 * numLowLevelNodes;
                m_nodes.reserve(estimatedNumberOfNodes);
                m_nodes.reset();

                // compute triangle centers
                for (auto& tri : m_triangles)
                {
                    Vector3 triMin, triMax;
                    triMin = Min(tri.v0, Min(tri.v1, tri.v2));
                    triMax = Max(tri.v0, Max(tri.v1, tri.v2));
                    tri.center = (triMin + triMax) * 0.5f;
                }

                // build nodes from triangles
                auto rootNode = allocNodeIndex();
                buildTree(rootNode, 0, m_triangles.size(), config);

                // final stats
                TRACE_INFO("Build KD tree from {} triangles (generated {} nodes) in {}", m_triangles.size(), m_nodes.size(), TimeInterval(timer.timeElapsed()));
            }

            int TreeBuilder::allocNodeIndex()
            {
                m_nodes.emplaceBack();
                return m_nodes.lastValidIndex();
            }

            void TreeBuilder::findSplitAxis(uint32_t firstTriangle, uint32_t lastTriangle, uint32_t& outSplitIndex)
            {
                // counts
                auto numTriangles = lastTriangle - firstTriangle;
                auto average = 1.0f / (float)numTriangles;

                // calculate center of the all triangles
                Vector3 means(0,0,0);
                {
                    auto triData = m_triangles.typedData() + firstTriangle;
                    for (uint32_t i = firstTriangle; i < lastTriangle; ++i, ++triData)
                        means += triData->center;
                    means *= average;
                }

                // calculate the variance
                Vector3 variance(0,0,0);
                {
                    auto triData = m_triangles.typedData() + firstTriangle;
                    for (uint32_t i = firstTriangle; i < lastTriangle; ++i, ++triData)
                    {
                        auto diff2 = triData->center - means;
                        diff2 = diff2 * diff2;
                        variance += diff2;
                    }
                   // variance *= average;
                }

                // use the axis with the biggest variance for the split
                auto axis = variance.largestAxis();
                auto splitValue = means[axis];

                // find the split index
                auto splitCount = 0;
                {
                    auto triData = m_triangles.typedData() + firstTriangle;
                    for (uint32_t i = firstTriangle; i < lastTriangle; ++i, ++triData)
                        if (triData->center[axis] > splitValue)
                            splitCount += 1;
                }

                // if we are getting to unbalanced then split in the middle
                auto rangeBalancedIndices = numTriangles / 4;
                bool unbalanced = (splitCount <= rangeBalancedIndices) || (splitCount >= (numTriangles - rangeBalancedIndices));
                if (unbalanced)
                {
                    // splitting the tree makes it unbalanced, split in half by count
                    // NOTE: we don't sort since there's no high variance any way (most likely all triangles are crammed)
                    auto center = numTriangles >> 1;
                    outSplitIndex = firstTriangle + center;
                    ASSERT(outSplitIndex != firstTriangle && outSplitIndex != lastTriangle);
                    return;
                }

                // split is kind of balanced, move triangles around
                {
                    auto triData = m_triangles.typedData();
                    auto splitIndex = firstTriangle;
                    for (uint32_t i = firstTriangle; i < lastTriangle; ++i)
                    {
                        if (triData[i].center[axis] > splitValue)
                        {
                            if (i != splitIndex)
                                std::swap(triData[i], triData[splitIndex]);
                            splitIndex += 1;
                        }
                    }

                    outSplitIndex = splitIndex;
                    ASSERT(outSplitIndex != firstTriangle && outSplitIndex != lastTriangle);
                }
            }

            void TreeBuilder::buildTree(int nodeIndex, uint32_t firstTriangle, uint32_t lastTriangle, const TreeBuildingSetup& config)
            {
                // set the triangle range
                auto numTriangles = lastTriangle - firstTriangle;
                m_nodes[nodeIndex].firstTriangle = firstTriangle;
                m_nodes[nodeIndex].numTriangles = numTriangles;

                // if we have reached the triangle limit stop recursion
                if (numTriangles <= config.numTrianglesPerNode)
                    return;

                // find split axis for the triangles
                uint32_t splitIndex;
                findSplitAxis(firstTriangle, lastTriangle, splitIndex);

                // create child nodes
                auto childIndex = allocNodeIndex(); // first child
                FATAL_CHECK(allocNodeIndex() == childIndex+1); // second child
                m_nodes[nodeIndex].childIndex = childIndex;

                // recurse
                buildTree(childIndex+0, firstTriangle, splitIndex, config);
                buildTree(childIndex+1, splitIndex, lastTriangle, config);
            }

        } // kd
    } // shape
} // base