/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#pragma once

#include "uiElementLayout.h"

BEGIN_BOOMER_NAMESPACE(ui)

/// hit cache for rendered element hierarchy, contains informations WHERE windows were rendered
/// can be utilized to find window at give absolute position without walking the rendering hierarchy again
class BASE_UI_API HitCache : public base::NoCopy
{
public:
    HitCache();

    /// reset cache, a top-level bounding box for the cached region should be provided
    /// elements outside this region will not be cached
    void reset(const ElementArea& topLevelArea);

    /// add information about ui element at given area
    /// NOTE: parent must be added before children are (obviously)
    /// NOTE: the stored area must be clipped area, not the draw area
    void storeElement(const ElementArea& clippedDrawArea, IElement* element);

    //--

    /// get all element chains at given position
    void traceAllElements(const Position& absolutePosition, base::Array<ElementPtr>& outAllElements) const;

    /// get top element at given position
    ElementPtr traceElement(const Position& absolutePosition) const;

    //--

    /// disable hit test collection for all following elements (must call enableHitTest to reenable), NOTE: stacked
    void disableHitTestCollection();

    /// reenable hit test collection
    void enableHitTestCollection();

private:
    static const uint32_t CELL_SIZE = 1 << 7; // 64;

    struct Entry
    {
        ElementArea m_area;
        ElementWeakPtr m_ptr;

        Entry(const ElementArea& area, IElement* ptr);
        ~Entry();
    };

    struct GridCell
    {
        typedef base::Array<uint16_t> TEntries;
        TEntries m_entries;
    };

    // disabled flags
    uint64_t m_hitTestDisabledFlags = 0;
    uint8_t m_hitTestStackLevels = 0;

    // top level area we are tracking
    ElementArea m_topArea;
    uint32_t m_numCellsX;
    uint32_t m_numCellsY;

    // lookup map
    typedef base::HashMap<ElementWeakPtr, uint16_t> TElementLookupMap;
    TElementLookupMap m_elementMap;

    // stored element informations
    typedef base::Array<Entry> TEntries;
    TEntries m_entries;

    // grid cells, number depends on the size of the area to track
    typedef base::Array<GridCell> TCells;
    TCells m_cells;
};

END_BOOMER_NAMESPACE(ui)