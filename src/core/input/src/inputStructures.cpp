/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#include "build.h"
#include "inputStructures.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ENUM(InputEventType);
    RTTI_ENUM_OPTION(None);
    RTTI_ENUM_OPTION(Key);
    RTTI_ENUM_OPTION(Char);
    RTTI_ENUM_OPTION(Axis);
    RTTI_ENUM_OPTION(MouseClick);
    RTTI_ENUM_OPTION(MouseMove);
    RTTI_ENUM_OPTION(MouseCaptureLost);
    RTTI_ENUM_OPTION(DragDrop);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(InputKey);
    RTTI_ENUM_OPTION(KEY_INVALID);
    RTTI_ENUM_OPTION(KEY_BACK);
    RTTI_ENUM_OPTION(KEY_TAB);
    RTTI_ENUM_OPTION(KEY_CLEAR);
    RTTI_ENUM_OPTION(KEY_RETURN);
    RTTI_ENUM_OPTION(KEY_PAUSE);
    RTTI_ENUM_OPTION(KEY_CAPITAL);
    RTTI_ENUM_OPTION(KEY_ESCAPE);
    RTTI_ENUM_OPTION(KEY_CONVERT);
    RTTI_ENUM_OPTION(KEY_NONCONVERT);
    RTTI_ENUM_OPTION(KEY_ACCEPT);
    RTTI_ENUM_OPTION(KEY_MODECHANGE);
    RTTI_ENUM_OPTION(KEY_SPACE);
    RTTI_ENUM_OPTION(KEY_PRIOR);
    RTTI_ENUM_OPTION(KEY_NEXT);
    RTTI_ENUM_OPTION(KEY_END);
    RTTI_ENUM_OPTION(KEY_HOME);
    RTTI_ENUM_OPTION(KEY_LEFT);
    RTTI_ENUM_OPTION(KEY_UP);
    RTTI_ENUM_OPTION(KEY_RIGHT);
    RTTI_ENUM_OPTION(KEY_DOWN);
    RTTI_ENUM_OPTION(KEY_SELECT);
    RTTI_ENUM_OPTION(KEY_PRINT);
    RTTI_ENUM_OPTION(KEY_SNAPSHOT);
    RTTI_ENUM_OPTION(KEY_INSERT);
    RTTI_ENUM_OPTION(KEY_DELETE);
    RTTI_ENUM_OPTION(KEY_HELP);
    RTTI_ENUM_OPTION(KEY_LWIN);
    RTTI_ENUM_OPTION(KEY_RWIN);
    RTTI_ENUM_OPTION(KEY_APPS);
    RTTI_ENUM_OPTION(KEY_SLEEP);
    RTTI_ENUM_OPTION(KEY_MINUS);
    RTTI_ENUM_OPTION(KEY_EQUAL);
    RTTI_ENUM_OPTION(KEY_LBRACKET);
    RTTI_ENUM_OPTION(KEY_RBRACKET);
    RTTI_ENUM_OPTION(KEY_SEMICOLON);
    RTTI_ENUM_OPTION(KEY_APOSTROPHE);
    RTTI_ENUM_OPTION(KEY_COMMA);
    RTTI_ENUM_OPTION(KEY_PERIOD);
    RTTI_ENUM_OPTION(KEY_SLASH);
    RTTI_ENUM_OPTION(KEY_BACKSLASH);
    RTTI_ENUM_OPTION(KEY_GRAVE);
    RTTI_ENUM_OPTION(KEY_NUMPAD0);
    RTTI_ENUM_OPTION(KEY_NUMPAD1);
    RTTI_ENUM_OPTION(KEY_NUMPAD2);
    RTTI_ENUM_OPTION(KEY_NUMPAD3);
    RTTI_ENUM_OPTION(KEY_NUMPAD4);
    RTTI_ENUM_OPTION(KEY_NUMPAD5);
    RTTI_ENUM_OPTION(KEY_NUMPAD6);
    RTTI_ENUM_OPTION(KEY_NUMPAD7);
    RTTI_ENUM_OPTION(KEY_NUMPAD8);
    RTTI_ENUM_OPTION(KEY_NUMPAD9);
    RTTI_ENUM_OPTION(KEY_NUMPAD_MULTIPLY);
    RTTI_ENUM_OPTION(KEY_NUMPAD_ADD);
    RTTI_ENUM_OPTION(KEY_NUMPAD_SEPARATOR);
    RTTI_ENUM_OPTION(KEY_NUMPAD_SUBTRACT);
    RTTI_ENUM_OPTION(KEY_NUMPAD_DECIMAL);
    RTTI_ENUM_OPTION(KEY_NUMPAD_DIVIDE);
    RTTI_ENUM_OPTION(KEY_F1);
    RTTI_ENUM_OPTION(KEY_F2);
    RTTI_ENUM_OPTION(KEY_F3);
    RTTI_ENUM_OPTION(KEY_F4);
    RTTI_ENUM_OPTION(KEY_F5);
    RTTI_ENUM_OPTION(KEY_F6);
    RTTI_ENUM_OPTION(KEY_F7);
    RTTI_ENUM_OPTION(KEY_F8);
    RTTI_ENUM_OPTION(KEY_F9);
    RTTI_ENUM_OPTION(KEY_F10);
    RTTI_ENUM_OPTION(KEY_F11);
    RTTI_ENUM_OPTION(KEY_F12);
    RTTI_ENUM_OPTION(KEY_F13);
    RTTI_ENUM_OPTION(KEY_F14);
    RTTI_ENUM_OPTION(KEY_F15);
    RTTI_ENUM_OPTION(KEY_F16);
    RTTI_ENUM_OPTION(KEY_F17);
    RTTI_ENUM_OPTION(KEY_F18);
    RTTI_ENUM_OPTION(KEY_F19);
    RTTI_ENUM_OPTION(KEY_F20);
    RTTI_ENUM_OPTION(KEY_F21);
    RTTI_ENUM_OPTION(KEY_F22);
    RTTI_ENUM_OPTION(KEY_F23);
    RTTI_ENUM_OPTION(KEY_F24);
    RTTI_ENUM_OPTION(KEY_NAVIGATION_VIEW);
    RTTI_ENUM_OPTION(KEY_NAVIGATION_MENU);
    RTTI_ENUM_OPTION(KEY_NAVIGATION_UP);
    RTTI_ENUM_OPTION(KEY_NAVIGATION_DOWN);
    RTTI_ENUM_OPTION(KEY_NAVIGATION_LEFT);
    RTTI_ENUM_OPTION(KEY_NAVIGATION_RIGHT);
    RTTI_ENUM_OPTION(KEY_NAVIGATION_ACCEPT);
    RTTI_ENUM_OPTION(KEY_NAVIGATION_CANCEL);
    RTTI_ENUM_OPTION(KEY_NUMLOCK);
    RTTI_ENUM_OPTION(KEY_SCROLL);
    RTTI_ENUM_OPTION(KEY_0);
    RTTI_ENUM_OPTION(KEY_1);
    RTTI_ENUM_OPTION(KEY_2);
    RTTI_ENUM_OPTION(KEY_3);
    RTTI_ENUM_OPTION(KEY_4);
    RTTI_ENUM_OPTION(KEY_5);
    RTTI_ENUM_OPTION(KEY_6);
    RTTI_ENUM_OPTION(KEY_7);
    RTTI_ENUM_OPTION(KEY_8);
    RTTI_ENUM_OPTION(KEY_9);
    RTTI_ENUM_OPTION(KEY_A);
    RTTI_ENUM_OPTION(KEY_B);
    RTTI_ENUM_OPTION(KEY_C);
    RTTI_ENUM_OPTION(KEY_D);
    RTTI_ENUM_OPTION(KEY_E);
    RTTI_ENUM_OPTION(KEY_F);
    RTTI_ENUM_OPTION(KEY_G);
    RTTI_ENUM_OPTION(KEY_H);
    RTTI_ENUM_OPTION(KEY_I);
    RTTI_ENUM_OPTION(KEY_J);
    RTTI_ENUM_OPTION(KEY_K);
    RTTI_ENUM_OPTION(KEY_L);
    RTTI_ENUM_OPTION(KEY_M);
    RTTI_ENUM_OPTION(KEY_N);
    RTTI_ENUM_OPTION(KEY_O);
    RTTI_ENUM_OPTION(KEY_P);
    RTTI_ENUM_OPTION(KEY_Q);
    RTTI_ENUM_OPTION(KEY_R);
    RTTI_ENUM_OPTION(KEY_S);
    RTTI_ENUM_OPTION(KEY_T);
    RTTI_ENUM_OPTION(KEY_U);
    RTTI_ENUM_OPTION(KEY_V);
    RTTI_ENUM_OPTION(KEY_W);
    RTTI_ENUM_OPTION(KEY_X);
    RTTI_ENUM_OPTION(KEY_Y);
    RTTI_ENUM_OPTION(KEY_Z);
    RTTI_ENUM_OPTION(KEY_MOUSE0);
    RTTI_ENUM_OPTION(KEY_MOUSE1);
    RTTI_ENUM_OPTION(KEY_MOUSE2);
    RTTI_ENUM_OPTION(KEY_MOUSE3);
    RTTI_ENUM_OPTION(KEY_MOUSE4);
    RTTI_ENUM_OPTION(KEY_MOUSE5);
    RTTI_ENUM_OPTION(KEY_MOUSE6);
    RTTI_ENUM_OPTION(KEY_MOUSE7);
    RTTI_ENUM_OPTION(KEY_MOUSE8);
    RTTI_ENUM_OPTION(KEY_MOUSE9);
    RTTI_ENUM_OPTION(KEY_MOUSE10);
    RTTI_ENUM_OPTION(KEY_MOUSE11);
    RTTI_ENUM_OPTION(KEY_MOUSE12);
    RTTI_ENUM_OPTION(KEY_MOUSE13);
    RTTI_ENUM_OPTION(KEY_MOUSE14);
    RTTI_ENUM_OPTION(KEY_MOUSE15);
    RTTI_ENUM_OPTION(KEY_LEFT_SHIFT);
    RTTI_ENUM_OPTION(KEY_RIGHT_SHIFT);
    RTTI_ENUM_OPTION(KEY_LEFT_CTRL);
    RTTI_ENUM_OPTION(KEY_RIGHT_CTRL);
    RTTI_ENUM_OPTION(KEY_LEFT_ALT);
    RTTI_ENUM_OPTION(KEY_RIGHT_ALT);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(InputAxis);
    RTTI_ENUM_OPTION(AXIS_MOUSEX);
    RTTI_ENUM_OPTION(AXIS_MOUSEY);
    RTTI_ENUM_OPTION(AXIS_MOUSEZ);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(MouseEventType);
	RTTI_ENUM_OPTION(Click);
	RTTI_ENUM_OPTION(DoubleClick);
	RTTI_ENUM_OPTION(Release);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(InputDeviceType);
	RTTI_ENUM_OPTION(Invalid);
	RTTI_ENUM_OPTION(Unknown);
	RTTI_ENUM_OPTION(Keyboard);
	RTTI_ENUM_OPTION(Mouse);
	RTTI_ENUM_OPTION(XboxPad);
	RTTI_ENUM_OPTION(PS4Pad);
	RTTI_ENUM_OPTION(GenericPad);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(CursorType)
    RTTI_ENUM_OPTION(Hidden);
    RTTI_ENUM_OPTION(Default);
    RTTI_ENUM_OPTION(Cross);
    RTTI_ENUM_OPTION(Hand);
    RTTI_ENUM_OPTION(Help);
    RTTI_ENUM_OPTION(TextBeam);
    RTTI_ENUM_OPTION(No);
    RTTI_ENUM_OPTION(SizeAll);
    RTTI_ENUM_OPTION(SizeNS);
    RTTI_ENUM_OPTION(SizeWE);
    RTTI_ENUM_OPTION(SizeNESW);
    RTTI_ENUM_OPTION(SizeNWSE);
    RTTI_ENUM_OPTION(UpArrow);
    RTTI_ENUM_OPTION(Wait);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(AreaType);
    RTTI_ENUM_OPTION(NotInWindow);
    RTTI_ENUM_OPTION(Client);
    RTTI_ENUM_OPTION(NonSizableBorder);
    RTTI_ENUM_OPTION(BorderBottom);
    RTTI_ENUM_OPTION(BorderBottomLeft);
    RTTI_ENUM_OPTION(BorderBottomRight);
    RTTI_ENUM_OPTION(BorderTop);
    RTTI_ENUM_OPTION(BorderTopLeft);
    RTTI_ENUM_OPTION(BorderTopRight);
    RTTI_ENUM_OPTION(BorderLeft);
    RTTI_ENUM_OPTION(BorderRight);
    RTTI_ENUM_OPTION(Caption);
    RTTI_ENUM_OPTION(Close);
    RTTI_ENUM_OPTION(SizeBox);
    RTTI_ENUM_OPTION(Help);
    RTTI_ENUM_OPTION(HorizontalScroll);
    RTTI_ENUM_OPTION(VerticalScroll);
    RTTI_ENUM_OPTION(Menu);
    RTTI_ENUM_OPTION(Minimize);
    RTTI_ENUM_OPTION(Maximize);
    RTTI_ENUM_OPTION(SysMenu);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(InputEvent);
	RTTI_PROPERTY(m_eventType);
	RTTI_PROPERTY(m_deviceType);
	RTTI_PROPERTY(m_deviceId);
