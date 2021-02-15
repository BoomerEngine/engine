/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#include "build.h"
#include "uiElementHitCache.h"
#include "uiElementLayout.h"
#include "uiElement.h"

namespace ui
{
    //--

    HitCache::Entry::Entry(const ElementArea& area, IElement* ptr)
        : m_area(area)
        , m_ptr(ptr)
    {}

    HitCache::Entry::~Entry()
    {}

    //--

    HitCache::HitCache()
        : m_numCellsX(1)
        , m_numCellsY(1)
    {
        auto maxWidth = 8192 * 3;
        auto maxHeight = 4096;
        m_cells.reserve((maxWidth / CELL_SIZE) * (maxHeight / CELL_SIZE));
        m_cells.resize(1);
        m_entries.reserve(1024);
    }

    void HitCache::reset(const ElementArea& topLevelArea)
    {
        for (auto& cells : m_cells)
            cells.m_entries.reset();

        m_topArea = topLevelArea;
        m_numCellsX = (uint32_t)std::max<float>(1.0f, ceil(topLevelArea.size().x / (float)CELL_SIZE));
        m_numCellsY = (uint32_t)std::max<float>(1.0f, ceil(topLevelArea.size().y / (float)CELL_SIZE));
        m_cells.resize(m_numCellsX * m_numCellsY);
        m_entries.reset();
        m_elementMap.clear();
    }

    void HitCache::disableHitTestCollection()
    {
        DEBUG_CHECK_EX(m_hitTestStackLevels < 64, "To many disabled hit tests");
        if (m_hitTestStackLevels < 64)
        {
            m_hitTestDisabledFlags <<= 1;
            m_hitTestDisabledFlags |= 1;
        }
        m_hitTestStackLevels += 1;
    }

    void HitCache::enableHitTestCollection()
    {
        DEBUG_CHECK_EX(m_hitTestStackLevels > 0, "Hit test flag stack is empty");
        if (m_hitTestStackLevels > 0)
        {
            if (m_hitTestStackLevels <= 64)
                m_hitTestDisabledFlags >>= 1;
            m_hitTestStackLevels -= 1;
        }
    }

    void HitCache::storeElement(const ElementArea& clippedDrawArea, IElement* element)
    {
        // do not add anything if hit testing is disabled
        if (m_hitTestDisabledFlags != 0)
            return;

        // do not store empty area
        if (clippedDrawArea.empty())
            return;

        // add entry
        auto retId = range_cast<uint16_t>(m_entries.size());
        m_entries.emplaceBack(Entry(clippedDrawArea, element));

        // store reference to element in the map for future lookup
        // element hierarchies are recovered for keyboard interactions
        ASSERT(!m_elementMap.contains(ElementWeakPtr(element)));
        m_elementMap.set(ElementWeakPtr(element), retId);

        // offset to the tracked area 
        auto startX = clippedDrawArea.absolutePosition().x - m_topArea.absolutePosition().x;
        auto startY = clippedDrawArea.absolutePosition().y - m_topArea.absolutePosition().y;
        auto endX = startX + clippedDrawArea.size().x;
        auto endY = startY + clippedDrawArea.size().y;

        // convert to grid range
        auto cellScale = 1.0f / (float)CELL_SIZE;
        auto firstCellX = std::clamp<int>((int)floor(startX * cellScale), 0, m_numCellsX - 1);
        auto firstCellY = std::clamp<int>((int)floor(startY * cellScale), 0, m_numCellsY - 1);
        auto lastCellX = std::clamp<int>((int)ceil(endX * cellScale), 1, m_numCellsX);
        auto lastCellY = std::clamp<int>((int)ceil(endY * cellScale), 1, m_numCellsY);

        // add to grid cells
        for (auto y = firstCellY; y < lastCellY; ++y)
        {
            for (auto x = firstCellX; x < lastCellX; ++x)
            {
                auto& cell = m_cells[x + y*m_numCellsX];
                cell.m_entries.pushBack(retId);
            }
        }
    }

    ElementPtr HitCache::traceElement(const Position& absolutePosition) const
    {
        // point must be inside the area
        if (!m_topArea.contains(absolutePosition))
            return nullptr;

        // compute grid cell index
        auto cellScale = 1.0f / (float)CELL_SIZE;
        auto cellX = std::clamp<int>((int)((absolutePosition.x - m_topArea.absolutePosition().x) * cellScale), 0, m_numCellsX - 1);
        auto cellY = std::clamp<int>((int)((absolutePosition.y - m_topArea.absolutePosition().y) * cellScale), 0, m_numCellsY - 1);

        // test all entries from the area, return the deepest one hit
        const auto& cell = m_cells[cellX + cellY*m_numCellsX];
        for (int i = (int)cell.m_entries.size()-1; i >= 0; --i)
        {
            const auto& entry = m_entries[cell.m_entries[i]];
            if (entry.m_area.contains(absolutePosition))
                return entry.m_ptr.lock();
        }

        // no entry found
        return nullptr;
    }

    void HitCache::traceAllElements(const Position& absolutePosition, base::Array<ElementPtr>& outAllChains) const
    {
        // point must be inside the area
        if (m_topArea.contains(absolutePosition))
        {
            // compute grid cell index
            auto cellScale = 1.0f / (float)CELL_SIZE;
            auto cellX = std::clamp<int>((int)((absolutePosition.x - m_topArea.absolutePosition().x) * cellScale), 0, m_numCellsX - 1);
            auto cellY = std::clamp<int>((int)((absolutePosition.y - m_topArea.absolutePosition().y) * cellScale), 0, m_numCellsY - 1);

            // test all entries from the area, return the deepest one hit
            const auto& cell = m_cells[cellX + cellY*m_numCellsX];
            for (int i = (int)cell.m_entries.size() - 1; i >= 0; --i)
            {
                const auto& entry = m_entries[cell.m_entries[i]];
                if (entry.m_area.contains(absolutePosition))
                {
                    if (auto elem = entry.m_ptr.lock())
                        outAllChains.pushBack(elem);
                }
            }
        }
    }

    //--

} // ui