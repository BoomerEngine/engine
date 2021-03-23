/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#include "build.h"
#include "inputContextWinApi.h"

BEGIN_BOOMER_NAMESPACE()

///--

static void BuildKeyMapping(InputKey* outKeyMapping)
{
    outKeyMapping[VK_BACK] = InputKey::KEY_BACK;
    outKeyMapping[VK_TAB] = InputKey::KEY_TAB;
    outKeyMapping[VK_CLEAR] = InputKey::KEY_CLEAR;
    outKeyMapping[VK_RETURN] = InputKey::KEY_RETURN;
    outKeyMapping[VK_PAUSE] = InputKey::KEY_PAUSE;
    outKeyMapping[VK_CAPITAL] = InputKey::KEY_CAPITAL;
    outKeyMapping[VK_ESCAPE] = InputKey::KEY_ESCAPE;
    outKeyMapping[VK_CONVERT] = InputKey::KEY_CONVERT;
    outKeyMapping[VK_NONCONVERT] = InputKey::KEY_NONCONVERT;
    outKeyMapping[VK_ACCEPT] = InputKey::KEY_ACCEPT;
    outKeyMapping[VK_MODECHANGE] = InputKey::KEY_MODECHANGE;
    outKeyMapping[VK_SPACE] = InputKey::KEY_SPACE;
    outKeyMapping[VK_PRIOR] = InputKey::KEY_PRIOR;
    outKeyMapping[VK_NEXT] = InputKey::KEY_NEXT;
    outKeyMapping[VK_END] = InputKey::KEY_END;
    outKeyMapping[VK_HOME] = InputKey::KEY_HOME;
    outKeyMapping[VK_LEFT] = InputKey::KEY_LEFT;
    outKeyMapping[VK_UP] = InputKey::KEY_UP;
    outKeyMapping[VK_RIGHT] = InputKey::KEY_RIGHT;
    outKeyMapping[VK_DOWN] = InputKey::KEY_DOWN;
    outKeyMapping[VK_SELECT] = InputKey::KEY_SELECT;
    outKeyMapping[VK_PRINT] = InputKey::KEY_PRINT;
    outKeyMapping[VK_SNAPSHOT] = InputKey::KEY_SNAPSHOT;
    outKeyMapping[VK_INSERT] = InputKey::KEY_INSERT;
    outKeyMapping[VK_DELETE] = InputKey::KEY_DELETE;
    outKeyMapping[VK_HELP] = InputKey::KEY_HELP;
    outKeyMapping[VK_LWIN] = InputKey::KEY_LWIN;
    outKeyMapping[VK_RWIN] = InputKey::KEY_RWIN;
    outKeyMapping[VK_APPS] = InputKey::KEY_APPS;
    outKeyMapping[VK_SLEEP] = InputKey::KEY_SLEEP;
    outKeyMapping[VK_OEM_PLUS] = InputKey::KEY_EQUAL;
    outKeyMapping[VK_OEM_MINUS] = InputKey::KEY_MINUS;
    outKeyMapping[VK_OEM_COMMA] = InputKey::KEY_COMMA;
    outKeyMapping[VK_OEM_PERIOD] = InputKey::KEY_PERIOD;
    outKeyMapping[VK_OEM_1] = InputKey::KEY_SEMICOLON;
    outKeyMapping[VK_OEM_2] = InputKey::KEY_BACKSLASH;
    outKeyMapping[VK_OEM_3] = InputKey::KEY_GRAVE;
    outKeyMapping[VK_OEM_4] = InputKey::KEY_LBRACKET;
    outKeyMapping[VK_OEM_5] = InputKey::KEY_SLASH;
    outKeyMapping[VK_OEM_6] = InputKey::KEY_RBRACKET;
    outKeyMapping[VK_OEM_7] = InputKey::KEY_APOSTROPHE;
    outKeyMapping[VK_NUMPAD0] = InputKey::KEY_NUMPAD0;
    outKeyMapping[VK_NUMPAD1] = InputKey::KEY_NUMPAD1;
    outKeyMapping[VK_NUMPAD2] = InputKey::KEY_NUMPAD2;
    outKeyMapping[VK_NUMPAD3] = InputKey::KEY_NUMPAD3;
    outKeyMapping[VK_NUMPAD4] = InputKey::KEY_NUMPAD4;
    outKeyMapping[VK_NUMPAD5] = InputKey::KEY_NUMPAD5;
    outKeyMapping[VK_NUMPAD6] = InputKey::KEY_NUMPAD6;
    outKeyMapping[VK_NUMPAD7] = InputKey::KEY_NUMPAD7;
    outKeyMapping[VK_NUMPAD8] = InputKey::KEY_NUMPAD8;
    outKeyMapping[VK_NUMPAD9] = InputKey::KEY_NUMPAD9;
    outKeyMapping[VK_MULTIPLY] = InputKey::KEY_NUMPAD_MULTIPLY;
    outKeyMapping[VK_ADD] = InputKey::KEY_NUMPAD_ADD;
    outKeyMapping[VK_SEPARATOR] = InputKey::KEY_NUMPAD_SEPARATOR;
    outKeyMapping[VK_SUBTRACT] = InputKey::KEY_NUMPAD_SUBTRACT;
    outKeyMapping[VK_DECIMAL] = InputKey::KEY_NUMPAD_DECIMAL;
    outKeyMapping[VK_DIVIDE] = InputKey::KEY_NUMPAD_DIVIDE;
    outKeyMapping[VK_LCONTROL] = InputKey::KEY_LEFT_CTRL;
    outKeyMapping[VK_RCONTROL] = InputKey::KEY_RIGHT_CTRL;
    outKeyMapping[VK_LSHIFT] = InputKey::KEY_LEFT_SHIFT;
    outKeyMapping[VK_RSHIFT] = InputKey::KEY_RIGHT_SHIFT;
    outKeyMapping[VK_LMENU] = InputKey::KEY_LEFT_ALT;
    outKeyMapping[VK_RMENU] = InputKey::KEY_RIGHT_ALT;
    outKeyMapping[VK_F1] = InputKey::KEY_F1;
    outKeyMapping[VK_F2] = InputKey::KEY_F2;
    outKeyMapping[VK_F3] = InputKey::KEY_F3;
    outKeyMapping[VK_F4] = InputKey::KEY_F4;
    outKeyMapping[VK_F5] = InputKey::KEY_F5;
    outKeyMapping[VK_F6] = InputKey::KEY_F6;
    outKeyMapping[VK_F7] = InputKey::KEY_F7;
    outKeyMapping[VK_F8] = InputKey::KEY_F8;
    outKeyMapping[VK_F9] = InputKey::KEY_F9;
    outKeyMapping[VK_F10] = InputKey::KEY_F10;
    outKeyMapping[VK_F11] = InputKey::KEY_F11;
    outKeyMapping[VK_F12] = InputKey::KEY_F12;
    outKeyMapping[VK_F13] = InputKey::KEY_F13;
    outKeyMapping[VK_F14] = InputKey::KEY_F14;
    outKeyMapping[VK_F15] = InputKey::KEY_F15;
    outKeyMapping[VK_F16] = InputKey::KEY_F16;
    outKeyMapping[VK_F17] = InputKey::KEY_F17;
    outKeyMapping[VK_F18] = InputKey::KEY_F18;
    outKeyMapping[VK_F19] = InputKey::KEY_F19;
    outKeyMapping[VK_F20] = InputKey::KEY_F20;
    outKeyMapping[VK_F21] = InputKey::KEY_F21;
    outKeyMapping[VK_F22] = InputKey::KEY_F22;
    outKeyMapping[VK_F23] = InputKey::KEY_F23;
    outKeyMapping[VK_F24] = InputKey::KEY_F24;
#if(_WIN32_WINNT >= 0x0604)
    outKeyMapping[VK_NAVIGATION_VIEW] = InputKey::KEY_NAVIGATION_VIEW;
    outKeyMapping[VK_NAVIGATION_MENU] = InputKey::KEY_NAVIGATION_MENU;
    outKeyMapping[VK_NAVIGATION_UP] = InputKey::KEY_NAVIGATION_UP;
    outKeyMapping[VK_NAVIGATION_DOWN] = InputKey::KEY_NAVIGATION_DOWN;
    outKeyMapping[VK_NAVIGATION_LEFT] = InputKey::KEY_NAVIGATION_LEFT;
    outKeyMapping[VK_NAVIGATION_RIGHT] = InputKey::KEY_NAVIGATION_RIGHT;
    outKeyMapping[VK_NAVIGATION_ACCEPT] = InputKey::KEY_NAVIGATION_ACCEPT;
    outKeyMapping[VK_NAVIGATION_CANCEL] = InputKey::KEY_NAVIGATION_CANCEL;
#endif
    outKeyMapping[VK_NUMLOCK] = InputKey::KEY_NUMLOCK;
    outKeyMapping[VK_SCROLL] = InputKey::KEY_SCROLL;
    outKeyMapping['0'] = InputKey::KEY_0;
    outKeyMapping['1'] = InputKey::KEY_1;
    outKeyMapping['2'] = InputKey::KEY_2;
    outKeyMapping['3'] = InputKey::KEY_3;
    outKeyMapping['4'] = InputKey::KEY_4;
    outKeyMapping['5'] = InputKey::KEY_5;
    outKeyMapping['6'] = InputKey::KEY_6;
    outKeyMapping['7'] = InputKey::KEY_7;
    outKeyMapping['8'] = InputKey::KEY_8;
    outKeyMapping['9'] = InputKey::KEY_9;
    outKeyMapping['A'] = InputKey::KEY_A;
    outKeyMapping['B'] = InputKey::KEY_B;
    outKeyMapping['C'] = InputKey::KEY_C;
    outKeyMapping['D'] = InputKey::KEY_D;
    outKeyMapping['E'] = InputKey::KEY_E;
    outKeyMapping['F'] = InputKey::KEY_F;
    outKeyMapping['G'] = InputKey::KEY_G;
    outKeyMapping['H'] = InputKey::KEY_H;
    outKeyMapping['I'] = InputKey::KEY_I;
    outKeyMapping['J'] = InputKey::KEY_J;
    outKeyMapping['K'] = InputKey::KEY_K;
    outKeyMapping['L'] = InputKey::KEY_L;
    outKeyMapping['M'] = InputKey::KEY_M;
    outKeyMapping['N'] = InputKey::KEY_N;
    outKeyMapping['O'] = InputKey::KEY_O;
    outKeyMapping['P'] = InputKey::KEY_P;
    outKeyMapping['Q'] = InputKey::KEY_Q;
    outKeyMapping['R'] = InputKey::KEY_R;
    outKeyMapping['S'] = InputKey::KEY_S;
    outKeyMapping['T'] = InputKey::KEY_T;
    outKeyMapping['U'] = InputKey::KEY_U;
    outKeyMapping['V'] = InputKey::KEY_V;
    outKeyMapping['W'] = InputKey::KEY_W;
    outKeyMapping['X'] = InputKey::KEY_X;
    outKeyMapping['Y'] = InputKey::KEY_Y;
    outKeyMapping['Z'] = InputKey::KEY_Z;

    outKeyMapping[VK_OEM_3] = InputKey::KEY_GRAVE;
}