RTTI_END_TYPE();

InputEvent::InputEvent(InputEventType eventType, InputDeviceType deviceType, InputDeviceID deviceId)
    : m_eventType(eventType)
    , m_deviceType(deviceType)
    , m_deviceId(deviceId)
    , m_timeStamp(NativeTimePoint::Now())
{}

const InputCharEvent* InputEvent::toCharEvent() const
{
    return (m_eventType == InputEventType::Char) ? static_cast<const InputCharEvent*>(this) : nullptr;
}

const InputKeyEvent* InputEvent::toKeyEvent() const
{
    return (m_eventType == InputEventType::Key) ? static_cast<const InputKeyEvent*>(this) : nullptr;
}

const InputAxisEvent* InputEvent::toAxisEvent() const
{
    return (m_eventType == InputEventType::Axis) ? static_cast<const InputAxisEvent*>(this) : nullptr;
}

const InputMouseClickEvent* InputEvent::toMouseClickEvent() const
{
    return (m_eventType == InputEventType::MouseClick) ? static_cast<const InputMouseClickEvent*>(this) : nullptr;
}

const InputMouseMovementEvent* InputEvent::toMouseMoveEvent() const
{
    return (m_eventType == InputEventType::MouseMove) ? static_cast<const InputMouseMovementEvent*>(this) : nullptr;
}

