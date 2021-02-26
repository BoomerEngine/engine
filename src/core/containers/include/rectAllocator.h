/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

/// Helper class used to allocate rectangles in 2D space
class CORE_CONTAINERS_API RectAllocator : public NoCopy
{
public:
    RectAllocator();

    INLINE uint32_t width() const { return m_width; }
    INLINE uint32_t height() const { return m_height; }

    //--

    //! allocate space in the controlled area, returns true if the allocation was successful
    bool allocate(uint32_t width, uint32_t height, uint32_t& outOffsetX, uint32_t& outOffsetY);

    //--

    //! reset allocator, a size of the region must be specified
    void reset(uint32_t width, uint32_t height);

    //! resize the allocator without resetting existing allocations, new size must be bigger
    void resize(uint32_t width, uint32_t height);

    //--

private:
    typedef short NodeID;

    /// internal node of the binary tree
    struct Node
    {
        NodeID children[2];
        uint16_t offset[2];
        uint16_t size[2];
        uint32_t spaceLeft = 0;

        INLINE Node(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
            : spaceLeft(width * height)
        {
            children[0] = -1;
            children[1] = -1;
            offset[0] = x;
            offset[1] = y;
            size[0] = width;
            size[1] = height;
        }

        INLINE bool leaf() const
        {
            return children[0] == -1 && children[1] == -1;
        }

        INLINE bool willFit(uint32_t width, uint32_t height) const
        {
            return width <= size[0] && height <= size[1];
        }
    };

    struct NodeInfo
    {
        NodeID id = -1;
        uint32_t area = INDEX_MAX;

        INLINE static NodeInfo SelectBest(const NodeInfo& a, const NodeInfo& b)
        {
            // select node with smaller area, this way we minimize the space wastage
            return (a.area < b.area) ? a : b;
        }

    };

    /// find best node to accommodate for given area size, returns -1 if no nodes with enough space were found
    /// this function does not change the tree
    NodeInfo findBestNode(NodeID curNode, uint32_t width, uint32_t height) const;

    /// split node producing the area for the content and most likely leftover nodes
    /// this function does change the tree
    void splitNode(NodeID node, uint32_t width, uint32_t height, uint32_t& outOffsetX, uint32_t& outOffsetY);

    //--

    uint32_t  m_width;
    uint32_t  m_height;

    typedef Array<Node> TNodes;
    TNodes m_nodes;

    void resizeHorizontal(uint32_t newWidth);
    void resizeVertical(uint32_t newHeight);
};

END_BOOMER_NAMESPACE()
