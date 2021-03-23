/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiButton.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

class ColumnHeader;
class ColumnSizeChangeInputAction;

/// a simple header with column names
class ENGINE_UI_API ColumnHeaderBar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ColumnHeaderBar, IElement);

public:
    ColumnHeaderBar();

    //--

    /// get the currently selected sorting column
    INLINE int sortingColumn() const { return m_sortingColumn; }

    /// is the sorting ascending or descending ?
    INLINE bool isSortingAscending() const { return m_sortingAscending; }

    //--

    // toggle sorting by given column
    void sort(int columnIndex, bool asc = true, bool callEvent = true);

    //--

    // add column
    void addColumn(StringView caption, float width, bool center = false, bool allowSort = true, bool allowResize = true);


private:
    virtual void prepareDynamicSizing(const ElementArea& drawArea, const ElementDynamicSizing*& dataPtr) const override;
    virtual InputActionPtr handleOverlayMouseClick(const ElementArea& area, const InputMouseClickEvent& evt) override;
    virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, CursorType& outCursorType) const override;

    //---

    int findSplitterColumnIndex(const ElementArea& area, const Position& absolutePosition) const;
    int findColumnIndex(IElement* ptr) const;

    void adjustColumnWidthInternal(int columnIndex, float pos);

    //--

    struct ColumnInfo
    {
        RefPtr<ColumnHeader> m_header;
        float m_currentWidth = 100.0f;
        float m_minWidth = 0.0f;
        float m_maxWidth = 0.0f;
        bool m_canResize = true;
        bool m_canSort = false;
        bool m_center = false;
    };

    Array<ColumnInfo> m_columns;

    int m_sortingColumn;
    bool m_sortingAscending;
    float m_splitterSnapDistance;

    friend class ColumnSizeChangeInputAction;

    //--

    mutable Array<ElementDynamicSizing::ColumnInfo> m_cachedColumnAreas;
    mutable ElementDynamicSizing m_cachedSizingData;

    //--
};

END_BOOMER_NAMESPACE_EX(ui)
