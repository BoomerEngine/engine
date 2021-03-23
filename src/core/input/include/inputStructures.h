/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once

#include "core/math/include/point.h"
#include "core/system/include/timing.h"

BEGIN_BOOMER_NAMESPACE()

/// type of the input event
enum class InputEventType : uint8_t
{
    None,
    Key,
    Char,
    Axis,
    MouseClick,
    MouseMove,
    MouseCaptureLost,
    DragDrop,
};

class InputKeyEvent;
class InputCharEvent;
class InputAxisEvent;
class InputMouseClickEvent;
class InputMouseMovementEvent;
class InputMouseCaptureLostEvent;
class InputDragDropEvent;

/// shared namespace for keys and mouse button IDs
enum class InputKey : uint8_t
{
    KEY_INVALID = 0x0,

    /// general keys
    KEY_BACK = 0x08,
    KEY_TAB = 0x09,
    KEY_CLEAR = 0x0C,
    KEY_RETURN = 0x0D,
    KEY_PAUSE = 0x13,
    KEY_CAPITAL = 0x14,
    KEY_ESCAPE = 0x1B,
    KEY_CONVERT = 0x1C,
    KEY_NONCONVERT = 0x1D,
    KEY_ACCEPT = 0x1E,
    KEY_MODECHANGE = 0x1F,
    KEY_SPACE = 0x20,
    KEY_PRIOR = 0x21,
    KEY_NEXT = 0x22,
    KEY_END = 0x23,
    KEY_HOME = 0x24,
    KEY_LEFT = 0x25,
    KEY_UP = 0x26,
    KEY_RIGHT = 0x27,
    KEY_DOWN = 0x28,
    KEY_SELECT = 0x29,
    KEY_PRINT = 0x2A,
    KEY_SNAPSHOT = 0x2C,
    KEY_INSERT = 0x2D,
    KEY_DELETE = 0x2E,
    KEY_HELP = 0x2F,

    // numbers
    KEY_0 = 0x30,
    KEY_1 = 0x31,
    KEY_2 = 0x32,
    KEY_3 = 0x33,
    KEY_4 = 0x34,
    KEY_5 = 0x35,
    KEY_6 = 0x36,
    KEY_7 = 0x37,
    KEY_8 = 0x38,
    KEY_9 = 0x39,

    // letters
    KEY_A = 0x41,
    KEY_B = 0x42,
    KEY_C = 0x43,
    KEY_D = 0x44,
    KEY_E = 0x45,
    KEY_F = 0x46,
    KEY_G = 0x47,
    KEY_H = 0x48,
    KEY_I = 0x49,
    KEY_J = 0x4A,
    KEY_K = 0x4B,
    KEY_L = 0x4C,
    KEY_M = 0x4D,
    KEY_N = 0x4E,
    KEY_O = 0x4F,
    KEY_P = 0x50,
    KEY_Q = 0x51,
    KEY_R = 0x52,
    KEY_S = 0x53,
    KEY_T = 0x54,
    KEY_U = 0x55,
    KEY_V = 0x56,
    KEY_W = 0x57,
    KEY_X = 0x58,
    KEY_Y = 0x59,
    KEY_Z = 0x5A,
    KEY_LWIN = 0x5B,
    KEY_RWIN = 0x5C,
    KEY_APPS = 0x5D,
    KEY_SLEEP = 0x5F,

    KEY_NUMPAD0 = 0x60,
    KEY_NUMPAD1 = 0x61,
    KEY_NUMPAD2 = 0x62,
    KEY_NUMPAD3 = 0x63,
    KEY_NUMPAD4 = 0x64,
    KEY_NUMPAD5 = 0x65,
    KEY_NUMPAD6 = 0x66,
    KEY_NUMPAD7 = 0x67,
    KEY_NUMPAD8 = 0x68,
    KEY_NUMPAD9 = 0x69,
    KEY_NUMPAD_MULTIPLY = 0x6A,
    KEY_NUMPAD_ADD = 0x6B,
    KEY_NUMPAD_SEPARATOR = 0x6C,
    KEY_NUMPAD_SUBTRACT = 0x6D,
    KEY_NUMPAD_DECIMAL = 0x6E,
    KEY_NUMPAD_DIVIDE = 0x6F,

