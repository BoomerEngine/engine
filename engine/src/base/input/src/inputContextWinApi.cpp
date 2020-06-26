/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#include "build.h"
#include "inputContextWinApi.h"

namespace base
{
    namespace input
    {
        ///--

        RawKeyboard::RawKeyboard(IContext* context)
            : GenericKeyboard(context)
        {
            memzero(&m_windowsKeyMapping, sizeof(m_windowsKeyMapping));
            m_windowsKeyMapping[VK_BACK] = KeyCode::KEY_BACK;
            m_windowsKeyMapping[VK_TAB] = KeyCode::KEY_TAB;
            m_windowsKeyMapping[VK_CLEAR] = KeyCode::KEY_CLEAR;
            m_windowsKeyMapping[VK_RETURN] = KeyCode::KEY_RETURN;
            m_windowsKeyMapping[VK_PAUSE] = KeyCode::KEY_PAUSE;
            m_windowsKeyMapping[VK_CAPITAL] = KeyCode::KEY_CAPITAL;
            m_windowsKeyMapping[VK_ESCAPE] = KeyCode::KEY_ESCAPE;
            m_windowsKeyMapping[VK_CONVERT] = KeyCode::KEY_CONVERT;
            m_windowsKeyMapping[VK_NONCONVERT] = KeyCode::KEY_NONCONVERT;
            m_windowsKeyMapping[VK_ACCEPT] = KeyCode::KEY_ACCEPT;
            m_windowsKeyMapping[VK_MODECHANGE] = KeyCode::KEY_MODECHANGE;
            m_windowsKeyMapping[VK_SPACE] = KeyCode::KEY_SPACE;
            m_windowsKeyMapping[VK_PRIOR] = KeyCode::KEY_PRIOR;
            m_windowsKeyMapping[VK_NEXT] = KeyCode::KEY_NEXT;
            m_windowsKeyMapping[VK_END] = KeyCode::KEY_END;
            m_windowsKeyMapping[VK_HOME] = KeyCode::KEY_HOME;
            m_windowsKeyMapping[VK_LEFT] = KeyCode::KEY_LEFT;
            m_windowsKeyMapping[VK_UP] = KeyCode::KEY_UP;
            m_windowsKeyMapping[VK_RIGHT] = KeyCode::KEY_RIGHT;
            m_windowsKeyMapping[VK_DOWN] = KeyCode::KEY_DOWN;
            m_windowsKeyMapping[VK_SELECT] = KeyCode::KEY_SELECT;
            m_windowsKeyMapping[VK_PRINT] = KeyCode::KEY_PRINT;
            m_windowsKeyMapping[VK_SNAPSHOT] = KeyCode::KEY_SNAPSHOT;
            m_windowsKeyMapping[VK_INSERT] = KeyCode::KEY_INSERT;
            m_windowsKeyMapping[VK_DELETE] = KeyCode::KEY_DELETE;
            m_windowsKeyMapping[VK_HELP] = KeyCode::KEY_HELP;
            m_windowsKeyMapping[VK_LWIN] = KeyCode::KEY_LWIN;
            m_windowsKeyMapping[VK_RWIN] = KeyCode::KEY_RWIN;
            m_windowsKeyMapping[VK_APPS] = KeyCode::KEY_APPS;
            m_windowsKeyMapping[VK_SLEEP] = KeyCode::KEY_SLEEP;
            m_windowsKeyMapping[VK_OEM_PLUS] = KeyCode::KEY_EQUAL;
            m_windowsKeyMapping[VK_OEM_MINUS] = KeyCode::KEY_MINUS;
            m_windowsKeyMapping[VK_OEM_COMMA] = KeyCode::KEY_COMMA;
            m_windowsKeyMapping[VK_OEM_PERIOD] = KeyCode::KEY_PERIOD;
            m_windowsKeyMapping[VK_OEM_1] = KeyCode::KEY_SEMICOLON;
            m_windowsKeyMapping[VK_OEM_2] = KeyCode::KEY_BACKSLASH;
            m_windowsKeyMapping[VK_OEM_3] = KeyCode::KEY_GRAVE;
            m_windowsKeyMapping[VK_OEM_4] = KeyCode::KEY_LBRACKET;
            m_windowsKeyMapping[VK_OEM_5] = KeyCode::KEY_SLASH;
            m_windowsKeyMapping[VK_OEM_6] = KeyCode::KEY_RBRACKET;
            m_windowsKeyMapping[VK_OEM_7] = KeyCode::KEY_APOSTROPHE;
            m_windowsKeyMapping[VK_NUMPAD0] = KeyCode::KEY_NUMPAD0;
            m_windowsKeyMapping[VK_NUMPAD1] = KeyCode::KEY_NUMPAD1;
            m_windowsKeyMapping[VK_NUMPAD2] = KeyCode::KEY_NUMPAD2;
            m_windowsKeyMapping[VK_NUMPAD3] = KeyCode::KEY_NUMPAD3;
            m_windowsKeyMapping[VK_NUMPAD4] = KeyCode::KEY_NUMPAD4;
            m_windowsKeyMapping[VK_NUMPAD5] = KeyCode::KEY_NUMPAD5;
            m_windowsKeyMapping[VK_NUMPAD6] = KeyCode::KEY_NUMPAD6;
            m_windowsKeyMapping[VK_NUMPAD7] = KeyCode::KEY_NUMPAD7;
            m_windowsKeyMapping[VK_NUMPAD8] = KeyCode::KEY_NUMPAD8;
            m_windowsKeyMapping[VK_NUMPAD9] = KeyCode::KEY_NUMPAD9;
            m_windowsKeyMapping[VK_MULTIPLY] = KeyCode::KEY_NUMPAD_MULTIPLY;
            m_windowsKeyMapping[VK_ADD] = KeyCode::KEY_NUMPAD_ADD;
            m_windowsKeyMapping[VK_SEPARATOR] = KeyCode::KEY_NUMPAD_SEPARATOR;
            m_windowsKeyMapping[VK_SUBTRACT] = KeyCode::KEY_NUMPAD_SUBTRACT;
            m_windowsKeyMapping[VK_DECIMAL] = KeyCode::KEY_NUMPAD_DECIMAL;
            m_windowsKeyMapping[VK_DIVIDE] = KeyCode::KEY_NUMPAD_DIVIDE;
            m_windowsKeyMapping[VK_LCONTROL] = KeyCode::KEY_LEFT_CTRL;
            m_windowsKeyMapping[VK_RCONTROL] = KeyCode::KEY_RIGHT_CTRL;
            m_windowsKeyMapping[VK_LSHIFT] = KeyCode::KEY_LEFT_SHIFT;
            m_windowsKeyMapping[VK_RSHIFT] = KeyCode::KEY_RIGHT_SHIFT;
            m_windowsKeyMapping[VK_LMENU] = KeyCode::KEY_LEFT_ALT;
            m_windowsKeyMapping[VK_RMENU] = KeyCode::KEY_RIGHT_ALT;
            m_windowsKeyMapping[VK_F1] = KeyCode::KEY_F1;
            m_windowsKeyMapping[VK_F2] = KeyCode::KEY_F2;
            m_windowsKeyMapping[VK_F3] = KeyCode::KEY_F3;
            m_windowsKeyMapping[VK_F4] = KeyCode::KEY_F4;
            m_windowsKeyMapping[VK_F5] = KeyCode::KEY_F5;
            m_windowsKeyMapping[VK_F6] = KeyCode::KEY_F6;
            m_windowsKeyMapping[VK_F7] = KeyCode::KEY_F7;
            m_windowsKeyMapping[VK_F8] = KeyCode::KEY_F8;
            m_windowsKeyMapping[VK_F9] = KeyCode::KEY_F9;
            m_windowsKeyMapping[VK_F10] = KeyCode::KEY_F10;
            m_windowsKeyMapping[VK_F11] = KeyCode::KEY_F11;
            m_windowsKeyMapping[VK_F12] = KeyCode::KEY_F12;
            m_windowsKeyMapping[VK_F13] = KeyCode::KEY_F13;
            m_windowsKeyMapping[VK_F14] = KeyCode::KEY_F14;
            m_windowsKeyMapping[VK_F15] = KeyCode::KEY_F15;
            m_windowsKeyMapping[VK_F16] = KeyCode::KEY_F16;
            m_windowsKeyMapping[VK_F17] = KeyCode::KEY_F17;
            m_windowsKeyMapping[VK_F18] = KeyCode::KEY_F18;
            m_windowsKeyMapping[VK_F19] = KeyCode::KEY_F19;
            m_windowsKeyMapping[VK_F20] = KeyCode::KEY_F20;
            m_windowsKeyMapping[VK_F21] = KeyCode::KEY_F21;
            m_windowsKeyMapping[VK_F22] = KeyCode::KEY_F22;
            m_windowsKeyMapping[VK_F23] = KeyCode::KEY_F23;
            m_windowsKeyMapping[VK_F24] = KeyCode::KEY_F24;
#if(_WIN32_WINNT >= 0x0604)
            m_windowsKeyMapping[VK_NAVIGATION_VIEW] = KeyCode::KEY_NAVIGATION_VIEW;
            m_windowsKeyMapping[VK_NAVIGATION_MENU] = KeyCode::KEY_NAVIGATION_MENU;
            m_windowsKeyMapping[VK_NAVIGATION_UP] = KeyCode::KEY_NAVIGATION_UP;
            m_windowsKeyMapping[VK_NAVIGATION_DOWN] = KeyCode::KEY_NAVIGATION_DOWN;
            m_windowsKeyMapping[VK_NAVIGATION_LEFT] = KeyCode::KEY_NAVIGATION_LEFT;
            m_windowsKeyMapping[VK_NAVIGATION_RIGHT] = KeyCode::KEY_NAVIGATION_RIGHT;
            m_windowsKeyMapping[VK_NAVIGATION_ACCEPT] = KeyCode::KEY_NAVIGATION_ACCEPT;
            m_windowsKeyMapping[VK_NAVIGATION_CANCEL] = KeyCode::KEY_NAVIGATION_CANCEL;
#endif
            m_windowsKeyMapping[VK_NUMLOCK] = KeyCode::KEY_NUMLOCK;
            m_windowsKeyMapping[VK_SCROLL] = KeyCode::KEY_SCROLL;
            m_windowsKeyMapping['0'] = KeyCode::KEY_0;
            m_windowsKeyMapping['1'] = KeyCode::KEY_1;
            m_windowsKeyMapping['2'] = KeyCode::KEY_2;
            m_windowsKeyMapping['3'] = KeyCode::KEY_3;
            m_windowsKeyMapping['4'] = KeyCode::KEY_4;
            m_windowsKeyMapping['5'] = KeyCode::KEY_5;
            m_windowsKeyMapping['6'] = KeyCode::KEY_6;
            m_windowsKeyMapping['7'] = KeyCode::KEY_7;
            m_windowsKeyMapping['8'] = KeyCode::KEY_8;
            m_windowsKeyMapping['9'] = KeyCode::KEY_9;
            m_windowsKeyMapping['A'] = KeyCode::KEY_A;
            m_windowsKeyMapping['B'] = KeyCode::KEY_B;
            m_windowsKeyMapping['C'] = KeyCode::KEY_C;
            m_windowsKeyMapping['D'] = KeyCode::KEY_D;
            m_windowsKeyMapping['E'] = KeyCode::KEY_E;
            m_windowsKeyMapping['F'] = KeyCode::KEY_F;
            m_windowsKeyMapping['G'] = KeyCode::KEY_G;
            m_windowsKeyMapping['H'] = KeyCode::KEY_H;
            m_windowsKeyMapping['I'] = KeyCode::KEY_I;
            m_windowsKeyMapping['J'] = KeyCode::KEY_J;
            m_windowsKeyMapping['K'] = KeyCode::KEY_K;
            m_windowsKeyMapping['L'] = KeyCode::KEY_L;
            m_windowsKeyMapping['M'] = KeyCode::KEY_M;
            m_windowsKeyMapping['N'] = KeyCode::KEY_N;
            m_windowsKeyMapping['O'] = KeyCode::KEY_O;
            m_windowsKeyMapping['P'] = KeyCode::KEY_P;
            m_windowsKeyMapping['Q'] = KeyCode::KEY_Q;
            m_windowsKeyMapping['R'] = KeyCode::KEY_R;
            m_windowsKeyMapping['S'] = KeyCode::KEY_S;
            m_windowsKeyMapping['T'] = KeyCode::KEY_T;
            m_windowsKeyMapping['U'] = KeyCode::KEY_U;
            m_windowsKeyMapping['V'] = KeyCode::KEY_V;
            m_windowsKeyMapping['W'] = KeyCode::KEY_W;
            m_windowsKeyMapping['X'] = KeyCode::KEY_X;
            m_windowsKeyMapping['Y'] = KeyCode::KEY_Y;
            m_windowsKeyMapping['Z'] = KeyCode::KEY_Z;

            m_windowsKeyMapping[VK_OEM_3] = KeyCode::KEY_GRAVE;

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

        KeyCode RawKeyboard::mapKeyCode(USHORT virtualKey, bool flag0) const
        {
            // handle special keys
            switch (virtualKey)
            {
            case VK_CONTROL:
                return flag0 ? KeyCode::KEY_RIGHT_CTRL : KeyCode::KEY_LEFT_CTRL;

            case VK_MENU:
                return flag0 ? KeyCode::KEY_RIGHT_ALT : KeyCode::KEY_LEFT_ALT;

            case VK_SHIFT:
                return flag0 ? KeyCode::KEY_RIGHT_SHIFT : KeyCode::KEY_LEFT_SHIFT;

            case VK_INSERT:
                return flag0 ? KeyCode::KEY_INSERT : KeyCode::KEY_NUMPAD0;

            case VK_DELETE:
                return flag0 ? KeyCode::KEY_DELETE : KeyCode::KEY_NUMPAD_DECIMAL;

            case VK_HOME:
                return flag0 ? KeyCode::KEY_HOME : KeyCode::KEY_NUMPAD1;

            case VK_END:
                return flag0 ? KeyCode::KEY_END : KeyCode::KEY_NUMPAD7;

            case VK_PRIOR:
                return flag0 ? KeyCode::KEY_PRIOR : KeyCode::KEY_NUMPAD9;

            case VK_NEXT:
                return flag0 ? KeyCode::KEY_NEXT : KeyCode::KEY_NUMPAD3;

            case VK_UP:
                return flag0 ? KeyCode::KEY_UP : KeyCode::KEY_NUMPAD2;

            case VK_LEFT:
                return flag0 ? KeyCode::KEY_LEFT : KeyCode::KEY_NUMPAD4;

            case VK_RIGHT:
                return flag0 ? KeyCode::KEY_RIGHT : KeyCode::KEY_NUMPAD6;

            case VK_DOWN:
                return flag0 ? KeyCode::KEY_DOWN : KeyCode::KEY_NUMPAD8;

            case VK_CLEAR:
                return KeyCode::KEY_NUMPAD5;
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
            if (keyCode != (KeyCode)0)
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
            if (keyCode != (KeyCode)0)
                keyDown(keyCode);
        }

        void RawKeyboard::interpretKeyUp(HWND hWnd, WPARAM virtualKeyCode, LPARAM flags)
        {
            bool extendedKey = 0 != (flags & (1U << 24));
            auto keyCode = mapKeyCode(virtualKeyCode, extendedKey);
            if (keyCode != (KeyCode)0)
                keyUp(keyCode);
        }

        ///--

        RawMouse::RawMouse(ContextWinApi* context, RawKeyboard* keyboard)
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
        
        void RawMouse::interpretRawInput(HWND hWnd, const RID_DEVICE_INFO* deviceInfo, const RAWINPUT* inputData)
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
        }
        ///--

        RTTI_BEGIN_TYPE_NATIVE_CLASS(ContextWinApi);
        RTTI_END_TYPE();

        ContextWinApi::ContextWinApi(uint64_t nativeWindow, uint64_t nativeDisplay, bool useRawInput)
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

        void ContextWinApi::resetInput()
        {
            DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Only allowed on main thread");
            if (Fibers::GetInstance().isMainThread())
            {
                m_keyboard.reset();
                m_mouse.reset(false);
            }
        }

        void ContextWinApi::processState()
        {
            DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Only allowed on main thread");
            if (Fibers::GetInstance().isMainThread())
            {
                m_keyboard.update();
                m_mouse.update();
            }
        }

        void ContextWinApi::releaseCapture()
        {
            if (m_activeCaptureMode)
            {
                if (m_activeCaptureMode == 2)
                {
                    ::SetCursorPos(m_activeCaptureInitialMousePos.x, m_activeCaptureInitialMousePos.y);
                    ::ShowCursor(TRUE);
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

                inject(CreateSharedPtr<MouseCaptureLostEvent>(0));
            }
        }

        void ContextWinApi::requestCapture(int captureMode)
        {
            DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Only allowed on main thread");
            if (Fibers::GetInstance().isMainThread())
            {
                if (m_activeCaptureMode != captureMode)
                {
                    // release current capture
                    releaseCapture();

                    // start new capture
                    if (captureMode == 1 || captureMode == 2)
                    {
                        ::GetCursorPos(&m_activeCaptureInitialMousePos);
                        m_activeCaptureLastMousePos = m_activeCaptureInitialMousePos;
                        m_activeCaptureMode = captureMode;

                        ::SetCapture(m_hWnd);

                        if (m_activeCaptureMode == 2)
                            ::ShowCursor(FALSE);

                        TRACE_INFO("Mouse capture enabled: {} at {}x{}", m_activeCaptureMode, m_activeCaptureInitialMousePos.x, m_activeCaptureInitialMousePos.y);
                    }
                }
            }
        }

        void ContextWinApi::processMessage(const void* msg)
        {
            DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Only allowed on main thread");
            if (Fibers::GetInstance().isMainThread())
            {
                auto evt = (NativeEventWinApi*)msg;
                evt->returnValue = windowProc(m_hWnd, (UINT)evt->m_message, (WPARAM)evt->m_wParam, (LPARAM)evt->m_lParam, evt->processed);
            }
        }

        void ContextWinApi::processRawInput(HWND hWnd, HRAWINPUT hRawInput)
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

            if (deviceInfo.dwType == RIM_TYPEKEYBOARD)
                m_keyboard.interpretRawInput(hWnd, &deviceInfo, &rawInput);
            else if (deviceInfo.dwType == RIM_TYPEMOUSE)
                m_mouse.interpretRawInput(hWnd, &deviceInfo, &rawInput);
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

        uint64_t ContextWinApi::windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& processed)
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

                        POINT pos;
                        pos.x = (short)LOWORD(lParam);
                        pos.y = (short)HIWORD(lParam);

                        POINT screenPoint = pos;
                        ::ClientToScreen(hWnd, &screenPoint);

                        if (m_activeCaptureMode)
                        {
                            if (m_activeCaptureMode == 2)
                            {
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
                            m_mouse.mouseMovement(Point(pos.x, pos.y), Point(screenPoint.x, screenPoint.y), delta, false);
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
                    else if (m_useRawInput)
                    {
                        requestCapture(2);
                    }

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

        ///--

    } // input
} // base
