/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"

#include "uiColumnHeaderBar.h"
#include "uiColumnHeaderItem.h"

#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiInputAction.h"

namespace ui
{

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ColumnHeader);
        RTTI_METADATA(ElementClassNameMetadata).name("ColumnHeader");
    RTTI_END_TYPE();

    ColumnHeader::ColumnHeader(base::StringView<char> text, bool canResize, bool canSort)
        : m_canResize(canResize)
        , m_canSort(canSort)
    {
        createChild<TextLabel>(text);

        if (canSort)
            m_sortingIcon = createNamedChild<TextLabel>("SortingIcon"_id);
    }
    
    void ColumnHeader::refreshSortingVisualization(bool active, bool asc)
    {
        if (m_sortingIcon)
        {
            if (active)
            {
                if (asc)
                {
                    m_sortingIcon->addStyleClass("ascending"_id);
                    m_sortingIcon->removeStyleClass("descending"_id);
                }
                else
                {
                    m_sortingIcon->addStyleClass("descending"_id);
                    m_sortingIcon->removeStyleClass("ascending"_id);
                }
            }
            else
            {
                m_sortingIcon->removeStyleClass("ascending"_id);
                m_sortingIcon->removeStyleClass("descending"_id);
            }
        }
    }

    //--

} // ui