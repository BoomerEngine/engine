/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#include "build.h"
#include "world.h"
#include "worldComponent.h"
#include "worldComponentGraph.h"
#include "worldEntity.h"

#include "base/containers/include/bitSet.h"

namespace base
{
    namespace world
    {

        //--

        ComponentGraph::ComponentGraph()
        {}

        void ComponentGraph::clear()
        {

        }

        bool ComponentGraph::addLink(short source, short target)
        {
            // check for duplicates
            for (const auto& link : m_links)
                if (link.source == source && link.target == target)
                    return false; // two components can only be connected once

            // TODO: check for cycles

            invalidateOrder();

            auto& entry = m_links.emplaceBack();
            entry.source = source;
            entry.target = target;

            return true;
        }

        int ComponentGraph::findLinkIndex(short source, short target) const
        {
            const auto* data = m_links.typedData();
            const auto* dataEnd = data + m_links.size();
            while (data < dataEnd)
            {
                if (data->source == source && data->target == target)
                    return (int)(data - m_links.typedData());
                ++data;
            }

            return INDEX_NONE;
        }

        bool ComponentGraph::removeLink(short source, short target)
        {
            const auto index = findLinkIndex(source, target);
            if (index != INDEX_NONE)
            {
                m_links.erase(index);
                return true;
            }

            return false;
        }

        bool ComponentGraph::hasLink(short source, short target) const
        {
            const auto index = findLinkIndex(source, target);
            return index != INDEX_NONE;
        }

        void ComponentGraph::reindexRemovedComponent(short deletedIndex)
        {
            removeAllLinks(deletedIndex);

            for (auto& entry : m_links)
            {
                DEBUG_CHECK(entry.source != deletedIndex);
                DEBUG_CHECK(entry.target != deletedIndex);

                if (entry.source > deletedIndex)
                    entry.source -= 1;

                if (entry.target > deletedIndex)
                    entry.target -= 1;
            }
        }

        void ComponentGraph::removeOutgoingLinks(short index)
        {
            for (int i = m_links.lastValidIndex(); i >= 0; --i)
            {
                if (m_links[i].source == index)
                {
                    m_links[i] = m_links.back();
                    m_links.popBack();
                }
            }
        }

        void ComponentGraph::removeIncomingLinks(short index)
        {
            for (int i = m_links.lastValidIndex(); i >= 0; --i)
            {
                if (m_links[i].target == index)
                {
                    m_links[i] = m_links.back();
                    m_links.popBack();
                }
            }
        }

        void ComponentGraph::removeAllLinks(short componentIndex)
        {
            for (int i = m_links.lastValidIndex(); i >= 0; --i)
            {
                if (m_links[i].target == componentIndex || m_links[i].source == componentIndex)
                {
                    m_links[i] = m_links.back();
                    m_links.popBack();
                }
            }
        }

        //--

        void ComponentGraph::invalidateOrder()
        {
            m_updateOrderInvalid = true;
        }

        void ComponentGraph::buildOrder()
        {
            m_updateOrderInvalid = false;
        }

        //--

        const Component** ComponentGraph::copyIncomingLinks(Component** componentTable, short componentIndex, uint8_t count, const short*& indexPtr)
        {
            if (!count)
                return nullptr;

             m_tempIncomingList.reserve(count);

             auto* writePtr = m_tempIncomingList.typedData();
             for (uint32_t i = 0; i < count; ++i)
             {
                 const auto& link = m_links[*indexPtr++];
                 DEBUG_CHECK_EX(link.target == componentIndex, "Invalid link in the table");
                 *writePtr++ = componentTable[link.source];
             }

             return  (const Component**)m_tempIncomingList.typedData();
        }

        const Component** ComponentGraph::copyOutgoingLinks(Component** componentTable, short componentIndex, uint8_t count, const short*& indexPtr)
        {
            if (!count)
                return nullptr;

            m_tempOutgoingList.reserve(count);

            auto* writePtr = m_tempOutgoingList.typedData();
            for (uint32_t i = 0; i < count; ++i)
            {
                const auto& link = m_links[*indexPtr++];
                DEBUG_CHECK_EX(link.source == componentIndex, "Invalid link in the table");
                *writePtr++ = componentTable[link.target];
            }

            return (const Component**) m_tempOutgoingList.typedData();
        }

        void ComponentGraph::updateComponents(Component** componentTable, uint16_t count)
        {
            if (!m_updateOrderInvalid)
                buildOrder();

            if (m_links.empty())
            {
                ComponentUpdateContext context;

    /*            for (uint32_t i = 0; i < count; ++i)
                    componentTable[i]->handleHierarchyUpdate(context);*/
            }
            else
            {
                InplaceBitSet<4096> updatedComponents(EBitStateZero::ZERO);
                updatedComponents.resizeWithZeros(count);

                const auto* indexPtr = m_linkIndices.typedData();
                const auto* indexEndPtr = indexPtr + m_linkIndices.size();

                for (const auto& entry : m_order)
                {
                    ComponentUpdateContext context;
                    context.numIncomingLinks = entry.numIncomingLinks;
                    context.incomingDependencies = copyIncomingLinks(componentTable, entry.componentIndex, entry.numIncomingLinks, indexPtr);
                    context.numOutgoingLinks = entry.numOutgoingLinks;
                    context.outgoingDependencies = copyOutgoingLinks(componentTable, entry.componentIndex, entry.numIncomingLinks, indexPtr);

                    //componentTable[entry.componentIndex]->handleHierarchyUpdate(context);
                }

                // update the rest of components (the ones not linked to anything)
                if (m_order.size() != count)
                {
                    updatedComponents.iterateClearBits([count, componentTable](uint32_t index)
                        {
                            ComponentUpdateContext context;
                            /*if (index < count)
                                componentTable[index]->handleHierarchyUpdate(context);*/
                            return false;
                        });
                }
            }
        }

        //--

    } // world
} // base