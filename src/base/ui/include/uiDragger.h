/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#pragma once

#include "uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

DECLARE_UI_EVENT(EVENT_VALUE_DRAG_STEP, int)
DECLARE_UI_EVENT(EVENT_VALUE_DRAG_STARTED)
DECLARE_UI_EVENT(EVENT_VALUE_DRAG_FINISHED)
DECLARE_UI_EVENT(EVENT_VALUE_DRAG_CANCELED)

//--

/// helper class to change the numerical value via dragging up/down
class BASE_UI_API Dragger : public TextLabel
{
    RTTI_DECLARE_VIRTUAL_CLASS(Dragger, TextLabel);

public:
    Dragger();

    // emits: OnDrag(stepCount:int), optional: OnDragStarted, OnDragFinished, OnDragCanceled
    // default drag increment is 10 steps, 1 step with Ctrl, 100 with Shift
    // NOTE: the frequency of events as well as dead zone is controlled by config value

protected:
    virtual InputActionPtr handleMouseClick(const ui::ElementArea& area, const base::input::MouseClickEvent& evt) override;
    virtual bool handleMouseWheel(const base::input::MouseMovementEvent& evt, float delta) override;
    virtual bool handleCursorQuery(const ui::ElementArea& area, const ui::Position& absolutePosition, base::input::CursorType& outCursorType) const override;
};

//--

END_BOOMER_NAMESPACE(ui)