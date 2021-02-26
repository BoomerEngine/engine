/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input\x11 #]
* [# platform: linux #]
***/

#include "build.h"
#include "inputContextX11.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/XKBlib.h>
#include "core/containers/include/utf8StringFunctions.h"

#undef bool

namespace base
{
    namespace input
    {

        //--

        X11Keyboard::X11Keyboard(IContext* context)
            : GenericKeyboard(context)
        {
            memzero(&m_keyMapping, sizeof(m_keyMapping));

            m_keyMapping[22] = KeyCode::KEY_BACK;
            m_keyMapping[23] = KeyCode::KEY_TAB;
            m_keyMapping[119] = KeyCode::KEY_CLEAR;
            m_keyMapping[36] = KeyCode::KEY_RETURN;
            m_keyMapping[127] = KeyCode::KEY_PAUSE;
            m_keyMapping[66] = KeyCode::KEY_CAPITAL;
            m_keyMapping[9] = KeyCode::KEY_ESCAPE;
            m_keyMapping[65] = KeyCode::KEY_SPACE;
            m_keyMapping[112] = KeyCode::KEY_PRIOR;
            m_keyMapping[117] = KeyCode::KEY_NEXT;
            m_keyMapping[115] = KeyCode::KEY_END;
            m_keyMapping[110] = KeyCode::KEY_HOME;
            m_keyMapping[133] = KeyCode::KEY_LEFT;
            m_keyMapping[111] = KeyCode::KEY_UP;
            m_keyMapping[114] = KeyCode::KEY_RIGHT;
            m_keyMapping[116] = KeyCode::KEY_DOWN;
            m_keyMapping[118] = KeyCode::KEY_INSERT;
            m_keyMapping[119] = KeyCode::KEY_DELETE;
            m_keyMapping[133] = KeyCode::KEY_LWIN;
            m_keyMapping[135] = KeyCode::KEY_NAVIGATION_MENU;
            m_keyMapping[90] = KeyCode::KEY_NUMPAD0;
            m_keyMapping[87] = KeyCode::KEY_NUMPAD1;
            m_keyMapping[88] = KeyCode::KEY_NUMPAD2;
            m_keyMapping[89] = KeyCode::KEY_NUMPAD3;
            m_keyMapping[83] = KeyCode::KEY_NUMPAD4;
            m_keyMapping[84] = KeyCode::KEY_NUMPAD5;
            m_keyMapping[85] = KeyCode::KEY_NUMPAD6;
            m_keyMapping[79] = KeyCode::KEY_NUMPAD7;
            m_keyMapping[80] = KeyCode::KEY_NUMPAD8;
            m_keyMapping[81] = KeyCode::KEY_NUMPAD9;
            m_keyMapping[63] = KeyCode::KEY_NUMPAD_MULTIPLY;
            m_keyMapping[86] = KeyCode::KEY_NUMPAD_ADD;
            m_keyMapping[82] = KeyCode::KEY_NUMPAD_SUBTRACT;
            m_keyMapping[91] = KeyCode::KEY_NUMPAD_DECIMAL;
            m_keyMapping[106] = KeyCode::KEY_NUMPAD_DIVIDE;

            m_keyMapping[37] = KeyCode::KEY_LEFT_CTRL;
            m_keyMapping[105] = KeyCode::KEY_RIGHT_CTRL;
            m_keyMapping[50] = KeyCode::KEY_LEFT_SHIFT;
            m_keyMapping[62] = KeyCode::KEY_RIGHT_SHIFT;
            m_keyMapping[64] = KeyCode::KEY_LEFT_ALT;
            m_keyMapping[108] = KeyCode::KEY_RIGHT_ALT;

            m_keyMapping[67] = KeyCode::KEY_F1;
            m_keyMapping[68] = KeyCode::KEY_F2;
            m_keyMapping[69] = KeyCode::KEY_F3;
            m_keyMapping[70] = KeyCode::KEY_F4;
            m_keyMapping[71] = KeyCode::KEY_F5;
            m_keyMapping[72] = KeyCode::KEY_F6;
            m_keyMapping[73] = KeyCode::KEY_F7;
            m_keyMapping[74] = KeyCode::KEY_F8;
            m_keyMapping[75] = KeyCode::KEY_F9;
            m_keyMapping[76] = KeyCode::KEY_F10;
            m_keyMapping[95] = KeyCode::KEY_F11;
            m_keyMapping[96] = KeyCode::KEY_F12;

            m_keyMapping[77] = KeyCode::KEY_NUMLOCK;
            m_keyMapping[78] = KeyCode::KEY_SCROLL;
            m_keyMapping[127] = KeyCode::KEY_PAUSE;
            m_keyMapping[107] = KeyCode::KEY_PRINT;

            m_keyMapping[20] = KeyCode::KEY_MINUS;
            m_keyMapping[21] = KeyCode::KEY_EQUAL;
            m_keyMapping[34] = KeyCode::KEY_LBRACKET;
            m_keyMapping[35] = KeyCode::KEY_RBRACKET;
            m_keyMapping[51] = KeyCode::KEY_BACKSLASH;

            m_keyMapping[47] = KeyCode::KEY_SEMICOLON;
            m_keyMapping[48] = KeyCode::KEY_APOSTROPHE;
            m_keyMapping[59] = KeyCode::KEY_COMMA;
            m_keyMapping[60] = KeyCode::KEY_PERIOD;
            m_keyMapping[61] = KeyCode::KEY_SLASH;
            m_keyMapping[49] = KeyCode::KEY_GRAVE;

            m_keyMapping[111] = KeyCode::KEY_UP;
            m_keyMapping[116] = KeyCode::KEY_DOWN;
            m_keyMapping[113] = KeyCode::KEY_LEFT;
            m_keyMapping[114] = KeyCode::KEY_RIGHT;

            m_keyMapping[10] = KeyCode::KEY_1;
            m_keyMapping[11] = KeyCode::KEY_2;
            m_keyMapping[12] = KeyCode::KEY_3;
            m_keyMapping[13] = KeyCode::KEY_4;
            m_keyMapping[14] = KeyCode::KEY_5;
            m_keyMapping[15] = KeyCode::KEY_6;
            m_keyMapping[16] = KeyCode::KEY_7;
            m_keyMapping[17] = KeyCode::KEY_8;
            m_keyMapping[18] = KeyCode::KEY_9;
            m_keyMapping[19] = KeyCode::KEY_0;

            m_keyMapping[24] = KeyCode::KEY_Q;
            m_keyMapping[25] = KeyCode::KEY_W;
            m_keyMapping[26] = KeyCode::KEY_E;
            m_keyMapping[27] = KeyCode::KEY_R;
            m_keyMapping[28] = KeyCode::KEY_T;
            m_keyMapping[29] = KeyCode::KEY_Y;
            m_keyMapping[30] = KeyCode::KEY_U;
            m_keyMapping[31] = KeyCode::KEY_I;
            m_keyMapping[32] = KeyCode::KEY_O;
            m_keyMapping[33] = KeyCode::KEY_P;

            m_keyMapping[38] = KeyCode::KEY_A;
            m_keyMapping[39] = KeyCode::KEY_S;
            m_keyMapping[40] = KeyCode::KEY_D;
            m_keyMapping[41] = KeyCode::KEY_F;
            m_keyMapping[42] = KeyCode::KEY_G;
            m_keyMapping[43] = KeyCode::KEY_H;
            m_keyMapping[44] = KeyCode::KEY_J;
            m_keyMapping[45] = KeyCode::KEY_K;
            m_keyMapping[46] = KeyCode::KEY_L;

            m_keyMapping[52] = KeyCode::KEY_Z;
            m_keyMapping[53] = KeyCode::KEY_X;
            m_keyMapping[54] = KeyCode::KEY_C;
            m_keyMapping[55] = KeyCode::KEY_V;
            m_keyMapping[56] = KeyCode::KEY_B;
            m_keyMapping[57] = KeyCode::KEY_N;
            m_keyMapping[58] = KeyCode::KEY_M;
        }

        X11Keyboard::~X11Keyboard()
        {
        }

        void X11Keyboard::nativeKeyPress(uint32_t keyCode)
        {
            // handle general key press
            if (keyCode <= 255)
            {
                auto mappedKeyCode = m_keyMapping[keyCode];
                if (mappedKeyCode != KeyCode::KEY_INVALID)
                    keyDown(mappedKeyCode);
            }
        }

        void X11Keyboard::nativeKeyRelease(uint32_t keyCode)
        {
            if (keyCode <= 255)
            {
                auto mappedKeyCode = m_keyMapping[keyCode];
                if (mappedKeyCode != KeyCode::KEY_INVALID)
                    keyUp(mappedKeyCode);
            }
        }

        void X11Keyboard::nativeKeyType(wchar_t ch)
        {
            charDown(ch);
        }

        //--

        X11Mouse::X11Mouse(ContextX11* context, GenericKeyboard* keyboard)
            : GenericMouse(context, keyboard)
            : m_context(context)
            , m_captureDisplay(nullptr)
            , m_captureWindow(0)
            , m_lastMouseMoveID(0)
            , m_lastMouseMovePos(0, 0)
        {}

        void X11Mouse::nativeButtonPress(uint32_t buttonIndex, const Point& windowPos, const Point& absolutePos)
        {
            m_lastMouseMovePos = absolutePos;
            mouseDown((MouseButtonIndex)buttonIndex, windowPos, absolutePos);
        }

        void X11Mouse::nativeButtonRelease(uint32_t buttonIndex, const Point& windowPos, const Point& absolutePos)
        {
            m_lastMouseMovePos = absolutePos;
            mouseUp((MouseButtonIndex)buttonIndex, windowPos, absolutePos);
        }

        void X11Mouse::nativeCursorMove(const Point& windowPos, const Point& absolutePos, const Point* knownDelta)
        {
            Vector3 delta(0, 0, 0);

            if (knownDelta)
            {
                delta.x = (float)knownDelta->x;
                delta.y = (float)knownDelta->y;
            }
            else if (id == m_lastMouseMoveID)
            {
                delta.x = (float)(absolutePos.x - m_lastMouseMovePos.x);
                delta.y = (float)(absolutePos.y - m_lastMouseMovePos.y);
            }

            mouseMovement(windowPos, absolutePos, delta);

            m_lastMouseMovePos = absolutePos;
        }

        void X11Mouse::nativeWheelMove(const Point& windowPos, const Point& absolutePos, int delta)
        {
            mouseMovement(windowPos, absolutePos, Vector3(0, 0, (float)delta));
        }

        bool X11Mouse::ensureCapture(const Point& windowPoint)
        {
            auto display  = m_context->nativeDisplay();
            auto window = m_context->nativeHandle();

            auto eventMask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask;// | EnterWindowMask | LeaveWindowMask;
            //if (0 == XGrabPointer(display, window, false, eventMask, GrabModeAsnyc, GrabModeAsnyc, None, None, CurrentTime))
            {
                m_captureWindow = window;
                m_captureDisplay = display;
                return true;
            }

            //retur
            //return false;
        }

        void X11Mouse::releaseCapture()
        {
            if (m_captureDisplay)
            {
                //XUngrabPointer((Display*)m_captureDisplay, CurrentTime);
                m_captureDisplay = nullptr;
                m_captureWindow = 0;
            }
        }

        bool X11Mouse::isCaptured(WindowID windowId) const
        {
            return m_captureWindow == m_context->nativeHandle();
        }

        //--

        ContextX11::ContextX11(uint64_t nativeWindow, uint64_t nativeDisplay)
            : m_window(nativeWindow)
            , m_display((void*)nativeDisplay)
            , m_keyboard(this)
            , m_mouse(this, &m_keyboard)
        {}

        void ContextX11::resetInput()
        {
            m_keyboard.reset();
            m_mouse.reset();
        }

        void ContextX11::processState()
        {
            m_keyboard.update();
            m_mouse.update();
        }

        void ContextX11::processMessage(const void* msg) override final
        {
            auto evt  = (const NativeEventX11*)msg;
            processNativeEvent(*evt);
        }

        Point absoluteCursorPos(window::IWindow* window) const
        {
            auto display = (Display *) m_display;
            auto screen = (Screen *) XDefaultScreenOfDisplay(display);
            auto rootWindow = XRootWindowOfScreen(screen);

            Window rootReturn = 0, childReturn = 0;
            int rootX = 0, rootY = 0;
            int childX = 0, childY = 0;
            uint32_t maskReturn = 0;
            if (XQueryPointer(display, rootWindow, &rootReturn, &childReturn, &rootX, &rootY, &childX, &childY, &maskReturn))
                return Point(rootX, rootY);

            return Point(0, 0);
        }

        static uint32_t TranslateButtonIndex(const XButtonEvent& ev)
        {
            if (ev.button == 1)
                return 0;
            else if (ev.button == 2)
                return 1;

            return 2;
        }

        struct DeltaMouseEvent
        {
            int type;
            Point absolutePos;
            Point windowPos;
            Point deltaPos;
        };

        void ContextX11::processNativeEvent(const NativeEventX11& evt)
        {
            Display* display = (Display*)m_display;
            Window window = (Window)m_window;

            // process the event
            auto &ev = *(const XEvent *) (evtData.m_message);
            if (ev.type == KeyPress)
            {
                // handle typing
                {
                    KeySym keysym = 0;
                    char buf[20];
                    memzero(buf, sizeof(buf));
                    Status status = 0;
                    auto ic = (XIC)evtData.m_inputContext;
                    auto count = Xutf8LookupString(ic, (XKeyPressedEvent*)&ev, buf, 20, &keysym, &status);
                    if (status != XBufferOverflow && count > 0)
                    {
                        wchar_t uniBuf[20];
                        memzero(uniBuf, sizeof(uniBuf));

                        auto uniCount = utf8::ToUniChar(uniBuf, 20, buf, count);
                        for (uint32_t i = 0; i < uniCount; ++i)
                        {
                            auto ch = uniBuf[i];
                            if ((ch >= 32 || ch == 13) && ch != 127)
                            {
                                m_keyboard.nativeKeyType(ch);
                            }
                        }
                    }
                }

                // raw key press
                m_keyboard.nativeKeyPress(ev.xkey.keycode);
            }
            else if (ev.type == KeyRelease)
            {
                m_keyboard.nativeKeyRelease(ev.xkey.keycode);
            }
            else if (ev.type == ButtonPress)
            {
                if (ev.xbutton.button == Button1)
                    m_mouse.nativeButtonPress(0, Point(ev.xbutton.x, ev.xbutton.y), Point(ev.xbutton.x_root, ev.xbutton.y_root));
                else if (ev.xbutton.button == Button2)
                    m_mouse.nativeButtonPress(2, Point(ev.xbutton.x, ev.xbutton.y), Point(ev.xbutton.x_root, ev.xbutton.y_root));
                else if (ev.xbutton.button == Button3)
                    m_mouse.nativeButtonPress(1, Point(ev.xbutton.x, ev.xbutton.y), Point(ev.xbutton.x_root, ev.xbutton.y_root));
                else if (ev.xbutton.button == Button4)
                    m_mouse.nativeWheelMove(Point(ev.xbutton.x, ev.xbutton.y), Point(ev.xbutton.x_root, ev.xbutton.y_root), 3);
                else if (ev.xbutton.button == Button5)
                    m_mouse.nativeWheelMove(Point(ev.xbutton.x, ev.xbutton.y), Point(ev.xbutton.x_root, ev.xbutton.y_root), -3);
            }
            else if (ev.type == ButtonRelease)
            {
                if (ev.xbutton.button == Button1)
                    m_mouse.nativeButtonRelease(0, Point(ev.xbutton.x, ev.xbutton.y), Point(ev.xbutton.x_root, ev.xbutton.y_root));
                else if (ev.xbutton.button == Button2)
                    m_mouse.nativeButtonRelease(2, Point(ev.xbutton.x, ev.xbutton.y), Point(ev.xbutton.x_root, ev.xbutton.y_root));
                else if (ev.xbutton.button == Button3)
                    m_mouse.nativeButtonRelease(1, Point(ev.xbutton.x, ev.xbutton.y), Point(ev.xbutton.x_root, ev.xbutton.y_root));
            }
            else if (ev.type == MotionNotify)
            {
                // make the current window active
                m_mouse.nativeCursorMove(Point(ev.xmotion.x, ev.xmotion.y), Point(ev.xmotion.x_root, ev.xmotion.y_root), nullptr);
            }
            else if (ev.type == LASTEvent + 666)
            {
                auto& data = *(const DeltaMouseEvent*) &ev.xmotion;
                m_mouse.nativeCursorMove(data.windowPos, data.absolutePos, &data.deltaPos);
            }
            else if (ev.type == FocusOut || ev.type == FocusIn)
            {
                m_keyboard.reset();
            }
        }

    } // input
} // base