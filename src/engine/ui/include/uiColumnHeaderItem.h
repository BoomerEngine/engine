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

/// element in the column header
class ENGINE_UI_API ColumnHeader : public Button
{
    RTTI_DECLARE_VIRTUAL_CLASS(ColumnHeader, Button);

public:
    ColumnHeader(StringView text, bool canResize=true, bool canSort=true);

    void refreshSortingVisualization(bool active, bool desc);

private:
    ColumnHeader();

    bool m_canSort;
    bool m_canResize;

    RefPtr<TextLabel> m_sortingIcon;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
