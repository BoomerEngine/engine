/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input\generic #]
***/

#include "build.h"
#include "inputGenericMouse.h"
#include "inputGenericKeyboard.h"

BEGIN_BOOMER_NAMESPACE()

//--

GenericMouse::GenericMouse(IInputContext* context, GenericKeyboard* keyboard, InputDeviceID id)
    : m_lastEventWindowPoint(0,0)
    , m_lastEventAbsolutePoint(0, 0)
    , m_lastClickWasDoubleClick(false)
    , m_lastMouseMoveValid(false)
    , m_hasMouseMovement(false)
    , m_context(context)
    , m_keyboard(keyboard)
    , m_id(id)
{
    m_mouseKeyMapping[0] = InputKey::KEY_MOUSE0;
    m_mouseKeyMapping[1] = InputKey::KEY_MOUSE1;
    m_mouseKeyMapping[2] = InputKey::KEY_MOUSE2;
    m_mouseKeyMapping[3] = InputKey::KEY_MOUSE3;
    m_mouseKeyMapping[4] = InputKey::KEY_MOUSE4;
    m_mouseKeyMapping[5] = InputKey::KEY_MOUSE5;
    m_mouseKeyMapping[6] = InputKey::KEY_MOUSE6;
    m_mouseKeyMapping[7] = InputKey::KEY_MOUSE7;
    m_mouseKeyMapping[8] = InputKey::KEY_MOUSE8;
    m_mouseKeyMapping[9] = InputKey::KEY_MOUSE9;
    m_mouseKeyMapping[10] = InputKey::KEY_MOUSE10;
    m_mouseKeyMapping[11] = InputKey::KEY_MOUSE11;
    m_mouseKeyMapping[12] = InputKey::KEY_MOUSE12;
    m_mouseKeyMapping[13] = InputKey::KEY_MOUSE13;
    m_mouseKeyMapping[14] = InputKey::KEY_MOUSE14;
    m_mouseKeyMapping[15] = InputKey::KEY_MOUSE15;
}       

InputKeyMask GenericMouse::currentKeyMask() const
{
    InputKeyMask mask;

    if (TestBit(&m_mouseButtonKeys, 0))
        mask |= InputKeyMaskBit::LEFT_MOUSE;
	if (TestBit(&m_mouseButtonKeys, 1))
        mask |= InputKeyMaskBit::MIDDLE_MOUSE;
	if (TestBit(&m_mouseButtonKeys, 2))
        mask |= InputKeyMaskBit::RIGHT_MOUSE;

    if (m_keyboard)
        mask |= m_keyboard->currentKeyMask();

    return mask;
}

bool GenericMouse::canEmitDoubleClick(MouseButtonIndex buttonIndex, const Point& windowPoint, const Point& absolutePoint) const
{
    // we must be in the same window
    if (m_lastClickWasDoubleClick)
        return false;

    // not to far away
    auto maxDoubleClickDistance = 10.0f * 10.0f;
    auto dist = absolutePoint.toVector().squareDistance(m_lastClickEventAbsolutePoint.toVector());
    if (dist >= maxDoubleClickDistance)
        return false;

    // in a small time window
    auto doubleClickTimeWindow = NativeTimeInterval(0.3f);
    auto curTime = NativeTimePoint::Now();
    if ((curTime - m_lastEventTime) > doubleClickTimeWindow)
        return false;

    // we can interpret the event as double click
    return true;
}

void GenericMouse::mouseDown(MouseButtonIndex buttonIndex, const Point& windowPoint, const Point& absolutePoint)
{
    ASSERT(buttonIndex < MAX_MOUSE_KEYS);

    // send event
    auto keyMask = currentKeyMask();
    auto keyCode = m_mouseKeyMapping[buttonIndex];

    // press only if no previous press was registered
    if (!TestBit(&m_mouseButtonKeys, buttonIndex))
    {
        SetBit(&m_mouseButtonKeys, buttonIndex);

        // test for double clicks
        if (buttonIndex == 0 && canEmitDoubleClick(buttonIndex, windowPoint, absolutePoint))
        {
            m_context->inject(RefNew<InputMouseClickEvent>(m_id, keyCode, MouseEventType::DoubleClick, keyMask, windowPoint, absolutePoint));
            m_lastClickWasDoubleClick = true;
        }
        else
        {
            m_context->inject(RefNew<InputMouseClickEvent>(m_id, keyCode, MouseEventType::Click, keyMask, windowPoint, absolutePoint));
            m_context->inject(RefNew<InputKeyEvent>(InputDeviceType::Mouse, m_id, keyCode, true, false, keyMask));
            m_lastClickWasDoubleClick = false;
        }
    }

    // remember event
    m_lastEventTime = NativeTimePoint::Now();
    m_lastEventWindowPoint = windowPoint;
    m_lastEventAbsolutePoint = absolutePoint;
    m_lastClickEventAbsolutePoint = absolutePoint;
}