const InputMouseCaptureLostEvent* InputEvent::toMouseCaptureLostEvent() const
{
    return (m_eventType == InputEventType::MouseCaptureLost) ? static_cast<const InputMouseCaptureLostEvent*>(this) : nullptr;
}

const InputDragDropEvent* InputEvent::toDragAndDropEvent() const
{
    return (m_eventType == InputEventType::DragDrop) ? static_cast<const InputDragDropEvent*>(this) : nullptr;
}

void InputEvent::printBase(IFormatStream& f) const
{
	f << m_eventType;
	if (m_deviceId != 0)
	f << " (from: " << m_deviceType << ", id=" << m_deviceId << ")";
	if (m_timeStamp.valid())
		f.appendf(" @{} ago", TimeInterval(m_timeStamp.timeTillNow().toSeconds()));
}

void InputEvent::print(IFormatStream& f) const
{
	switch (m_eventType)
	{
		case InputEventType::Key: toKeyEvent()->print(f); break;
		case InputEventType::Char: toCharEvent()->print(f); break;
		case InputEventType::Axis: toAxisEvent()->print(f); break;
		case InputEventType::MouseClick: toMouseClickEvent()->print(f); break;
		case InputEventType::MouseMove: toMouseMoveEvent()->print(f); break;
		case InputEventType::MouseCaptureLost: toMouseCaptureLostEvent()->print(f); break;
		case InputEventType::DragDrop: toDragAndDropEvent()->print(f); break;
		default: printBase(f);
	}
}