///--

RawKeyboard::RawKeyboard(IInputContext* context)
    : GenericKeyboard(context)
{
    memzero(&m_windowsKeyMapping, sizeof(m_windowsKeyMapping));
    BuildKeyMapping(m_windowsKeyMapping);

    uint32_t keyboardDelay = 0, keyboardSpeed = 0;
    SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &keyboardDelay, 0);
    SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &keyboardSpeed, 0);

    auto delay = 0.25f + (0.25f * keyboardDelay);
    auto rate = 2.5f + ((keyboardSpeed / 30.0f) * 27.5f);
    repeatAndDelay(delay, rate);
}

RawKeyboard::~RawKeyboard()
{
}

void RawKeyboard::interpretChar(HWND hWnd, WPARAM wParam)
{
    auto scanCode = (KeyScanCode)wParam;

    if (scanCode >= 128 && scanCode <= 255)
    {
        char str[] = { (char)scanCode , 0 };
        wchar_t uniStr[5] = { 0,0,0,0,0 };
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, 2, uniStr, ARRAY_COUNT(uniStr));
        scanCode = uniStr[0];
    }

    charDown(scanCode);
}

InputKey RawKeyboard::mapKeyCode(USHORT virtualKey, bool flag0) const
{
    // handle special keys
    switch (virtualKey)
    {
    case VK_CONTROL:
        return flag0 ? InputKey::KEY_RIGHT_CTRL : InputKey::KEY_LEFT_CTRL;

    case VK_MENU:
        return flag0 ? InputKey::KEY_RIGHT_ALT : InputKey::KEY_LEFT_ALT;

    case VK_SHIFT:
        return flag0 ? InputKey::KEY_RIGHT_SHIFT : InputKey::KEY_LEFT_SHIFT;

    case VK_INSERT:
        return flag0 ? InputKey::KEY_INSERT : InputKey::KEY_NUMPAD0;

    case VK_DELETE:
        return flag0 ? InputKey::KEY_DELETE : InputKey::KEY_NUMPAD_DECIMAL;

    case VK_HOME:
        return flag0 ? InputKey::KEY_HOME : InputKey::KEY_NUMPAD1;

    case VK_END:
        return flag0 ? InputKey::KEY_END : InputKey::KEY_NUMPAD7;

    case VK_PRIOR:
        return flag0 ? InputKey::KEY_PRIOR : InputKey::KEY_NUMPAD9;

    case VK_NEXT:
        return flag0 ? InputKey::KEY_NEXT : InputKey::KEY_NUMPAD3;

    case VK_UP:
        return flag0 ? InputKey::KEY_UP : InputKey::KEY_NUMPAD2;

    case VK_LEFT:
        return flag0 ? InputKey::KEY_LEFT : InputKey::KEY_NUMPAD4;

    case VK_RIGHT:
        return flag0 ? InputKey::KEY_RIGHT : InputKey::KEY_NUMPAD6;

    case VK_DOWN:
        return flag0 ? InputKey::KEY_DOWN : InputKey::KEY_NUMPAD8;

    case VK_CLEAR:
        return InputKey::KEY_NUMPAD5;
    }

    // use standard mapping table for the rest of the keys
    return m_windowsKeyMapping[virtualKey];
}

