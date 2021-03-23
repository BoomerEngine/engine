/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\window #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// Title bar for the window, allows to move the window and, if window supports resizing, also maximize/minimize it
/// NOTE: for best effect add as a first element to the window's sizer :)
class ENGINE_UI_API WindowTitleBar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(WindowTitleBar, IElement);

public:
    WindowTitleBar(WindowFeatureFlags flags, StringView title);

private:
    virtual bool handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, AreaType& outAreaType) const override final;

    ButtonPtr m_minimizeButton;
    ButtonPtr m_maximizeButton;
    ButtonPtr m_closeButton;
    TextLabelPtr m_title;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
