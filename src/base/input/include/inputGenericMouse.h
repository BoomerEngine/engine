/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input\generic #]
***/

#pragma once

#include "inputContext.h"
#include "inputStructures.h"
#include "base/containers/include/bitUtils.h"

BEGIN_BOOMER_NAMESPACE(base::input)

class GenericKeyboard;

/// generic mouse device helper
class BASE_INPUT_API GenericMouse : public base::NoCopy
{
public:
    GenericMouse(IContext* context, GenericKeyboard* keyboard, DeviceID id=0);

    // reset state
    void reset(bool postReleaseEvents);

    // process persistent state
    void update();

    // emit a mouse button down event
    void mouseDown(MouseButtonIndex buttonIndex, const Point& windowPoint, const Point& absolutePoint);

    // emit a mouse button up event
    void mouseUp(MouseButtonIndex buttonIndex, const Point& windowPoint, const Point& absolutePoint);

    // emit a mouse movement event
    void mouseMovement(const Point& windowPoint, const Point& absolutePoint, const Vector3& delta, bool captured);

    // get the mouse key mask
    KeyMask currentKeyMask() const; // get the current SHIFT/CTRL/ALT key mask

private:
    static const uint32_t MAX_MOUSE_KEYS = 16;
    static const uint32_t MAX_AXIS = 32;

    KeyCode m_mouseKeyMapping[MAX_MOUSE_KEYS];
	BitWord m_mouseButtonKeys = 0;

	BitWord m_movementAxisPerturbed = 0;
	BitWord m_movementAxisTracked = 0;

    NativeTimePoint m_lastEventTime;
    Point m_lastEventWindowPoint;
    Point m_lastEventAbsolutePoint;
    Point m_lastClickEventAbsolutePoint;
    KeyMask m_lastEventKeyMask;
    bool m_lastClickWasDoubleClick;
    bool m_lastMouseMoveValid;
    bool m_hasMouseMovement;

    IContext* m_context;
    DeviceID m_id;

    GenericKeyboard* m_keyboard;

    // test if the given event could be converted into a double click event
    bool canEmitDoubleClick(MouseButtonIndex buttonIndex, const Point& windowPoint, const Point& absolutePoint) const;

    //---
};

END_BOOMER_NAMESPACE(base::input)