//--

RTTI_BEGIN_TYPE_CLASS(BaseKeyFlags);
	RTTI_FUNCTION("IsLeftDown", isLeftDown);
	RTTI_FUNCTION("IsMidDown", isMidDown);
	RTTI_FUNCTION("IsRightDown", isRightDown);
	RTTI_FUNCTION("IsAltDown", isAltDown);
	RTTI_FUNCTION("IsShiftDown", isShiftDown);
	RTTI_FUNCTION("IsCtrlDown", isCtrlDown);
	RTTI_FUNCTION("IsLeftAltDown", isLeftAltDown);
	RTTI_FUNCTION("IsLeftShiftDown", isLeftShiftDown);
	RTTI_FUNCTION("IsLeftControlDown", isLeftCtrlDown);
	RTTI_FUNCTION("IsRightAltDown", isRightAltDown);
	RTTI_FUNCTION("IsRightShiftDown", isRightShiftDown);
	RTTI_FUNCTION("IsRightControlDown", isRightCtrlDown);
RTTI_END_TYPE();

BaseKeyFlags::BaseKeyFlags()
{}

BaseKeyFlags::BaseKeyFlags(const InputKeyMask& mask)
	: m_keyMask(mask)
{}

BaseKeyFlags::BaseKeyFlags(const BaseKeyFlags& other) = default;
BaseKeyFlags& BaseKeyFlags::operator=(const BaseKeyFlags& other) = default;