void RawKeyboard::interpretRawInput(HWND hWnd, const RID_DEVICE_INFO* deviceInfo, const RAWINPUT* inputData)
{
    auto rawKeyboard = inputData->data.keyboard;

    auto virtualKey = rawKeyboard.VKey;
    if (virtualKey >= 255)
        return;

    // remap the left-right version of the function keys
    if (virtualKey == VK_SHIFT || virtualKey == VK_MENU || virtualKey == VK_CONTROL)
        virtualKey = (USHORT)MapVirtualKey(rawKeyboard.MakeCode, MAPVK_VSC_TO_VK_EX);

    // remap virtual key to internal key code
    bool extendedKey = 0 != (rawKeyboard.Flags & RI_KEY_E0);
    auto keyCode = mapKeyCode(virtualKey, extendedKey);
    if (keyCode != (InputKey)0)
    {
        // emit event
        if (rawKeyboard.Flags & RI_KEY_BREAK)
            keyUp(keyCode);
        else
            keyDown(keyCode);
    }
}

void RawKeyboard::interpretKeyDown(HWND hWnd, WPARAM virtualKeyCode, LPARAM flags)
{
    bool extendedKey = 0 != (flags & (1U << 24));
    auto keyCode = mapKeyCode(virtualKeyCode, extendedKey);
    if (keyCode != (InputKey)0)
        keyDown(keyCode);
}