    KEY_F1 = 0x70,
    KEY_F2 = 0x71,
    KEY_F3 = 0x72,
    KEY_F4 = 0x73,
    KEY_F5 = 0x74,
    KEY_F6 = 0x75,
    KEY_F7 = 0x76,
    KEY_F8 = 0x77,
    KEY_F9 = 0x78,
    KEY_F10 = 0x79,
    KEY_F11 = 0x7A,
    KEY_F12 = 0x7B,
    KEY_F13 = 0x7C,
    KEY_F14 = 0x7D,
    KEY_F15 = 0x7E,
    KEY_F16 = 0x7F,
    KEY_F17 = 0x80,
    KEY_F18 = 0x81,
    KEY_F19 = 0x82,
    KEY_F20 = 0x83,
    KEY_F21 = 0x84,
    KEY_F22 = 0x85,
    KEY_F23 = 0x86,
    KEY_F24 = 0x87,

    KEY_NAVIGATION_VIEW = 0x88,
    KEY_NAVIGATION_MENU = 0x89,
    KEY_NAVIGATION_UP = 0x8A,
    KEY_NAVIGATION_DOWN = 0x8B,
    KEY_NAVIGATION_LEFT = 0x8C,
    KEY_NAVIGATION_RIGHT = 0x8D,
    KEY_NAVIGATION_ACCEPT = 0x8E,
    KEY_NAVIGATION_CANCEL = 0x8F,
    KEY_NUMLOCK = 0x90,
    KEY_SCROLL = 0x91,

    /// function keys
    KEY_LEFT_SHIFT = 0xA0,
    KEY_RIGHT_SHIFT = 0xA1,
    KEY_LEFT_CTRL = 0xA2,
    KEY_RIGHT_CTRL = 0xA3,
    KEY_LEFT_ALT = 0xA4,
    KEY_RIGHT_ALT = 0xA5,

    /// additional mouse keys
    KEY_MOUSE0 = 0xB0,
    KEY_MOUSE1 = 0xB1,
    KEY_MOUSE2 = 0xB2,
    KEY_MOUSE3 = 0xB3,
    KEY_MOUSE4 = 0xB4,
    KEY_MOUSE5 = 0xB5,
    KEY_MOUSE6 = 0xB6,
    KEY_MOUSE7 = 0xB7,
    KEY_MOUSE8 = 0xB8,
    KEY_MOUSE9 = 0xB9,
    KEY_MOUSE10 = 0xBA,
    KEY_MOUSE11 = 0xBB,
    KEY_MOUSE12 = 0xBC,
    KEY_MOUSE13 = 0xBD,
    KEY_MOUSE14 = 0xBE,
    KEY_MOUSE15 = 0xBF,

    // additional keys
    KEY_MINUS = 0xC0,
    KEY_EQUAL = 0xC1,
    KEY_LBRACKET = 0xC2,
    KEY_RBRACKET = 0xC3,
    KEY_SEMICOLON = 0xC4,
    KEY_APOSTROPHE = 0xC5,
    KEY_COMMA = 0xC6,
    KEY_PERIOD = 0xC7,
    KEY_SLASH = 0xC8,
    KEY_BACKSLASH = 0xC9,
    KEY_GRAVE = 0xCA,

    KEY_MAX = 0xFF,
};

enum class InputAxis : uint8_t
{
    AXIS_MOUSEX = 0x00,
    AXIS_MOUSEY = 0x01,
    AXIS_MOUSEZ = 0x02,
    AXIS_MAX,
};

enum class InputKeyMaskBit : uint16_t
{
    LEFT_SHIFT = FLAG(0),
    RIGHT_SHIFT = FLAG(1),
    LEFT_CTRL = FLAG(2),
    RIGHT_CTRL = FLAG(3),
    LEFT_ALT = FLAG(4),
    RIGHT_ALT = FLAG(5),

    LEFT_MOUSE = FLAG(6),
    RIGHT_MOUSE = FLAG(7),
    MIDDLE_MOUSE = FLAG(8),

    ANY_SHIFT = LEFT_SHIFT | RIGHT_SHIFT,
    ANY_CTRL = LEFT_CTRL | RIGHT_CTRL,
    ANY_ALT = LEFT_ALT | RIGHT_ALT,
};

typedef DirectFlags<InputKeyMaskBit> InputKeyMask;

//--

/// base class for event type
class CORE_INPUT_API InputEvent : public IReferencable
{
	RTTI_DECLARE_VIRTUAL_ROOT_CLASS(InputEvent);

public:
    /// get the type of the event, allows event casting
    INLINE InputEventType eventType() const { return m_eventType; }

    /// get the type of the device that produced the event
    INLINE InputDeviceType deviceType() const { return m_deviceType; }