void BaseKeyFlags::print(IFormatStream& f) const
{
	if (m_keyMask.test(InputKeyMaskBit::LEFT_SHIFT))
		f.append(" +LSHIFT");
	if (m_keyMask.test(InputKeyMaskBit::RIGHT_SHIFT))
		f.append(" +RSHIFT");
	if (m_keyMask.test(InputKeyMaskBit::LEFT_CTRL))
		f.append(" +LCTRL");
	if (m_keyMask.test(InputKeyMaskBit::RIGHT_CTRL))
		f.append(" +RCTRL");
	if (m_keyMask.test(InputKeyMaskBit::LEFT_ALT))
		f.append(" +LALT");
	if (m_keyMask.test(InputKeyMaskBit::RIGHT_ALT))
		f.append(" +RALT");
	if (m_keyMask.test(InputKeyMaskBit::LEFT_MOUSE))
		f.append(" +LMB");
	if (m_keyMask.test(InputKeyMaskBit::RIGHT_MOUSE))
		f.append(" +RMB");
	if (m_keyMask.test(InputKeyMaskBit::MIDDLE_MOUSE))
		f.append(" +MMB");
}

//--

RTTI_BEGIN_TYPE_CLASS(InputMouseClickEvent);
	RTTI_PROPERTY_VIRTUAL("base", InputEvent, 0);
	RTTI_PROPERTY(m_key);
	RTTI_PROPERTY(m_clickType);
	RTTI_PROPERTY(m_keyMask);
	RTTI_PROPERTY(m_windowPos);
	RTTI_PROPERTY(m_absolutePos);
	RTTI_FUNCTION("LeftClicked", leftClicked);
	RTTI_FUNCTION("LeftDoubleClicked", leftDoubleClicked);
	RTTI_FUNCTION("LeftReleased", leftReleased);
	RTTI_FUNCTION("MidClicked", midClicked);
	RTTI_FUNCTION("MidReleased", midReleased);
	RTTI_FUNCTION("RightClicked", rightClicked);
	RTTI_FUNCTION("RightReleased", rightReleased);
RTTI_END_TYPE();