void RawKeyboard::interpretKeyUp(HWND hWnd, WPARAM virtualKeyCode, LPARAM flags)
{
    bool extendedKey = 0 != (flags & (1U << 24));
    auto keyCode = mapKeyCode(virtualKeyCode, extendedKey);
    if (keyCode != (InputKey)0)
        keyUp(keyCode);
}

///--

RawMouse::RawMouse(InputContextWinApi* context, RawKeyboard* keyboard)
    : GenericMouse(context, keyboard)
    , m_context(context)
{}

RawMouse::~RawMouse()
{}

bool RawMouse::IsCaptureWindow(HWND hWnd)
{
    return ::GetCapture() == hWnd;
}

Rect RawMouse::GetWindowClientRect(HWND hWnd)
{
    RECT rect = {};
    ::GetClientRect(hWnd, &rect);
    return Rect(rect.left, rect.top, rect.right, rect.bottom);
}

Point RawMouse::GetMousePositionAbsolute()
{
    auto pos = ::GetMessagePos();

    Point point;
    point.x = ((int)(short)LOWORD(pos));
    point.y = ((int)(short)HIWORD(pos));
    return point;
}

Point RawMouse::GetMousePositionInWindow(HWND hWnd)
{
    // Get mouse cursor position
    auto pos = ::GetMessagePos();

    POINT point;
    point.x = ((int)(short)LOWORD(pos));
    point.y = ((int)(short)HIWORD(pos));

    // get position in the window space (note: may be negative)
    ::ScreenToClient(hWnd, &point);
    return Point(point.x, point.y);
}
        