    /// get ID of the device that generated the event (support for multiple pads/keyboards/mouses)
    INLINE InputDeviceID deviceID() const { return m_deviceId; }

    /// get timestamp of where was this event generated
    INLINE NativeTimePoint timestamp() const { return m_timeStamp; }

    //--

	INLINE InputEvent() = default;
	InputEvent(InputEventType eventType, InputDeviceType deviceType, InputDeviceID deviceId);

    //--

    // convert to key event
    const InputKeyEvent* toKeyEvent() const;

    // convert to axis event
    const InputAxisEvent* toAxisEvent() const;

    // convert to character event
    const InputCharEvent* toCharEvent() const;

    // convert to mouse click event
    const InputMouseClickEvent* toMouseClickEvent() const;

    // convert to mouse move event
    const InputMouseMovementEvent* toMouseMoveEvent() const;

    // convert to mouse capture lost event
    const InputMouseCaptureLostEvent* toMouseCaptureLostEvent() const;

    // convert to drag&drop event
    const InputDragDropEvent* toDragAndDropEvent() const;

	//--

	// describe the event
	void print(IFormatStream& f) const;

protected:
    InputEventType m_eventType;
    InputDeviceType m_deviceType;
    InputDeviceID m_deviceId;            
    NativeTimePoint m_timeStamp;

	void printBase(IFormatStream& f) const;
};

//--

/// key flags
class CORE_INPUT_API BaseKeyFlags
{
	RTTI_DECLARE_NONVIRTUAL_CLASS(BaseKeyFlags);

public:
	BaseKeyFlags();
	BaseKeyFlags(const InputKeyMask& mask);
	BaseKeyFlags(const BaseKeyFlags& other);
	BaseKeyFlags& operator=(const BaseKeyFlags& other);
	INLINE bool operator==(const BaseKeyFlags& other) const { return m_keyMask == other.m_keyMask; }
	INLINE bool operator!=(const BaseKeyFlags& other) const { return m_keyMask != other.m_keyMask; }

	/// get the raw mask
	INLINE InputKeyMask mask() const { return m_keyMask; }

	/// is the left button pressed right now ?
	INLINE bool isLeftDown() const { return m_keyMask.test(InputKeyMaskBit::LEFT_MOUSE); }

	/// is the middle button pressed NOW ?
	INLINE bool isMidDown() const { return m_keyMask.test(InputKeyMaskBit::MIDDLE_MOUSE); }

	/// is the right button pressed NOW ?
	INLINE bool isRightDown() const { return m_keyMask.test(InputKeyMaskBit::RIGHT_MOUSE); }

	/// is the alt key pressed during this mouse event ?
	INLINE bool isAltDown() const { return m_keyMask.test(InputKeyMaskBit::ANY_ALT); }

	/// is the shift key pressed during this mouse event ?
	INLINE bool isShiftDown() const { return m_keyMask.test(InputKeyMaskBit::ANY_SHIFT); }

	/// is the control key pressed during this mouse event ?
	INLINE bool isCtrlDown() const { return m_keyMask.test(InputKeyMaskBit::ANY_CTRL); }

	/// is the left alt key pressed during this mouse event ?
	INLINE bool isLeftAltDown() const { return m_keyMask.test(InputKeyMaskBit::LEFT_ALT); }

	/// is the left shift key pressed during this mouse event ?
	INLINE bool isLeftShiftDown() const { return m_keyMask.test(InputKeyMaskBit::LEFT_SHIFT); }

	/// is the left control key pressed during this mouse event ?
	INLINE bool isLeftCtrlDown() const { return m_keyMask.test(InputKeyMaskBit::LEFT_CTRL); }

	/// is the right alt key pressed during this mouse event ?
	INLINE bool isRightAltDown() const { return m_keyMask.test(InputKeyMaskBit::RIGHT_ALT); }

	/// is the right shift key pressed during this mouse event ?
	INLINE bool isRightShiftDown() const { return m_keyMask.test(InputKeyMaskBit::RIGHT_SHIFT); }

	/// is the right control key pressed during this mouse event ?
	INLINE bool isRightCtrlDown() const { return m_keyMask.test(InputKeyMaskBit::RIGHT_CTRL); }

	//--

	// describe the event
	void print(IFormatStream& f) const;

private:
	InputKeyMask m_keyMask;
};

//--