void GenericMouse::mouseUp(MouseButtonIndex buttonIndex, const Point& windowPoint, const Point& absolutePoint)
{
    ASSERT(buttonIndex < MAX_MOUSE_KEYS);

    if (TestBit(&m_mouseButtonKeys, buttonIndex))
    {
        auto keyMask = currentKeyMask();
        auto keyCode = m_mouseKeyMapping[buttonIndex];

        // track event positions
        m_lastEventWindowPoint = windowPoint;
        m_lastEventAbsolutePoint = absolutePoint;

        // release only if press was registered
        {
            ClearBit(&m_mouseButtonKeys, buttonIndex);
            m_context->inject(RefNew<InputMouseClickEvent>(m_id, keyCode, MouseEventType::Release, keyMask, windowPoint, absolutePoint));
            m_context->inject(RefNew<InputKeyEvent>(InputDeviceType::Mouse, m_id, keyCode, false, false, keyMask));
        }
    }
}

void GenericMouse::mouseMovement(const Point& windowPoint, const Point& absolutePoint, const Vector3& delta, bool captured)
{
    // track event positions
    m_lastEventWindowPoint = windowPoint;
    m_lastEventAbsolutePoint = absolutePoint;

    // send the displacement event
    if (0.0f != delta.x)
    {
        SetBit(&m_movementAxisPerturbed, (uint8_t)InputAxis::AXIS_MOUSEX);
        m_context->inject(RefNew<InputAxisEvent>(InputDeviceType::Mouse, m_id, InputAxis::AXIS_MOUSEX, delta.x));
    }
    if (0.0f != delta.y)
    {
		SetBit(&m_movementAxisPerturbed, (uint8_t)InputAxis::AXIS_MOUSEY);
        m_context->inject(RefNew<InputAxisEvent>(InputDeviceType::Mouse, m_id, InputAxis::AXIS_MOUSEY, delta.y));
    }
    if (0.0f != delta.z)
    {
		SetBit(&m_movementAxisPerturbed, (uint8_t)InputAxis::AXIS_MOUSEZ);
        m_context->inject(RefNew<InputAxisEvent>(InputDeviceType::Mouse, m_id, InputAxis::AXIS_MOUSEZ, delta.z));
    }

    // send the mouse movement event
    auto keyMask = currentKeyMask();
    m_context->inject(RefNew<InputMouseMovementEvent>(m_id, keyMask, captured, windowPoint, absolutePoint, delta));
    m_lastEventKeyMask = keyMask;
    m_lastMouseMoveValid = true;
    m_hasMouseMovement = true;
}
        
void GenericMouse::reset(bool postReleaseEvents)
{
	if (auto pressedbuttons = m_mouseButtonKeys)
	{
		m_mouseButtonKeys = 0;

        if (postReleaseEvents)
        {
            for (uint32_t i = 0; i < MAX_MOUSE_KEYS; ++i)
            {
                if (TestBit(&m_mouseButtonKeys, i))
                {
                    auto keyCode = m_mouseKeyMapping[i];

                    m_context->inject(RefNew<InputMouseClickEvent>(m_id, keyCode, MouseEventType::Release, InputKeyMask(), m_lastEventWindowPoint, m_lastEventAbsolutePoint));
                    m_context->inject(RefNew<InputKeyEvent>(InputDeviceType::Mouse, m_id, keyCode, false, false, InputKeyMask()));
                }
            }
        }
	}

    m_lastMouseMoveValid = false;
}

void GenericMouse::update()
{
    // for all movement axis that were not perturbed previously but not in this frame send a termination event (0 displacement)
    // this simplifies the receiving code a lot
    for (uint32_t i = 0; i < MAX_AXIS; ++i)
    {
        if (TestBit(&m_movementAxisPerturbed, i))
        {
            SetBit(&m_movementAxisTracked, i);
        }
        else if (TestBit(&m_movementAxisTracked, i))
        {
            m_context->inject(RefNew<InputAxisEvent>(InputDeviceType::Mouse, m_id, (InputAxis)i, 0.0f));
            ClearBit(&m_movementAxisTracked, i);
        }
    }

    // if the control key mask changed since last mouse move emit it again
    if (m_lastMouseMoveValid)
    {
        auto controlKeyMask = currentKeyMask();
        if (controlKeyMask != m_lastEventKeyMask)
        {
            m_lastEventKeyMask = controlKeyMask;

            m_context->inject(RefNew<InputMouseMovementEvent>(m_id, controlKeyMask, false, m_lastEventWindowPoint, m_lastEventAbsolutePoint, Vector3::ZERO()));
        }
    }
	m_movementAxisPerturbed = 0;
}

//--

END_BOOMER_NAMESPACE()