void RawMouse::interpretRawInput(HWND hWnd, const RID_DEVICE_INFO* deviceInfo, const RAWINPUT* inputData, bool& outGotClick)
{
    const RAWMOUSE& rawMouse = inputData->data.mouse;

    // Get delta movement
    auto nativeSensitivity = 1.0f;
    auto dx = (float)rawMouse.lLastX * nativeSensitivity;
    auto dy = (float)rawMouse.lLastY * nativeSensitivity;

    // get mouse position in the window
    auto absolutePos = GetMousePositionAbsolute();
    auto windowPos = GetMousePositionInWindow(hWnd);

    // mouse events can be passed to the window only if we are in it or the mouse is captured
    auto clientRect = GetWindowClientRect(hWnd);
    auto inWindow = IsCaptureWindow(hWnd) || clientRect.contains(windowPos);

    // get mouse wheel delta
    float dz = 0.0f;
    if (rawMouse.ulButtons & RI_MOUSE_WHEEL && inWindow)
    {
        float nativeWheelSensitivity = 1.0f;
        dz = (SHORT)rawMouse.usButtonData * nativeWheelSensitivity;
    }

    // send the mouse movement
    auto captured = ::GetCapture() == hWnd;
    mouseMovement(windowPos, absolutePos, Vector3(dx, dy, dz), captured);

    // send the key events
    if (rawMouse.ulButtons & RI_MOUSE_BUTTON_1_DOWN && inWindow)
        mouseDown(0, windowPos, absolutePos);
    else if (rawMouse.ulButtons & RI_MOUSE_BUTTON_1_UP)
        mouseUp(0, windowPos, absolutePos);

    if (rawMouse.ulButtons & RI_MOUSE_BUTTON_2_DOWN && inWindow)
        mouseDown(1, windowPos, absolutePos);
    else if (rawMouse.ulButtons & RI_MOUSE_BUTTON_2_UP)
        mouseUp(1, windowPos, absolutePos);

    if (rawMouse.ulButtons & RI_MOUSE_BUTTON_3_DOWN && inWindow)
        mouseDown(2, windowPos, absolutePos);
    else if (rawMouse.ulButtons & RI_MOUSE_BUTTON_3_UP)
        mouseUp(2, windowPos, absolutePos);

    if (rawMouse.ulButtons & RI_MOUSE_BUTTON_4_DOWN && inWindow)
        mouseDown(3, windowPos, absolutePos);
    else if (rawMouse.ulButtons & RI_MOUSE_BUTTON_4_UP)
        mouseUp(3, windowPos, absolutePos);

    if (rawMouse.ulButtons & RI_MOUSE_BUTTON_5_DOWN && inWindow)
        mouseDown(4, windowPos, absolutePos);
    else if (rawMouse.ulButtons & RI_MOUSE_BUTTON_5_UP)
        mouseUp(4, windowPos, absolutePos);

    if (rawMouse.ulButtons & RI_MOUSE_BUTTON_1_DOWN && inWindow)
        outGotClick = true;
}
///--

RTTI_BEGIN_TYPE_NATIVE_CLASS(InputContextWinApi);
RTTI_END_TYPE();

InputContextWinApi::InputContextWinApi(uint64_t nativeWindow, uint64_t nativeDisplay, bool useRawInput)
    : m_hWnd((HWND)nativeWindow)
    , m_keyboard(this)
    , m_mouse(this, &m_keyboard)
    , m_useRawInput(useRawInput)
{ 
    static bool rawInputInitialized = false;
    if (!rawInputInitialized)
    {
        rawInputInitialized = true;

        // enable RawInput on both keyboard and mouse
        RAWINPUTDEVICE rawDevices[2];
        rawDevices[0].dwFlags = 0;// RIDEV_NOLEGACY;
        rawDevices[0].hwndTarget = NULL;
        rawDevices[0].usUsage = 0x06;
        rawDevices[0].usUsagePage = 0x01;

        rawDevices[1].dwFlags = 0;// RIDEV_NOLEGACY;
        rawDevices[1].hwndTarget = NULL;
        rawDevices[1].usUsage = 0x02;
        rawDevices[1].usUsagePage = 0x01;

        if (!::RegisterRawInputDevices(rawDevices, ARRAY_COUNT(rawDevices), sizeof(RAWINPUTDEVICE)))
        {
            TRACE_ERROR("Failed to initialize RawInput devices");
        }
    }
}

void InputContextWinApi::resetInput()
{
    DEBUG_CHECK_EX(IsMainThread(), "Only allowed on main thread");
    if (IsMainThread())
    {
        m_keyboard.reset();
        m_mouse.reset(false);
    }
}

void InputContextWinApi::processState()
{
    DEBUG_CHECK_EX(IsMainThread(), "Only allowed on main thread");
    if (IsMainThread())
    {
        m_keyboard.update();
        m_mouse.update();
    }
}

