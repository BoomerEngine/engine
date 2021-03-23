/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_input_glue.inl"

BEGIN_BOOMER_NAMESPACE()

class IInputContext;
typedef RefPtr<IInputContext> ContextPtr;

class InputEvent;
typedef RefPtr<InputEvent> EventPtr;

class EventBuffer;

enum class InputEventType : uint8_t;
enum class InputKey : uint8_t;
enum class InputAxis : uint8_t;

enum class MouseEventType : uint8_t
{
    Click,
    DoubleClick,
    Release,
};

enum class InputDeviceType : uint8_t
{
    Invalid,
    Unknown,

    Keyboard,
    Mouse,
    XboxPad,
    PS4Pad,
    GenericPad,
};

enum class CursorType : uint8_t
{
    Hidden, // hides the mouse 
    Default, // default cursor 
    Cross, // Crosshair
    Hand, // Hand icon (grab)
    Help, // Help icon (arrow and question mark)
    TextBeam, // The I-beam
    No, // Slashed circle
    SizeAll, // Four - pointed arrow pointing north, south, east, and west
    SizeNS, // double - pointed arrow pointing north and south
    SizeWE, // double - pointed arrow pointing west and east
    SizeNESW, // double - pointed arrow pointing northeast and southwest
    SizeNWSE, // double - pointed arrow pointing northwest and southeast
    UpArrow, // Vertical arrow
    Wait, // Hourglass
};

enum class AreaType : uint8_t
{
    NotInWindow, // Point is not inside the window
    Client, // Client area
    NonSizableBorder, //  In the border of a window that does not have a sizing border.
    BorderBottom, // In the lower - horizontal border of a resizable window(the user can click the mouse to resize the window vertically).
    BorderBottomLeft, //  In the lower - left corner of a border of a resizable window(the user can click the mouse to resize the window diagonally).
    BorderBottomRight, // In the lower - right corner of a border of a resizable window(the user can click the mouse to resize the window diagonally).
    BorderTop, // In the upper - horizontal border of a resizable window(the user can click the mouse to resize the window vertically).
    BorderTopLeft, //  In the upper - left corner of a border of a resizable window(the user can click the mouse to resize the window diagonally).
    BorderTopRight, // In the upper - right corner of a border of a resizable window(the user can click the mouse to resize the window diagonally).
    BorderLeft, // In the left border of a resizable window(the user can click the mouse to resize the window horizontally).
    BorderRight, // In the right border of a resizable window(the user can click the mouse to resize the window horizontally).
    Caption, // In a title bar
    Close, // In a Close button
    SizeBox,  // In a size box(same as HTSIZE).
    Help, // In a Help button.
    HorizontalScroll, // In a horizontal scroll bar.
    VerticalScroll, // In a horizontal scroll bar.
    Menu, // In a menu.
    Minimize, // In a Minimize button.
    Maximize, // In a Maximize button.
    SysMenu, // In a window menu or in a Close button in a child window.
};

typedef uint8_t InputDeviceID;
typedef wchar_t KeyScanCode;
typedef uint8_t MouseButtonIndex;

class IInputDevicve;
class IGenericKeyboard;
class IGenericMouse;
class IGenericPad;

class InputKeyEvent;
class InputCharEvent;
class InputAxisEvent;
class InputMouseClickEvent;
class InputMouseMovementEvent;
class InputMouseCaptureLostEvent;
class InputDragDropEvent;

class ISystem;
typedef RefPtr<ISystem> SystemPtr;

// quick and dirty global input state - fast prototyping and debug

//--

// is key down
extern CORE_INPUT_API bool CheckInputKeyState(InputKey key);

// was key pressed (resets only when key is released)
extern CORE_INPUT_API bool CheckInputKeyPressed(InputKey key);

//--

END_BOOMER_NAMESPACE()
