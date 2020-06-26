/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#include "build.h"
#include "rectAllocator.h"

namespace base
{

    RectAllocator::RectAllocator()
        : m_width(0)
        , m_height(0)
    {}

    void RectAllocator::reset(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
        m_nodes.clear();
        m_nodes.emplaceBack(0, 0, m_width, m_height);
    }

    bool RectAllocator::allocate(uint32_t width, uint32_t height, uint32_t& outOffsetX, uint32_t& outOffsetY)
    {
        // find node that will fit the area
        auto node = findBestNode(0, width, height);
        if (node.id == -1)
            return false; // we don't have space in the tree for the new area

        // split the node carving the allocated space
        // this finalizes the placement of the allocated region
        splitNode(node.id, width, height, outOffsetX, outOffsetY);
        return true;
    }

    void RectAllocator::resize(uint32_t newWidth, uint32_t newHeight)
    {
        ASSERT(newWidth >= m_width && newHeight >= m_height);
        ASSERT(newWidth > m_width || newHeight > m_height);

        // determine size of the additional area
        auto extraWidth = newWidth - m_width;
        auto extraHeight = newHeight - m_height;

        // resize smaller size first
        if (extraWidth < extraHeight)
        {
            resizeHorizontal(extraWidth);
            resizeVertical(extraHeight);
        }
        else
        {
            resizeVertical(extraHeight);
            resizeHorizontal(extraWidth);
        }
    }

    void RectAllocator::resizeHorizontal(uint32_t newWidth)
    {
        auto extraWidth = newWidth - m_width;

        // copy root to new location
        auto firstExtraNode = m_nodes.size();
        auto rootNode = m_nodes[0];
        m_nodes.pushBack(rootNode); // original area on the left
        m_nodes.pushBack(Node(m_width, 0, extraWidth, m_height)); // free area on the right

        // set new root node
        m_nodes[0] = Node(0, 0, newWidth, m_height);

        // setup new root
        auto& newRoot = m_nodes[0];
        newRoot.children[0] = firstExtraNode + 0; // old area
        newRoot.children[1] = firstExtraNode + 1; // new area
        newRoot.spaceLeft = m_nodes[newRoot.children[0]].spaceLeft + m_nodes[newRoot.children[1]].spaceLeft;

        // set new size
        m_width = newWidth;
    }

    void RectAllocator::resizeVertical(uint32_t newHeight)
    {
        auto extraHeight = newHeight - m_height;

        // copy root to new location
        auto firstExtraNode = m_nodes.size();
        auto rootNode = m_nodes[0];
        m_nodes.pushBack(rootNode); // original area on the top
        m_nodes.pushBack(Node(0, m_height, m_width, extraHeight)); // free area on the bottom

        // set new root node
        m_nodes[0] = Node(0, 0, m_width, newHeight);

        // setup new root
        auto& newRoot = m_nodes[0];
        newRoot.children[0] = firstExtraNode + 0; // old area
        newRoot.children[1] = firstExtraNode + 1; // new area
        newRoot.spaceLeft = m_nodes[newRoot.children[0]].spaceLeft + m_nodes[newRoot.children[1]].spaceLeft;

        // set new size
        m_height = newHeight;
    }

    RectAllocator::NodeInfo RectAllocator::findBestNode(NodeID nodeId, uint32_t width, uint32_t height) const
    {
        const Node& node = m_nodes[nodeId];

        // Node (and children) does not have enough free space
        if (node.spaceLeft < (width * height))
            return NodeInfo();

        // Check if the node's absolute size is enough
        if (!node.willFit(width, height))
            return NodeInfo();

        // if we've found a leaf return the information about the node
        if (node.leaf())
            return { nodeId, node.spaceLeft };

        // recurse to children and select best child
        return NodeInfo::SelectBest(findBestNode(node.children[0], width, height),
            findBestNode(node.children[1], width, height));
    }

    void RectAllocator::splitNode(NodeID nodeId, uint32_t width, uint32_t height, uint32_t& outOffsetX, uint32_t& outOffsetY)
    {
        // Get node data
        auto& node = m_nodes[nodeId]; // BEWARE: do not access the reference AFTER the nodes was resized
        outOffsetX = node.offset[0];
        outOffsetY = node.offset[1];

        // deduce node free space
        ASSERT(node.spaceLeft >= (width*height));
        node.spaceLeft -= width * height;

        // determine the leftovers
        auto sizeX = node.size[0];
        auto sizeY = node.size[1];
        auto leftX = sizeX - width;
        auto leftY = sizeY - height;
        if (leftX > 0 && leftY > 0) // we have to parts left, we need to allocate child nodes
        {
            // we alway create 2 child nodes
            node.children[0] = m_nodes.size();
            node.children[1] = m_nodes.size() + 1;

            // split is based on the where we have more free space left, we try to preserve the bigger region
            if (leftX > leftY) // we have more on the X side,
            {
                m_nodes.pushBack(Node(outOffsetX + width, outOffsetY, sizeX - width, sizeY)); // big chunk on the left of the allocated area, goes all way down
                m_nodes.pushBack(Node(outOffsetX, outOffsetY + height, width, sizeY - height)); // smaller part just under the allocated area
            }
            else
            {
                m_nodes.pushBack(Node(outOffsetX + width, outOffsetY, sizeX - width, height)); // smaller chunk on the left of the allocated area
                m_nodes.pushBack(Node(outOffsetX, outOffsetY + height, sizeX, sizeY - height)); // big chunk at the bottom of the allocated area, going all the way
            }

            // check the sizes, this validates the split
            auto& leftChild = m_nodes[m_nodes[nodeId].children[0]];
            auto& rightChild  = m_nodes[m_nodes[nodeId].children[1]];
            ASSERT(m_nodes[nodeId].spaceLeft == leftChild.spaceLeft + rightChild.spaceLeft);
        }
        else if (leftX > 0 && leftY == 0) // we have only leftovers in the X axis, update existing node
        {
            ASSERT(node.size[0] > leftX);
            node.offset[0] += width;
            node.size[0] -= width;
            ASSERT(node.spaceLeft == (node.size[0] * node.size[1]));
        }
        else if (leftY > 0 && leftX == 0) // we have only leftovers in the Y axis, update existing node
        {
            ASSERT(node.size[1] > leftY);
            node.offset[1] += height;
            node.size[1] -= height;
            ASSERT(node.spaceLeft == (node.size[0] * node.size[1]));
        }
        else
        {
            // whole node was consumed
            ASSERT(node.spaceLeft == 0);
        }
    }

} // base