void InputContextWinApi::releaseCapture()
{
    if (m_activeCaptureMode)
    {
        if (m_activeCaptureMode == 2)
        {
            ::SetCursorPos(m_activeCaptureInitialMousePos.x, m_activeCaptureInitialMousePos.y);

            if (m_useRawInput)
                while (::ShowCursor(TRUE) < 0) {};

            ::ReleaseCapture();
        }
        else if (m_activeCaptureMode == 1)
        {
            ::ReleaseCapture();
        }

        m_activeCaptureMode = 0;
        m_activeCaptureInitialMousePos.x = 0;
        m_activeCaptureInitialMousePos.y = 0;
        m_activeCaptureLastMousePos.x = 0;
        m_activeCaptureLastMousePos.y = 0;

        TRACE_INFO("Mouse capture disabled");

        inject(RefNew<InputMouseCaptureLostEvent>(0));
    }
}

void InputContextWinApi::requestCapture(int captureMode)
{
    DEBUG_CHECK_EX(IsMainThread(), "Only allowed on main thread");
    if (IsMainThread())
    {
        if (m_activeCaptureMode != captureMode)
        {
            // release current capture
            releaseCapture();

            // start new capture
            const auto canCapture = (::GetActiveWindow() == m_hWnd) && !m_captureWaitingForClick;
            if (captureMode == 1 && canCapture)
            {
                ::GetCursorPos(&m_activeCaptureInitialMousePos);
                m_activeCaptureLastMousePos = m_activeCaptureInitialMousePos;
                m_activeCaptureMode = captureMode;

                ::SetCapture(m_hWnd);

                TRACE_INFO("Mouse capture enabled: {} at {}x{}", m_activeCaptureMode, m_activeCaptureInitialMousePos.x, m_activeCaptureInitialMousePos.y);
            }
            else if (captureMode == 2 && canCapture)
            {
                RECT windowRect;
                ::GetWindowRect(m_hWnd, &windowRect);

                POINT pos;
                pos.x = (windowRect.left + windowRect.right) / 2;
                pos.y = (windowRect.top + windowRect.bottom) / 2;
                ::SetCursorPos(pos.x, pos.y);

                if (m_useRawInput)
                    while (::ShowCursor(FALSE) >= 0) {};

                m_activeCaptureInitialMousePos = pos;
                m_activeCaptureLastMousePos = m_activeCaptureInitialMousePos;
                m_activeCaptureMode = captureMode;

                ::SetCapture(m_hWnd);

                TRACE_INFO("Mouse capture enabled: {} at {}x{}", m_activeCaptureMode, m_activeCaptureInitialMousePos.x, m_activeCaptureInitialMousePos.y);
            }
        }
    }
}

void InputContextWinApi::processMessage(const void* msg)
{
    DEBUG_CHECK_EX(IsMainThread(), "Only allowed on main thread");
    if (IsMainThread())
    {
        auto evt = (NativeWindowEventWinApi*)msg;
        evt->returnValue = windowProc(m_hWnd, (UINT)evt->m_message, (WPARAM)evt->m_wParam, (LPARAM)evt->m_lParam, evt->processed);
    }
}

void InputContextWinApi::processRawInput(HWND hWnd, HRAWINPUT hRawInput)
{
    // get data 
    RAWINPUT rawInput;
    UINT size = sizeof(RAWINPUT);
    ::GetRawInputData(hRawInput, RID_INPUT, &rawInput, &size, sizeof(RAWINPUTHEADER));

    // get information about source of the data
    auto& deviceHandle = rawInput.header.hDevice;
    RID_DEVICE_INFO deviceInfo;
    UINT bufferSizeTemp = sizeof(RID_DEVICE_INFO);
    ::GetRawInputDeviceInfo(deviceHandle, RIDI_DEVICEINFO, &deviceInfo, &bufferSizeTemp);

    bool gotClicked = false;

    if (deviceInfo.dwType == RIM_TYPEKEYBOARD)
        m_keyboard.interpretRawInput(hWnd, &deviceInfo, &rawInput);
    else if (deviceInfo.dwType == RIM_TYPEMOUSE)
        m_mouse.interpretRawInput(hWnd, &deviceInfo, &rawInput, gotClicked);

    if (m_captureWaitingForClick && gotClicked)
        m_captureWaitingForClick = false;

    if (m_activeCaptureMode == 2)
        ::SetCursorPos(m_activeCaptureInitialMousePos.x, m_activeCaptureInitialMousePos.y);
}

static MouseButtonIndex DecodeButtonIndex(UINT uMsg)
{
    switch (uMsg)
    {
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONUP:
        return 2;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:
        return 1;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP:
        return 0;
    }

    DEBUG_CHECK(!"Invalid message");
    return 0;
}

#pragma optimize("",off)

