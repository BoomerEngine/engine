/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#pragma once

namespace game
{
    //---

    // graph of links between components
    class GAME_WORLD_API ComponentGraph : public base::NoCopy
    {
    public:
        ComponentGraph();

        //--

        // Remove all links between components
        // NOTE: you might as well just delete the whole update graph
        void clear();

        //--

        // Add dependency between source and target that will make source update before target
        // Returns true when the link is valid and false if link would cause a cycle
        bool addLink(short sourceComponentIndex, short targetComponentIndex);

        // Remove specific link between components, returns true if we had such link
        // NOTE: links are directional
        bool removeLink(short sourceComponentIndex, short targetComponentIndex);

        // Check if have link between components
        // NOTE: links are directional
        bool hasLink(short sourceComponentIndex, short targetComponentIndex) const;

        //--        

        // Reindex links when component X got deleted, will remove any links to removed component as well
        void reindexRemovedComponent(short deletedComponentIndex);

        //--

        // Remove outgoing links for given component, returns true if that happened
        void removeOutgoingLinks(short componentIndex);

        // Remove incoming links for given component, returns true if that happened
        void removeIncomingLinks(short componentIndex);

        // Remove all links of given component
        void removeAllLinks(short componentIndex);

        //--

        // Iterate components in update order
        // NOTE: may rebuild the order of needed
        void updateComponents(Component** componentTable, uint16_t count);

        //--

    private:
        struct LinkEntry
        {
            short source = -1;
            short target = -1;
        };

        struct OrderEntry
        {
            short componentIndex = -1;
            uint8_t numIncomingLinks = 0;
            uint8_t numOutgoingLinks = 0;
        };

        base::Array<LinkEntry> m_links;
        base::Array<short> m_linkIndices;
        base::Array<OrderEntry> m_order;
        bool m_updateOrderInvalid = false;

        base::InplaceArray<Component*, 2> m_tempIncomingList;
        base::InplaceArray<Component*, 2> m_tempOutgoingList;

        void invalidateOrder();
        void buildOrder();

        int findLinkIndex(short source, short target) const;

        const Component** copyIncomingLinks(Component** componentTable, short componentIndex, uint8_t linkCount, const short*& linkIndexPtr);
        const Component** copyOutgoingLinks(Component** componentTable, short componentIndex, uint8_t linkCount, const short*& linkIndexPtr);
    };

    //---

} // game