InputMouseClickEvent::InputMouseClickEvent(InputDeviceID deviceId, InputKey key, MouseEventType type, BaseKeyFlags BaseKeyFlags, const Point& windowPos, const Point& absolutePos)
    : InputEvent(InputEventType::MouseClick, InputDeviceType::Mouse, deviceId)
    , m_key(key)
    , m_clickType(type)
    , m_keyMask(BaseKeyFlags)
    , m_windowPos(windowPos)
    , m_absolutePos(absolutePos)
{}

//--

void InputMouseClickEvent::print(IFormatStream& f) const
{
	printBase(f);
			
	f.appendf(" (key: {}, type: {})", m_key, m_clickType);
	f.appendf(" (window: {},{})", m_windowPos.x, m_windowPos.y);
	f.appendf(" (absolute: {},{})", m_absolutePos.x, m_absolutePos.y);
				
	f << m_keyMask;
}

//--

RTTI_BEGIN_TYPE_CLASS(InputMouseMovementEvent);
	RTTI_PROPERTY_VIRTUAL("base", InputEvent, 0);
	RTTI_PROPERTY(m_keyMask);
	RTTI_PROPERTY(m_windowPos);
	RTTI_PROPERTY(m_absolutePos);
	RTTI_PROPERTY(m_captured);
	RTTI_PROPERTY(m_delta);
RTTI_END_TYPE();

InputMouseMovementEvent::InputMouseMovementEvent(InputDeviceID deviceId, BaseKeyFlags BaseKeyFlags, bool captured, const Point& windowPoint, const Point& absolutePoint, const Vector3& delta)
    : InputEvent(InputEventType::MouseMove, InputDeviceType::Mouse, deviceId)
    , m_keyMask(BaseKeyFlags)
    , m_captured(captured)
    , m_windowPos(windowPoint)
    , m_absolutePos(absolutePoint)
    , m_delta(delta)
{}

bool InputMouseMovementEvent::merge(const InputMouseMovementEvent& source)
{
    if (m_keyMask != source.m_keyMask)
        return false;

    m_absolutePos = source.m_absolutePos;
    m_windowPos = source.m_windowPos;
    m_delta += source.m_delta;
    return true;
}

void InputMouseMovementEvent::print(IFormatStream& f) const
{
	printBase(f);

	if (m_delta.x != 0)
		f.appendf(" dx:{}", m_delta.x);
	if (m_delta.y != 0)
		f.appendf(" dy:{}", m_delta.y);
	if (m_delta.z != 0)
		f.appendf(" dz:{}", m_delta.z);

	f.appendf(" (window: {},{})", m_windowPos.x, m_windowPos.y);
	f.appendf(" (absolute: {},{})", m_absolutePos.x, m_absolutePos.y);

	f << m_keyMask;

	if (m_captured)
		f.append(" +CAPTURED");
}

//---

RTTI_BEGIN_TYPE_CLASS(InputMouseCaptureLostEvent);
	RTTI_PROPERTY_VIRTUAL("base", InputEvent, 0);
RTTI_END_TYPE();

InputMouseCaptureLostEvent::InputMouseCaptureLostEvent(InputDeviceID deviceId)
    : InputEvent(InputEventType::MouseCaptureLost, InputDeviceType::Mouse, deviceId)
{}

void InputMouseCaptureLostEvent::print(IFormatStream& f) const
{
	printBase(f);
}

//---

RTTI_BEGIN_TYPE_CLASS(InputCharEvent);
	RTTI_PROPERTY_VIRTUAL("base", InputEvent, 0);
	RTTI_PROPERTY_FORCE_TYPE(m_scanCode, uint16_t);
	RTTI_PROPERTY(m_repeated);
	RTTI_PROPERTY(m_keyMask);
	RTTI_FUNCTION("GetPrintableText", text);
RTTI_END_TYPE();

InputCharEvent::InputCharEvent(InputDeviceID deviceId, KeyScanCode scanCode, bool repeated, BaseKeyFlags BaseKeyFlags)
    : InputEvent(InputEventType::Char, InputDeviceType::Keyboard, deviceId)
    , m_scanCode(scanCode)
    , m_repeated(repeated)
    , m_keyMask(BaseKeyFlags)
{}