uint64_t InputContextWinApi::windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& processed)
{
    switch (uMsg)
    {
        case WM_INPUT:
        {
            if (m_useRawInput)
            {
                processRawInput(hWnd, reinterpret_cast<HRAWINPUT>(lParam));
                processed = true;
                return 0;
            }
            break;
        }

        case WM_SETCURSOR:
        {
            if (m_activeCaptureMode != 0)
            {
                SetCursor(NULL);
                processed = true;
                return 1;
            }
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (!m_useRawInput)
            {
                POINT pos;
                pos.x = (short)LOWORD(lParam);
                pos.y = (short)HIWORD(lParam);

                POINT screenPoint = pos;
                ::ClientToScreen(hWnd, &screenPoint);

                if (m_activeCaptureMode)
                {
                    Vector3 delta;
                    delta.x = screenPoint.x - m_activeCaptureLastMousePos.x;
                    delta.y = screenPoint.y - m_activeCaptureLastMousePos.y;
                    delta.z = 0.0f;

                    if (m_activeCaptureMode == 2)
                    {
                        if (screenPoint.x != m_activeCaptureInitialMousePos.x || screenPoint.y != m_activeCaptureInitialMousePos.y)
                            ::SetCursorPos(m_activeCaptureInitialMousePos.x, m_activeCaptureInitialMousePos.y);

                        screenPoint = m_activeCaptureInitialMousePos;
                        pos = screenPoint;
                        ::ScreenToClient(m_hWnd, &pos); // always report the original capture position
                    }
                    else if (m_activeCaptureMode == 1)
                    {
                        m_activeCaptureLastMousePos = screenPoint;
                    }

                    m_mouse.mouseMovement(Point(pos.x, pos.y), Point(screenPoint.x, screenPoint.y), delta, true);
                }
                else
                {
                    m_mouse.mouseMovement(Point(pos.x, pos.y), Point(screenPoint.x, screenPoint.y), Vector3(0, 0, 0), false);
                }

                processed = true;
                return 0;
            }

            break;
        }

        case WM_MOUSEWHEEL:
        {
            if (!m_useRawInput)
            {
                Vector3 delta;
                delta.z = (short)HIWORD(wParam) / (float)WHEEL_DELTA;

                POINT screenPos;
                screenPos.x = (short)LOWORD(lParam);
                screenPos.y = (short)HIWORD(lParam);

                POINT windowPos = screenPos;
                ::ScreenToClient(hWnd, &windowPos);

                if (m_activeCaptureMode)
                {
                    if (m_activeCaptureMode == 2)
                    {
                        screenPos = m_activeCaptureInitialMousePos;
                        windowPos = screenPos;
                        ::ScreenToClient(m_hWnd, &windowPos); // always report the original capture position
                    }
                    else if (m_activeCaptureMode == 1)
                    {
                        m_activeCaptureLastMousePos = screenPos;
                    }

                    m_mouse.mouseMovement(Point(windowPos.x, windowPos.y), Point(screenPos.x, screenPos.y), delta, true);
                }
                else
                {
                    m_mouse.mouseMovement(Point(windowPos.x, windowPos.y), Point(screenPos.x, screenPos.y), delta, false);
                }

                processed = true;
                return 0;
            }

            break;
        }

        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        {
            if (!m_useRawInput)
            {
                POINT pos;
                pos.x = (short)LOWORD(lParam);
                pos.y = (short)HIWORD(lParam);

                POINT screenPoint = pos;
                ::ClientToScreen(hWnd, &screenPoint);

                if (m_activeCaptureMode == 0)
                    ::SetActiveWindow(hWnd);

                if (m_activeCaptureMode == 2)
                {
                    screenPoint = m_activeCaptureInitialMousePos;
                    pos = screenPoint;
                    ::ScreenToClient(m_hWnd, &pos); // always report the original capture position
                }

                m_mouse.mouseDown(DecodeButtonIndex(uMsg), Point(pos.x, pos.y), Point(screenPoint.x, screenPoint.y));
                m_captureWaitingForClick = false;

                processed = true;
                return 0;
            }
            break;
        }

        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_LBUTTONUP:
        {
            if (!m_useRawInput)
            {
                POINT pos;
                pos.x = (short)LOWORD(lParam);
                pos.y = (short)HIWORD(lParam);

                POINT screenPoint = pos;
                ::ClientToScreen(hWnd, &screenPoint);

                if (m_activeCaptureMode == 2)
                {
                    screenPoint = m_activeCaptureInitialMousePos;
                    pos = screenPoint;
                    ::ScreenToClient(m_hWnd, &pos); // always report the original capture position
                }

                m_mouse.mouseUp(DecodeButtonIndex(uMsg), Point(pos.x, pos.y), Point(screenPoint.x, screenPoint.y));

                processed = true;
                return 0;
            }
            break;
        }

        case WM_SYSCHAR:
        {
            processed = true;
            return 0;
        }

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            if (!m_useRawInput)
            {
                m_keyboard.interpretKeyDown(hWnd, wParam, lParam);
                processed = true;
                return 0;
            }
            break;
        }

        case WM_KEYUP:
        {
            if (!m_useRawInput)
            {
                m_keyboard.interpretKeyUp(hWnd, wParam, lParam);
                processed = true;
                return 0;
            }
            break;
        }

        case WM_MOUSELEAVE:
        {
            if (!m_useRawInput && !m_activeCaptureMode)
            {
                POINT screenPoint;
                ::GetCursorPos(&screenPoint);

                POINT pos = screenPoint;
                ::ScreenToClient(hWnd, &pos);

                m_mouse.mouseMovement(Point(pos.x, pos.y), Point(screenPoint.x, screenPoint.y), Vector3(0, 0, 0), false);
                processed = true;
                return 0;
            }
            break;
        }

        case WM_CHAR:
        {
            m_keyboard.interpretChar(hWnd, wParam);
            processed = true;
            return 0;
        }

        case WM_CAPTURECHANGED:
        {
            TRACE_INFO("WM_CAPTURECHANGED");
            releaseCapture();
            m_mouse.reset(false); // prevent sending the "release" until next "click" is registered
            m_captureWaitingForClick = true;
            processed = true;
            return 0;
        }

        case WM_ACTIVATE:
        {
            if (wParam == WA_INACTIVE)
            {
                m_keyboard.reset();
                m_mouse.reset(false);
            }
            /*else if (m_useRawInput)
            {
                requestCapture(2);
            }*/

            break;
        }

        /*
        case WM_MOUSEACTIVATE:
            return MA_ACTIVATE;{
            // Get mouse cursor position
            auto pos = ::GetMessagePos();

            POINT point;
            point.x = ((int)(short)LOWORD(pos));
            point.y = ((int)(short)HIWORD(pos));

            Point absolutePos(point.x, point.y);

            // get position in the window space (note: may be negative)
            ::ScreenToClient(hWnd, &point);
            Point windowPos(point.x, point.y);

            // determine which button was pressed
            MouseButtonIndex buttonIndex = 0;
            if (HIWORD(lParam) == WM_MBUTTONDOWN)
                buttonIndex = 2;
            else if (HIWORD(lParam) == WM_RBUTTONDOWN)
                buttonIndex = 1;

            // create event for the input system
            m_mouse.mouseDown(buttonIndex, windowPos, absolutePos);
            SetFocus(hWnd);
            break;
        }*/
    }

    return 0;
}