/// mouse event
class CORE_INPUT_API InputMouseClickEvent : public InputEvent
{
	RTTI_DECLARE_VIRTUAL_CLASS(InputMouseClickEvent, InputEvent);

public:
    /// is the left button just clicked ?
    INLINE bool leftClicked() const { return (m_key == InputKey::KEY_MOUSE0) && (m_clickType == MouseEventType::Click); }

    /// is the left button just double clicked ?
    INLINE bool leftDoubleClicked() const { return (m_key == InputKey::KEY_MOUSE0) && (m_clickType == MouseEventType::DoubleClick); }

    /// is the left button just released ?
    INLINE bool leftReleased() const { return (m_key == InputKey::KEY_MOUSE0) && (m_clickType == MouseEventType::Release);}

    /// is the middle button just clicked ?
    INLINE bool midClicked() const { return (m_key == InputKey::KEY_MOUSE2) && (m_clickType == MouseEventType::Click); }

    /// is the middle button just released ?
    INLINE bool midReleased() const { return (m_key == InputKey::KEY_MOUSE2) && (m_clickType == MouseEventType::Release); }

    /// is the right button just clicked ?
    INLINE bool rightClicked() const { return (m_key == InputKey::KEY_MOUSE1) && (m_clickType == MouseEventType::Click); }

    /// is the right button just released ?
    INLINE bool rightReleased() const { return (m_key == InputKey::KEY_MOUSE1) && (m_clickType == MouseEventType::Release); }

    /// get position in the window space where the event occurred
    INLINE const Point& windowPosition() const { return m_windowPos; }

    /// get position in the absolute desktop space where the event occurred
    INLINE const Point& absolutePosition() const { return m_absolutePos; }

    /// get the code of the mouse button pressed
    INLINE InputKey keyCode() const { return m_key; }

    /// get the mouse event type
    INLINE MouseEventType type() const { return m_clickType; }

	/// get key mask
	INLINE const BaseKeyFlags& keyMask() const { return m_keyMask;  }

    //---

	INLINE InputMouseClickEvent() = default;
	InputMouseClickEvent(InputDeviceID deviceId, InputKey key, MouseEventType type, BaseKeyFlags keyMask, const Point& windowPos, const Point& absolutePos);

	//--
	
	// describe the event
	void print(IFormatStream& f) const;

private:
    InputKey         m_key;      // button pressed/released
    MouseEventType  m_clickType; // type of event
	BaseKeyFlags    m_keyMask;  // shift/ctrl/alt key mask (so we don't have to track it)
    Point     m_windowPos; // position in window's client space where the click occurred
    Point     m_absolutePos; // absolute position where the even occurred
};

//--

/// mouse movement event
class CORE_INPUT_API InputMouseMovementEvent : public InputEvent
{
	RTTI_DECLARE_VIRTUAL_CLASS(InputMouseMovementEvent, InputEvent);

public:
    /// is the mouse input captured
    INLINE bool isCaptured() const { return m_captured; }

    /// get position in window space
    INLINE const Point& windowPosition() const { return m_windowPos; }

    /// get position in the absolute desktop space where the event occurred
    INLINE const Point& absolutePosition() const { return m_absolutePos; }

    /// get movement delta since last movement
    INLINE const Vector3& delta() const { return m_delta; }

	/// get key mask
	INLINE const BaseKeyFlags& keyMask() const { return m_keyMask; }

    ///---

	INLINE InputMouseMovementEvent() = default;
	InputMouseMovementEvent(InputDeviceID deviceId, BaseKeyFlags keyMask, bool captured, const Point& windowPoint, const Point& absolutePoint, const Vector3& delta);

    // merge this event with other instance
    bool merge(const InputMouseMovementEvent& source);

	// describe the event
	void print(IFormatStream& f) const;

private:
    Point     m_windowPos; // position in the window, NOTE: may be outside the window bounds if the mouse is captured
    Point     m_absolutePos; // absolute position where the even occurred
	BaseKeyFlags    m_keyMask; // mouse buttons + shift/ctrl/alt key mask (so we don't have to track it)
    bool            m_captured; // true if the mouse input is captured to the window receiving this event
    Vector3   m_delta; // delta position since last movement, NOTE: may not be integer (due to mouse sensitivity and other settings), the Z is the wheel
};

//--

/// mouse capture was lost
class CORE_INPUT_API InputMouseCaptureLostEvent : public InputEvent
{
	RTTI_DECLARE_VIRTUAL_CLASS(InputMouseCaptureLostEvent, InputEvent);

public:
	INLINE InputMouseCaptureLostEvent() = default;
	InputMouseCaptureLostEvent(InputDeviceID deviceId);

