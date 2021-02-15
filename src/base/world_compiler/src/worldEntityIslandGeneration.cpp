/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*
***/

#include "build.h"
#include "worldEntitySoup.h"
#include "worldEntityIslandGeneration.h"

namespace base
{
    namespace world
    {

        //---

        Box CalcEntityStreamingBox(const HierarchyEntity& ent)
        {
            const auto center = ent.entity->absoluteTransform().position().approximate();

            if (ent.streamingDistanceOverride > 0)
                return Box(center, ent.streamingDistanceOverride);

            /*Box componentBox;
            bool validBox = false;
            for (const auto& comp : ent.entity->components())
            {
                const auto componentStreamingBounds = comp->calcStreamingBounds();
                if (!componentStreamingBounds.empty())
                {
                    componentBox.merge(componentStreamingBounds);
                    validBox = true;
                }
            }

            if (validBox)
                return componentBox;*/

            return Box(center, 0.1f);
        }

        void CalcIslandStreamingBox(SourceIsland& island)
        {
            Box streamingBox;

            for (const auto& ent : island.flatEntities)
                streamingBox.merge(CalcEntityStreamingBox(*ent));

            island.localStreamingBox = streamingBox;
            island.mergedStreamingBox = streamingBox;

            for (const auto& child : island.children)
            {
                CalcIslandStreamingBox(*child);
                island.mergedStreamingBox.merge(child->mergedStreamingBox);
            }
        }

        bool ShouldExportEntity(HierarchyEntity* ent)
        {
            /*if (!ent->entity->components().empty())
                return true;*/

            if (!ent->children.empty())
                return true;

            if (!ent->entity->cls() != Entity::GetStaticClass())
                return true;

            return false;
        }

        void ExtractSourceIsland(HierarchyEntity* ent, SourceIsland* island)
        {
            island->flatEntities.pushBack(AddRef(ent));

            for (const auto& child : ent->children)
            {
                if (ent->streamingGroupChildren && !child->streamingBreakFromGroup)
                {
                    ExtractSourceIsland(child, island);
                }
                else
                {
                    auto childIsland = RefNew<SourceIsland>();
                    ExtractSourceIsland(child, childIsland);
                    island->children.pushBack(childIsland);
                }
            }
        }

        uint32_t CountIslands(const SourceIsland* island)
        {
            uint32_t ret = 1;

            for (const auto& child : island->children)
                ret += CountIslands(child);

            return ret;
        }

        void ExtractSourceIslands(const SourceEntitySoup& soup, SourceIslands& outSourceIslands)
        {
            ScopeTimer timer;

            // generate most simplistic islands possible
            // TODO: each root can be analyzed on it's own
            uint32_t numTotalIslands = 0;
            for (const auto& root : soup.rootEntities)
            {
                auto island = RefNew<SourceIsland>();

                ExtractSourceIsland(root, island);
                CalcIslandStreamingBox(*island);

                outSourceIslands.rootIslands.pushBack(island);
                numTotalIslands += CountIslands(island);
            }

            // TODO: initialize global entity lookup for link resolving

            // TODO: merge small islands

            // calculate the total streaming area
            outSourceIslands.totalStreamingArea = Box(Vector3(0, 0, 0), 1.0f); // always stream around the root
            outSourceIslands.largestStreamingDistance = 1.0f;
            for (const auto& island : outSourceIslands.rootIslands)
            {
                outSourceIslands.totalStreamingArea.merge(island->mergedStreamingBox);

                const auto streamingSize = island->mergedStreamingBox.extents();
                const auto streamingSizeExt = std::max<float>(streamingSize.x, streamingSize.y);
                outSourceIslands.largestStreamingDistance = std::max<float>(streamingSizeExt, outSourceIslands.largestStreamingDistance);
            }

            TRACE_INFO("Generated {} islands ({} root islands) in {}", numTotalIslands, outSourceIslands.rootIslands.size(), timer);
            TRACE_INFO("Total streaming area from {} to {}", outSourceIslands.totalStreamingArea.min, outSourceIslands.totalStreamingArea.max);
            TRACE_INFO("Largest streaming distance: {}", outSourceIslands.largestStreamingDistance);
        }

        //---

    } // world
} // base