//---

class ReverseKeyMap
{
public:
    InputKey m_windowsKeyMapping[256];
    DWORD m_reverseWindowsKeyMapping[(int)InputKey::KEY_MAX];

    bool m_reportedPressedState[(int)InputKey::KEY_MAX];

    ReverseKeyMap()
    {
        memzero(m_windowsKeyMapping, sizeof(m_windowsKeyMapping));
        memzero(m_reverseWindowsKeyMapping, sizeof(m_reverseWindowsKeyMapping));
        memzero(m_reportedPressedState, sizeof(m_reportedPressedState));

        BuildKeyMapping(m_windowsKeyMapping);

        for (uint32_t i = 0; i < ARRAY_COUNT(m_windowsKeyMapping); ++i)
        {
            if (m_windowsKeyMapping[i] != InputKey::KEY_INVALID)
                m_reverseWindowsKeyMapping[(int)m_windowsKeyMapping[i]] = i;
        }
    }

    bool checkKeyDown(InputKey key)
    {
        if (const auto dwVirtualKey = m_reverseWindowsKeyMapping[(int)key])
        {
            auto down = (GetAsyncKeyState(dwVirtualKey) & 1) != 0;
            if (!down)
                m_reportedPressedState[(int)key] = false;

            return down;
        }

        return false;
    }

    bool checkKeyPressed(InputKey key)
    {
        if (auto down = checkKeyDown(key))
        {
            if (!m_reportedPressedState[(int)key])
            {
                m_reportedPressedState[(int)key] = true;
                return true;
            }
        }

        return false;
    }
};

static ReverseKeyMap GReverseKeyMap;

bool CheckInputKeyState(InputKey key)
{
    return GReverseKeyMap.checkKeyDown(key);
}

bool CheckInputKeyPressed(InputKey key)
{
    return GReverseKeyMap.checkKeyPressed(key);
}

//---

END_BOOMER_NAMESPACE()