	// describe the event
	void print(IFormatStream& f) const;
};

//--

/// character event (for typing)
class CORE_INPUT_API InputCharEvent : public InputEvent
{
	RTTI_DECLARE_VIRTUAL_CLASS(InputCharEvent, InputEvent);

public:
    /// get the key scan code of the character
    INLINE KeyScanCode scanCode() const { return m_scanCode; }

    /// is this a repeated character ?
    INLINE bool isRepeated() const { return m_repeated; }

	/// get key mask
	INLINE const BaseKeyFlags& keyMask() const { return m_keyMask; }

    //---

	INLINE InputCharEvent() = default;
	InputCharEvent(InputDeviceID deviceId, KeyScanCode scanCode, bool repeated, BaseKeyFlags keyMask);

	// describe the event
	void print(IFormatStream& f) const;

	// get printable text
	StringBuf text() const;

private:
    wchar_t m_scanCode; // key code of the key being pressed
    bool    m_repeated;  // is this a auto-repeat press event ?
	BaseKeyFlags m_keyMask; // shift/ctrl/alt key mask (so we don't have to track it)
};

//--

/// key event (key was pressed/released)
class CORE_INPUT_API InputKeyEvent : public InputEvent
{
	RTTI_DECLARE_VIRTUAL_CLASS(InputKeyEvent, InputEvent);

public:
    /// get the key that was pressed ?
    INLINE InputKey keyCode() const { return m_key; }

    /// was the key just pressed ? NOTE: returns false if it's a repeat
    INLINE bool pressed() const { return m_pressed && !m_repeated; }

    /// was the key pressed (or repeated)
    INLINE bool pressedOrRepeated() const { return m_pressed || m_repeated; }

    /// was the key just released ? NOTE: returns false if it's a repeat
    INLINE bool released() const { return !m_pressed && !m_repeated; }

    /// is the key down ?
    INLINE bool isDown() const { return m_pressed; }

	/// get key mask
	INLINE const BaseKeyFlags& keyMask() const { return m_keyMask; }

    //---

	INLINE InputKeyEvent() = default;
	InputKeyEvent(InputDeviceType deviceType, InputDeviceID deviceId, InputKey key, bool pressed, bool repeated, BaseKeyFlags keyMask);

	//---

	// make a matching "key release" event that originates from the same source etc
	RefPtr<InputKeyEvent> makeReleaseEvent() const;

	//---

	// describe the event
	void print(IFormatStream& f) const;

private:
    InputKey m_key; // key in question
    bool    m_pressed; // is this a press event ?
    bool    m_repeated;  // is this a auto-repeat press event ?
	BaseKeyFlags m_keyMask; // shift/ctrl/alt key mask (so we don't have to track it)
};

//---

/// axis event (mouse deltas, pad/joystick displacements)
class CORE_INPUT_API InputAxisEvent : public InputEvent
{
	RTTI_DECLARE_VIRTUAL_CLASS(InputAxisEvent, InputEvent);

public:
    /// get the axis that is changing
    INLINE InputAxis axisCode() const { return m_axis; }

    /// get axis displacement
    INLINE float displacement() const { return m_displacement; }

    //---

	INLINE InputAxisEvent() = default;
    InputAxisEvent(InputDeviceType deviceType, InputDeviceID deviceId, InputAxis axisCode, float value);

    // merge two events
    bool merge(const InputAxisEvent& source);

	// make a reset event for this axis
	RefPtr<InputAxisEvent> makeResetEvent() const;

	//---

	// describe the event
	void print(IFormatStream& f) const;

private:
    InputAxis m_axis;
    float m_displacement;
};

//---

enum class DragDropAction : uint8_t
{
    Enter,
    Leave,
    Over,
    Drop,
};

class CORE_INPUT_API InputDragDropEvent : public InputEvent
{
    RTTI_DECLARE_VIRTUAL_CLASS(InputDragDropEvent, InputEvent);

public:
    // get drag&drop action type
    INLINE DragDropAction action() const { return m_action;  }

    // get data buffer
    INLINE const Buffer& data() const { return m_data; }

    // get target point in the window
    INLINE const Point& position() const { return m_point; }

    //--

	INLINE InputDragDropEvent() = default;
	InputDragDropEvent(DragDropAction action, const Buffer& ptr, const Point& point);

	//---

	// describe the event
	void print(IFormatStream& f) const;

private:
    DragDropAction m_action;
    Buffer m_data;
    Point m_point;
};

END_BOOMER_NAMESPACE()