void InputCharEvent::print(IFormatStream& f) const
{
	printBase(f);

	if (m_scanCode >= 0 && m_scanCode <= 127)
	{
		char txt[2] = { 0, (char)m_scanCode };
		f.appendf(" key: '{}'", txt);
	}

	f.appendf(" (scan code: {})", (uint32_t)m_scanCode);

	f << m_keyMask;

	if (m_repeated)
		f.append(" +REPEATED");
}

StringBuf InputCharEvent::text() const
{
	const wchar_t str[2] = { m_scanCode, 0 };
	return UTF16StringVector(str).ansi_str();
}

//---

RTTI_BEGIN_TYPE_CLASS(InputKeyEvent);
	RTTI_PROPERTY_VIRTUAL("base", InputEvent, 0);
	RTTI_PROPERTY(m_key);
	RTTI_PROPERTY(m_pressed);
	RTTI_PROPERTY(m_repeated);
	RTTI_PROPERTY(m_keyMask);
	RTTI_FUNCTION("Pressed", pressed);
	RTTI_FUNCTION("PressedOrRepeated", pressedOrRepeated);
	RTTI_FUNCTION("Released", released);
	RTTI_FUNCTION("IsDown", isDown);
RTTI_END_TYPE();

InputKeyEvent::InputKeyEvent(InputDeviceType deviceType, InputDeviceID deviceId, InputKey key, bool pressed, bool repeated, BaseKeyFlags BaseKeyFlags)
    : InputEvent(InputEventType::Key, deviceType, deviceId)
    , m_key(key)
    , m_pressed(pressed)
    , m_repeated(repeated)
    , m_keyMask(BaseKeyFlags)
{}

RefPtr<InputKeyEvent> InputKeyEvent::makeReleaseEvent() const
{
    return RefNew<InputKeyEvent>(deviceType(), deviceID(), keyCode(), false, false, keyMask());
}

void InputKeyEvent::print(IFormatStream& f) const
{
	printBase(f);

	f.appendf(" key: {}", m_key);

	f << m_keyMask;

	if (m_pressed)
		f.append(" +PRESSED");
	if (m_repeated)
		f.append(" +REPEATED");
}

//---

RTTI_BEGIN_TYPE_CLASS(InputAxisEvent);
	RTTI_PROPERTY_VIRTUAL("base", InputEvent, 0);
	RTTI_PROPERTY(m_axis);
	RTTI_PROPERTY(m_displacement);
RTTI_END_TYPE();


InputAxisEvent::InputAxisEvent(InputDeviceType deviceType, InputDeviceID deviceId, InputAxis axisCode, float value)
    : InputEvent(InputEventType::Axis, deviceType, deviceId)
    , m_axis(axisCode)
    , m_displacement(value)
{}

    bool InputAxisEvent::merge(const InputAxisEvent& source)
{
    if (m_axis != source.axisCode())
        return false;

    m_displacement += source.m_displacement;
    return true;
}

    RefPtr<InputAxisEvent> InputAxisEvent::makeResetEvent() const
{
    return RefNew<InputAxisEvent>(deviceType(), deviceID(), axisCode(), 0.0f);
}

void InputAxisEvent::print(IFormatStream& f) const
{
	printBase(f);

	f.appendf(" axis: {}", m_axis);
	f.appendf(" delta: {}", m_displacement);
}

//---

RTTI_BEGIN_TYPE_CLASS(InputDragDropEvent);
RTTI_END_TYPE();

InputDragDropEvent::InputDragDropEvent(DragDropAction action, const Buffer& ptr, const Point& point)
    : InputEvent(InputEventType::DragDrop, InputDeviceType::Unknown, 0)
    , m_action(action)
    , m_point(point)
    , m_data(ptr)
{}

void InputDragDropEvent::print(IFormatStream& f) const
{
	printBase(f);

	// TODO ?
}

//---

END_BOOMER_NAMESPACE()
