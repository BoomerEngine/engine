/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# platform: winapi #]
***/

#include "build.h"
#include "renderingWindowWinApi.h"

#include "base/input/include/inputContext.h"

#include <dwmapi.h>
#include <shellapi.h>

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Shcore.lib")

#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00AF)
#endif

namespace rendering
{
      
    //--

    WindowWinApi::WindowWinApi(ObjectID owner)
        : m_owner(owner)
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be accessed on main thread");
        m_nextFPSCapture = base::NativeTimePoint::Now() + 1.0;
    }

    WindowWinApi::~WindowWinApi()
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be accessed on main thread");
        if (m_hWnd != NULL)
        {
            DestroyWindow(m_hWnd);
            m_hWnd = NULL;
        }

        m_inputContext.reset();
    }

    bool WindowWinApi::prepareWindowForRendering(uint16_t& outWidth, uint16_t& outHeight)
    {
        auto lock = CreateLock(m_windowLock);

        // do not render to invisible windows
        if (m_initialShowDone && (!IsWindowVisible(m_hWnd) || IsIconic(m_hWnd)))
            return false;

        // check size
        // TODO: use cached size ?
        RECT rect;
        GetClientRect(m_hWnd, &rect);
        auto clientWidth = rect.right - rect.left;
        auto clientHeight = rect.bottom - rect.top;

        // check size
        if (clientWidth <= 0 || clientHeight <= 0)
            return false;

        // we are already rendering
        m_numFramesStarted++;

        // ok, we can render
        outWidth = clientWidth;
        outHeight = clientHeight;
        return true;
    }

    void WindowWinApi::disconnectWindow()
    {
        m_callback = nullptr;
        SetWindowLongPtr(m_hWnd, GWLP_USERDATA, 0);
    }

    void WindowWinApi::releaseWindowFromRendering()
    {
        m_numFramesRendered += 1;
    }

    static void MessageLoop(HWND hWnd)
    {
        MSG msg;
        while (PeekMessageW(&msg, hWnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void WindowWinApi::update()
    {
        PC_SCOPE_LVL0(WindowUpdate);

        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be accessed on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            // update the FPS display
            if (m_nextFPSCapture.reached())
            {
                m_numLastFramesRendered = m_numFramesRendered;
                m_nextFPSCapture = base::NativeTimePoint::Now() + 1.0;
                m_numFramesRendered = 0;
                updateTitle_NoLock();
            }
        }
    }

    void WindowWinApi::RegisterWindowClass()
    {
        static bool classRegisterd = false;
        if (!classRegisterd)
        {
            classRegisterd = true;

            WNDCLASSEXW cls;

            // Create the application window
            memzero(&cls, sizeof(cls));
            cls.cbSize = sizeof(WNDCLASSEXW);
            cls.style = CS_VREDRAW | CS_HREDRAW;
            cls.lpfnWndProc = &WindowWinApi::WindowProc;
            cls.cbClsExtra = 0;
            cls.cbWndExtra = sizeof(void*);
            cls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            cls.hCursor = LoadCursor(NULL, IDC_ARROW);
            cls.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
            cls.lpszMenuName = NULL;
            cls.lpszClassName = L"BoomerEngineGL4WindowClass";
            cls.hInstance = GetModuleHandle(NULL);
            cls.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

            // Register the application
            if (!RegisterClassExW(&cls))
            {
                TRACE_ERROR("Failed to register window class");
            }
        }
    }

    WindowWinApi* WindowWinApi::Create(ObjectID owner, const DriverOutputInitInfo& creationInfo)
    {
        ASSERT_EX(Fibers::GetInstance().isMainThread(), "Windows can only be created on main thread");

        RegisterWindowClass();

        // get parent window handle
        HWND hParent = NULL;
        if (0 != creationInfo.m_windowNativeParent)
            hParent = (HWND)creationInfo.m_windowNativeParent;

        // full screen window
        int offsetX = creationInfo.m_windowPlacementX;
        int offsetY = creationInfo.m_windowPlacementY;
        int sizeX = creationInfo.m_width;
        int sizeY = creationInfo.m_height;
        DWORD dwStyle = 0, dwExStyle = 0;
        if (creationInfo.m_class == DriverOutputClass::Fullscreen)
        {
            dwStyle |= WS_POPUP | WS_MAXIMIZE;

            POINT point;
            ::GetCursorPos(&point);

            sizeX = GetSystemMetrics(SM_CXSCREEN);
            sizeY = GetSystemMetrics(SM_CYSCREEN);
        }
        else
        {
            // child window style
            if (creationInfo.m_windowPopup)
            {
                dwStyle |= WS_POPUP;
            }
            else
            {
                dwExStyle = (creationInfo.m_windowShowOnTaskBar ? WS_EX_APPWINDOW : WS_EX_TOOLWINDOW) | WS_EX_LAYERED;

                if (creationInfo.m_windowMaximized)
                    dwStyle |= WS_MAXIMIZE;
                if (creationInfo.m_windowMinimized)
                    dwStyle |= WS_MINIMIZE;

                if (creationInfo.m_windowSystemBorder)
                {
                    dwStyle |= WS_SYSMENU | WS_ACTIVECAPTION | WS_BORDER | WS_CAPTION;

                    if (creationInfo.m_windowCanResize)
                        dwStyle |= WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
                }
                else
                {
                    dwStyle |= WS_OVERLAPPEDWINDOW | WS_SIZEBOX;
                }
            }

            if (creationInfo.m_windowTopmost)
                dwExStyle |= WS_EX_TOPMOST;

            // compute offset
            if (hParent == NULL)
            {
                if (offsetX == INT_MIN)
                {
                    auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
                    offsetX = (screenWidth - creationInfo.m_width) / 2;
                }
                if (offsetY == INT_MIN)
                {
                    auto screenHeight = GetSystemMetrics(SM_CYSCREEN);
                    offsetY = (screenHeight - creationInfo.m_height) / 2;
                }
            }
        }

        // compute position
        RECT windowRect;
        windowRect.left = offsetX;
        windowRect.top = offsetY;
        windowRect.right = offsetX + sizeX;
        windowRect.bottom = offsetY + sizeY;
        if (creationInfo.m_windowAdjustArea)
            AdjustWindowRectEx(&windowRect, dwStyle, NULL, dwExStyle);

        // create window
        HWND hWnd = CreateWindowExW(
            dwExStyle,
            L"BoomerEngineGL4WindowClass",
            creationInfo.m_windowTitle.uni_str().c_str(),
            dwStyle,
            windowRect.left,
            windowRect.top,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            hParent,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        if (NULL == hWnd)
        {
            TRACE_ERROR("Unable to create output window");
            return nullptr;
        }

        // set the window as topmost
        if (creationInfo.m_windowTopmost)
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        // get the pixel scale
#if (WINVER >= 0x0605)
        auto pixelScale = GetDpiForWindow(hWnd) / 96.0f;
#else
        auto pixelScale = 1.0f;
#endif

        // set window transparency
        //if (creationInfo.m_windowOpacity != 1.0f)
        {
            auto alpha = std::clamp<int>((int)(creationInfo.m_windowOpacity * 255.0f), 0, 255);
            ::SetLayeredWindowAttributes(hWnd, 0, (uint8_t)alpha, LWA_ALPHA);
        }

#if WINVER > 0x502  
        // disable title bar and frame drawing 
        if (!creationInfo.m_windowSystemBorder && (NULL == hParent) && !creationInfo.m_windowPopup)
        {
            DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;// DWMNCRP_DISABLED;
            ::DwmSetWindowAttribute(hWnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));

            MARGINS margins = { 0, 0, 1, 0 };
            DwmExtendFrameIntoClientArea(hWnd, &margins);

            //BOOL useNCPaint = true;
            //::DwmSetWindowAttribute(hWnd, DWMWA_ALLOW_NCPAINT, &useNCPaint, sizeof(useNCPaint));
        }
#endif

        // show the window
        if (creationInfo.m_windowShow)
        {
            if (creationInfo.m_windowMaximized)
                ShowWindow(hWnd, SW_MAXIMIZE);
            else if (creationInfo.m_windowMinimized)
                ShowWindow(hWnd, SW_MINIMIZE);
            else if (creationInfo.m_windowActivate)
                ShowWindow(hWnd, SW_SHOW);
            else
                ShowWindow(hWnd, SW_SHOWNOACTIVATE);
        }

        // create the holder object
        auto ret = MemNew(WindowWinApi, owner);
        ret->m_hWnd = hWnd;
        ret->m_currentPixelScale = pixelScale;
        ret->m_currentTitle = creationInfo.m_windowTitle;
        ret->m_currentActiveFlag = creationInfo.m_windowActivate;
        ret->m_hasSystemBorder = creationInfo.m_windowSystemBorder && !creationInfo.m_windowPopup;

        // show state
        ret->m_initialShowDone = creationInfo.m_windowShow;
        ret->m_initialShowMinimize = creationInfo.m_windowMinimized;
        ret->m_initialShowMaximize = creationInfo.m_windowMaximized;
        ret->m_initialShowActivate = creationInfo.m_windowActivate;

        // mount callback
        ret->m_callback = creationInfo.m_windowCallback;

        // create input callback
        if (creationInfo.m_windowCreateInputContext)
            ret->m_inputContext = base::input::IContext::CreateNativeContext((uint64_t)hWnd, 0, creationInfo.m_windowInputContextGameMode);

        // bind data
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ret.ptr);
        return ret;
    }

    //--

    static base::Point GetAbsoluteMousePosition()
    {
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        return base::Point(cursorPos.x, cursorPos.y);
    }
    static bool HasAutohideAppbar(UINT edge, RECT mon)
    {
        APPBARDATA data;
        memzero(&data, sizeof(data));
        data.cbSize = sizeof(APPBARDATA);
        data.uEdge = edge;
        data.rc = mon;
        return ::SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &data);
    }

    LRESULT WindowWinApi::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_MOUSEACTIVATE:
                return MA_ACTIVATE;

            case WM_ACTIVATE:
            {
                auto isActive = (wParam != WA_INACTIVE);
                TRACE_INFO("Active state for window {}: {}, current {}", Hex(m_hWnd), isActive, m_currentActiveFlag);
                if (m_currentActiveFlag != isActive || !m_currentActiveFlagSeen)
                {
                    m_currentActiveFlag = isActive;
                    m_currentActiveFlagSeen = true;
                    if (m_callback)
                        m_callback->onOutputWindowStateChanged(m_owner, isActive);
                }

                break;
            }

            case WM_MOUSEMOVE:
            {
                if (GetCapture() == m_hWnd)
                {
                    if (m_callback)
                    {
                        auto pos = GetAbsoluteMousePosition();

                        auto cursorType = base::input::CursorType::Default;
                        m_callback->onOutputWindowSelectCursor(m_owner, pos, cursorType);

                        switch (cursorType)
                        {
                        case base::input::CursorType::Hidden: SetCursor(NULL); break;
                        case base::input::CursorType::Default: SetCursor(LoadCursor(NULL, IDC_ARROW)); break;
                        case base::input::CursorType::Cross: SetCursor(LoadCursor(NULL, IDC_CROSS)); break;
                        case base::input::CursorType::Hand: SetCursor(LoadCursor(NULL, IDC_HAND)); break;
                        case base::input::CursorType::Help: SetCursor(LoadCursor(NULL, IDC_HELP)); break;
                        case base::input::CursorType::TextBeam: SetCursor(LoadCursor(NULL, IDC_IBEAM)); break;
                        case base::input::CursorType::No: SetCursor(LoadCursor(NULL, IDC_NO)); break;
                        case base::input::CursorType::SizeAll: SetCursor(LoadCursor(NULL, IDC_SIZEALL)); break;
                        case base::input::CursorType::SizeNS: SetCursor(LoadCursor(NULL, IDC_SIZENS)); break;
                        case base::input::CursorType::SizeWE: SetCursor(LoadCursor(NULL, IDC_SIZEWE)); break;
                        case base::input::CursorType::SizeNESW: SetCursor(LoadCursor(NULL, IDC_SIZENESW)); break;
                        case base::input::CursorType::SizeNWSE: SetCursor(LoadCursor(NULL, IDC_SIZENWSE)); break;
                        case base::input::CursorType::UpArrow: SetCursor(LoadCursor(NULL, IDC_UPARROW)); break;
                        case base::input::CursorType::Wait: SetCursor(LoadCursor(NULL, IDC_WAIT)); break;
                        default: ASSERT(!"Invalid window cursor");
                        }
                    }
                }

                break;
            }

            case WM_SETCURSOR:
            {
                // we only pass this in case we are asking about stuff inside the client area
                if ((LOWORD(lParam) == HTCLIENT) && m_callback)
                {
                    auto pos = GetAbsoluteMousePosition();

                    auto cursorType = base::input::CursorType::Default;
                    m_callback->onOutputWindowSelectCursor(m_owner, pos, cursorType);

                    switch (cursorType)
                    {
                        case base::input::CursorType::Hidden: SetCursor(NULL); break;
                        case base::input::CursorType::Default: SetCursor(LoadCursor(NULL, IDC_ARROW)); break;
                        case base::input::CursorType::Cross: SetCursor(LoadCursor(NULL, IDC_CROSS)); break;
                        case base::input::CursorType::Hand: SetCursor(LoadCursor(NULL, IDC_HAND)); break;
                        case base::input::CursorType::Help: SetCursor(LoadCursor(NULL, IDC_HELP)); break;
                        case base::input::CursorType::TextBeam: SetCursor(LoadCursor(NULL, IDC_IBEAM)); break;
                        case base::input::CursorType::No: SetCursor(LoadCursor(NULL, IDC_NO)); break;
                        case base::input::CursorType::SizeAll: SetCursor(LoadCursor(NULL, IDC_SIZEALL)); break;
                        case base::input::CursorType::SizeNS: SetCursor(LoadCursor(NULL, IDC_SIZENS)); break;
                        case base::input::CursorType::SizeWE: SetCursor(LoadCursor(NULL, IDC_SIZEWE)); break;
                        case base::input::CursorType::SizeNESW: SetCursor(LoadCursor(NULL, IDC_SIZENESW)); break;
                        case base::input::CursorType::SizeNWSE: SetCursor(LoadCursor(NULL, IDC_SIZENWSE)); break;
                        case base::input::CursorType::UpArrow: SetCursor(LoadCursor(NULL, IDC_UPARROW)); break;
                        case base::input::CursorType::Wait: SetCursor(LoadCursor(NULL, IDC_WAIT)); break;
                        default: ASSERT(!"Invalid window cursor");
                    }

                    return TRUE;
                }
                break;
            }

            case WM_NCHITTEST:
            {
                if (m_callback != nullptr && !m_hasSystemBorder)
                {
                    base::Point pos;
                    pos.x = (int)(short)LOWORD(lParam);
                    pos.y = (int)(short)HIWORD(lParam);

                    auto area = base::input::AreaType::Client;
                    if (m_callback->onOutputWindowHitTestNonClientArea(m_owner, pos, area))
                    {
                        switch (area)
                        {
                            case base::input::AreaType::NotInWindow: return HTTRANSPARENT;
                            case base::input::AreaType::Client: return HTCLIENT;
                            case base::input::AreaType::NonSizableBorder: return HTBORDER;
                            case base::input::AreaType::BorderBottom: return HTBOTTOM;
                            case base::input::AreaType::BorderBottomLeft: return HTBOTTOMLEFT;
                            case base::input::AreaType::BorderBottomRight: return HTBOTTOMRIGHT;
                            case base::input::AreaType::BorderTop: return HTTOP;
                            case base::input::AreaType::BorderTopLeft: return HTTOPLEFT;
                            case base::input::AreaType::BorderTopRight: return HTTOPRIGHT;
                            case base::input::AreaType::BorderLeft: return HTLEFT;
                            case base::input::AreaType::BorderRight: return HTRIGHT;
                            case base::input::AreaType::Caption: return HTCAPTION;
                            case base::input::AreaType::Close: return HTCLOSE;
                            case base::input::AreaType::SizeBox: return HTSIZE;
                            case base::input::AreaType::Help: return HTHELP;
                            case base::input::AreaType::HorizontalScroll: return HTHSCROLL;
                            case base::input::AreaType::VerticalScroll: return HTVSCROLL;
                            case base::input::AreaType::Menu: return HTMENU;
                            case base::input::AreaType::Minimize: return HTMINBUTTON;
                            case base::input::AreaType::Maximize: return HTMAXBUTTON;
                            case base::input::AreaType::SysMenu: return HTSYSMENU;
                            default: ASSERT(!"Invalid window area");
                        }
                    }
                }

                break;
            }

            case WM_NCACTIVATE:
            {
                if (!m_hasSystemBorder)
                {
                    // DefWindowProc won't repaint the window border if lParam (normally a HRGN) is -1. This is recommended in:
                    // https://blogs.msdn.microsoft.com/wpfsdk/2008/09/08/custom-window-chrome-in-wpf/
                    return DefWindowProcW(m_hWnd, uMsg, wParam, -1);
                }
                break;
            }

            case WM_NCUAHDRAWCAPTION:
            case WM_NCUAHDRAWFRAME:
                /* These undocumented messages are sent to draw themed window borders.
                    Block them to prevent drawing borders over the client area. */
                return 0;

            case WM_NCCALCSIZE:
                if (!m_hasSystemBorder)
                {
                    auto* rect = (RECT*)lParam;

                    /* DefWindowProc must be called in both the maximized and non-maximized
                        cases, otherwise tile/cascade windows won't work */
                    RECT nonclient = *rect;
                    DefWindowProcW(m_hWnd, WM_NCCALCSIZE, wParam, lParam);
                    RECT client = *rect;

                    if (windowIsMaximized()) {
                        WINDOWINFO wi;
                        memzero(&wi, sizeof(wi));
                        wi.cbSize = sizeof(wi);
                        GetWindowInfo(m_hWnd, &wi);

                        /* Maximized windows always have a non-client border that hangs over
                            the edge of the screen, so the size proposed by WM_NCCALCSIZE is
                            fine. Just adjust the top border to remove the window title. */
                        *rect = {
                            client.left,
                            (int)(nonclient.top + wi.cyWindowBorders),
                            client.right,
                            client.bottom,
                        };

                        HMONITOR mon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
                        MONITORINFO mi;
                        mi.cbSize = sizeof(mi);
                        memzero(&mi, sizeof(mi));
                        GetMonitorInfoW(mon, &mi);

                        /* If the client rectangle is the same as the monitor's rectangle,
                            the shell assumes that the window has gone fullscreen, so it removes
                            the topmost attribute from any auto-hide appbars, making them
                            inaccessible. To avoid this, reduce the size of the client area by
                            one pixel on a certain edge. The edge is chosen based on which side
                            of the monitor is likely to contain an auto-hide appbar, so the
                            missing client area is covered by it. */
                        if (EqualRect(rect, &mi.rcMonitor)) {
                            if (HasAutohideAppbar(ABE_BOTTOM, mi.rcMonitor))
                                rect->bottom--;
                            else if (HasAutohideAppbar(ABE_LEFT, mi.rcMonitor))
                                rect->left++;
                            else if (HasAutohideAppbar(ABE_TOP, mi.rcMonitor))
                                rect->top++;
                            else if (HasAutohideAppbar(ABE_RIGHT, mi.rcMonitor))
                                rect->right--;
                        }
                    }
                    else {
                        /* For the non-maximized case, set the output RECT to what it was
                            before WM_NCCALCSIZE modified it. This will make the client size the
                            same as the non-client size. */
                        *rect = nonclient;
                    }

                    return 0;
                }
                break;

            case WM_CLOSE:
            {
                if (0 == m_windowCloseRequest.exchange(1))
                {
                    TRACE_INFO("Native window got close request");
                }
                return 0;
            }

            case WM_DESTROY:
            {
                TRACE_INFO("Native window destroyed");
                SetWindowLongPtr(m_hWnd, GWLP_USERDATA, NULL);
                m_hWnd = NULL;
                return 0;
            }

            case WM_WINDOWPOSCHANGED:
            {
                auto* data = (WINDOWPOS*)lParam;
                TRACE_INFO("WM_WINDOWPOSCHANGED: [{},{}] [{},{}]", data->x, data->y, data->cx, data->cy);

                base::Rect rect;
                rect.min.x = data->x;
                rect.min.y = data->y;
                rect.max.x = data->x + data->cx;
                rect.max.y = data->y + data->cy;                    

                if (m_callback)
                    m_callback->onOutputWindowPlacementChanged(m_owner, rect, m_currentPixelScale, m_duringSizeMove);

                return 0;
            }

            case WM_ENTERSIZEMOVE:
            {
                TRACE_INFO("WM_ENTERSIZEMOVE");
                m_duringSizeMove = true;
                break;
            }

            case WM_EXITSIZEMOVE:
            {
                TRACE_INFO("WM_EXITSIZEMOVE");
                m_duringSizeMove = false;
                break;
            }

            case WM_SIZING:
            {
                auto* currentSize = (RECT*)lParam;
                TRACE_INFO("WM_SIZING: [{},{}] [{},{}]", currentSize->left, currentSize->top, currentSize->right - currentSize->left, currentSize->bottom - currentSize->top);
                break;
            }

            case WM_SIZE:
            {
                TRACE_INFO("WM_SIZE: [{},{}]", LOWORD(lParam), HIWORD(lParam));
                break;
            }

            case WM_DPICHANGED:
            {
                auto newDpi = HIWORD(wParam);
                m_currentPixelScale = newDpi / 96.0f;

                auto& srcRect = *(RECT*)lParam;

                base::Rect rect;
                rect.min.x = srcRect.left;
                rect.min.y = srcRect.top;
                rect.max.x = srcRect.right;
                rect.max.y = srcRect.bottom;

                // use the requested position
                SetWindowPos(m_hWnd, NULL, rect.min.x, rect.min.y, rect.width(), rect.height(), SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOZORDER);

                // notify the window about changes placement
                // NOTE: this will probably recompute size of the content
                if (m_callback)
                    m_callback->onOutputWindowPlacementChanged(m_owner, rect, m_currentPixelScale, m_duringSizeMove);
                return 0;
            }
        }

        // input
        if (m_inputContext)
        {
            base::input::NativeEventWinApi msg;
            msg.m_window = (uint64_t)m_hWnd;
            msg.m_message = (uint32_t)uMsg;
            msg.m_wParam = (uint64_t)wParam;
            msg.m_lParam = (uint64_t)lParam;
            msg.processed = false;
            msg.returnValue = 0;
            m_inputContext->processMessage(&msg);

            if (msg.processed)
                return msg.returnValue;
        }

        // default case
        return DefWindowProcW(m_hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK WindowWinApi::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        auto windowPtr  = (WindowWinApi*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (windowPtr && windowPtr->m_hWnd == hWnd)
            return windowPtr->windowProc(uMsg, wParam, lParam);
        else
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    //--

    void WindowWinApi::windowAbandon()
    {
        m_callback = nullptr;
        SetWindowLongPtr(m_hWnd, GWLP_USERDATA, 0);
    }

    void WindowWinApi::windowMinimize()
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
            ShowWindow(m_hWnd, SW_MINIMIZE);
    }

    void WindowWinApi::windowMaximize()
    {
       DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
            ShowWindow(m_hWnd, SW_MAXIMIZE);
    }

    void WindowWinApi::windowRestore()
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
            ShowWindow(m_hWnd, SW_RESTORE);
    }

    void WindowWinApi::windowActivate()
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            SetActiveWindow(m_hWnd);
            SetForegroundWindow(m_hWnd);
        }
    }

    void WindowWinApi::windowShow(bool bringToFront)
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            if (m_initialShowDone)
            {
                ::ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);// SW_SHOW);
            }
            else
            {
                if (m_initialShowMaximize)
                    ShowWindow(m_hWnd, SW_MAXIMIZE);
                else if (m_initialShowMinimize)
                    ShowWindow(m_hWnd, SW_MINIMIZE);
                else if (m_initialShowActivate)
                    ShowWindow(m_hWnd, SW_SHOW);
                else
                    ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);

                m_initialShowDone = true;
            }
        }
    }

    void WindowWinApi::windowHide()
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
            ::ShowWindow(m_hWnd, SW_HIDE);
    }

    uint64_t WindowWinApi::windowGetNativeHandle() const
    {
        return (uint64_t)m_hWnd;
    }

    uint64_t WindowWinApi::windowGetNativeDisplay() const
    {
        return 0; // X11 stuff, not used on WinApi
    }

    float WindowWinApi::windowGetPixelScale() const
    {
        return m_currentPixelScale;
    }

    base::Point WindowWinApi::windowGetClientPlacement() const
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            POINT point;
            point.x = 0;
            point.y = 0;
            ClientToScreen(m_hWnd, &point);
            return base::Point(point.x, point.y);
        }
        else
        {
            return base::Point(0, 0);
        }
    }

    base::Point WindowWinApi::windowGetClientSize() const
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            RECT rect;
            GetClientRect(m_hWnd, &rect);
            return base::Point(rect.right - rect.left, rect.bottom - rect.top);
        }
        else
        {
            return base::Point(0, 0);
        }
    }

    base::Point WindowWinApi::windowGetWindowPlacement() const
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            RECT rect;
            GetWindowRect(m_hWnd, &rect);
            return base::Point(rect.left, rect.top);
        }
        else
        {
            return base::Point(0, 0);
        }
    }

    base::Point WindowWinApi::windowGetWindowSize() const
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            RECT rect;
            GetWindowRect(m_hWnd, &rect);
            return base::Point(rect.right - rect.left, rect.bottom - rect.top);
        }
        else
        {
            return base::Point(0, 0);
        }
    }

    bool WindowWinApi::windowHasCloseRequest() const
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        return m_windowCloseRequest.load();
    }

    bool WindowWinApi::windowIsActive() const
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        return m_currentActiveFlag;
    }

    bool WindowWinApi::windowIsVisible() const
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
            return IsWindowVisible(m_hWnd) && !IsIconic(m_hWnd);
        else
            return false;
    }

    bool WindowWinApi::windowIsMaximized() const
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            WINDOWPLACEMENT placement = {};
            placement.length = sizeof(placement);
            GetWindowPlacement(m_hWnd, &placement);
            return (placement.showCmd == SW_MAXIMIZE);
        }
        else
        {
            return false;
        }
    }

    bool WindowWinApi::windowIsMinimized() const
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            return !!IsIconic(m_hWnd);
        }
        else
        {
            return false;
        }
    }

    void WindowWinApi::updateTitle_NoLock()
    {
        m_fullTitleString = base::TempString("{} (FPS: {})", m_currentTitle, m_numLastFramesRendered);
        SetWindowTextA(m_hWnd, m_fullTitleString.c_str());
    }

    void WindowWinApi::windowSetTitle(const base::StringBuf& title)
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            auto lock = base::CreateLock(m_windowLock);
            if (m_currentTitle != title)
            {
                m_currentTitle = title;
                updateTitle_NoLock();
            }
        }
    }

    void WindowWinApi::windowAdjustClientPlacement(const base::Rect& clientRect)
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            RECT innerRect;
            innerRect.left = clientRect.left();
            innerRect.top = clientRect.top();
            innerRect.right = clientRect.right();
            innerRect.bottom = clientRect.bottom();

            DWORD dwStyle = GetWindowLong(m_hWnd, GWL_STYLE);
            DWORD dwExStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);

            ::AdjustWindowRectEx(&innerRect, dwStyle, FALSE, dwExStyle);

            SetWindowPos(m_hWnd, NULL, innerRect.left, innerRect.top, innerRect.right - innerRect.left, innerRect.bottom - innerRect.top, SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }

    void WindowWinApi::windowAdjustWindowPlacement(const base::Rect& windowRect)
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            RECT innerRect;
            innerRect.left = windowRect.left();
            innerRect.top = windowRect.top();
            innerRect.right = windowRect.right();
            innerRect.bottom = windowRect.bottom();
                
            SetWindowPos(m_hWnd, NULL, innerRect.left, innerRect.top, innerRect.right - innerRect.left, innerRect.bottom - innerRect.top, SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }
        
    void WindowWinApi::windowSetAlpha(float alpha)
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be touched on main thread");
        if (Fibers::GetInstance().isMainThread())
        {
            auto alphaByte = std::clamp<int>((int)(alpha * 255.0f), 0, 255);
            ::SetLayeredWindowAttributes(m_hWnd, 0, (uint8_t)alphaByte, LWA_ALPHA);
        }
    }

    void WindowWinApi::windowCancelCloseRequest()
    {
        if (1 == m_windowCloseRequest.exchange(0))
        {
            TRACE_INFO("Close request canceled for window '{}'", m_currentTitle);
        }
    }

    base::input::ContextPtr WindowWinApi::windowGetInputContext()
    {
        return m_inputContext;
    }

    //--

    bool WindowManagerWinApi::initialize(bool apiNeedsWindowForOutput)
    {
        // cache display modes and monitor list
        enumerateMonitors();
        cacheDisplayModes();

        // setup the DPI awareness
#if (WINVER >= 0x0605)
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif

        // create a dummy class
        static bool FakeWindowClassInitialized = false;
        if (!FakeWindowClassInitialized)
        {
            WNDCLASSEXW cls;
            memzero(&cls, sizeof(cls));
            cls.cbSize = sizeof(WNDCLASSEXW);
            cls.style = CS_OWNDC;
            cls.lpfnWndProc = &DummyWindowProc;
            cls.cbClsExtra = 0;
            cls.cbWndExtra = sizeof(void*);
            cls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            cls.hCursor = LoadCursor(NULL, IDC_ARROW);
            cls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            cls.lpszMenuName = NULL;
            cls.lpszClassName = L"BoomerEngineDummyWindowClass";
            cls.hInstance = GetModuleHandle(NULL);
            cls.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

            // Register the application
            if (!RegisterClassExW(&cls))
            {
                TRACE_ERROR("Failed to register dummy window class");
                return false;
            }
        }

        // create small invisible window
        if (apiNeedsWindowForOutput)
        {
            m_hFakeWnd = CreateWindowW(L"BoomerEngineDummyWindowClass", L"BoomerDummyWindow", WS_POPUP, 0, 0, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
            if (NULL == m_hFakeWnd)
            {
                UnregisterClassA("BoomerEngineDummyWindowClass", GetModuleHandle(NULL));
                TRACE_ERROR("Failed to create a dummy window class");
                return false;
            }
        }

        return true;
    }

    WindowManagerWinApi::WindowManagerWinApi()
    {}

    WindowManagerWinApi::~WindowManagerWinApi()
    {
        // NOTE: any rendering must be FINISHED by this point (ie. GPU stopped, all swapchains released)

        // close all normal windows
        m_windows.clearPtr();

        // close fake window
        if (NULL != m_hFakeWnd)
        {
            TRACE_INFO("Destroying dummy window");
            DestroyWindow(m_hFakeWnd);
            m_hFakeWnd = NULL;
        }
    }

    uint64_t WindowManagerWinApi::offscreenWindow()
    {
        return (uint64_t)m_hFakeWnd;
    }

    void WindowManagerWinApi::updateWindows()
    {
        DEBUG_CHECK_EX(!m_duringUpdate, "Recursive update");
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be updated from main thread");

        base::InplaceArray<HWND, 10> windowHandles;

        {
            if (m_hFakeWnd)
                windowHandles.pushBack(m_hFakeWnd);

            auto lock = CreateLock(m_windowsLock);
            for (auto* window : m_windows)
            {
                window->update();
                windowHandles.pushBack(window->handle());
            }
        }

        // setup the DPI awareness
#if (WINVER >= 0x0605)
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif
        // process messages on all active windows
        m_duringUpdate = true;
        for (HWND hWnd : windowHandles)
            MessageLoop(hWnd);
        m_duringUpdate = false;
    }

    uint64_t WindowManagerWinApi::createWindow(ObjectID owner, const DriverOutputInitInfo& initInfo)
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be created from main thread");
        DEBUG_CHECK_EX(!m_duringUpdate, "Closing window from within a message loop");
        DEBUG_CHECK_EX(owner, "Windows requires an owner");

        if (owner && Fibers::GetInstance().isMainThread() && !m_duringUpdate)
        {
            auto wnd = WindowWinApi::Create(owner, initInfo);
            if (wnd)
            {
                auto lock = CreateLock(m_windowsLock);
                m_windows.pushBack(wnd);
                return (uint64_t)wnd->handle();
            }
        }

        return 0;
    }

    void WindowManagerWinApi::closeWindow(uint64_t handle)
    {
        DEBUG_CHECK_EX(Fibers::GetInstance().isMainThread(), "Windows can only be closed from main thread");
        DEBUG_CHECK_EX(!m_duringUpdate, "Closing window from within a message loop");
        if (Fibers::GetInstance().isMainThread() && !m_duringUpdate)
        {
            auto lock = CreateLock(m_windowsLock);

            for (auto wnd : m_windows)
            {
                if (wnd->handle() == (HWND)handle)
                {
                    m_windows.remove(wnd);
                    MemDelete(wnd);
                }
            }
        }
    }

    bool WindowManagerWinApi::prepareWindowForRendering(uint64_t handle, uint16_t& outWidth, uint16_t& outHeight)
    {
        auto lock = CreateLock(m_windowsLock);

        for (auto wnd : m_windows)
            if (wnd->handle() == (HWND)handle)
                return wnd->prepareWindowForRendering(outWidth, outHeight);

        return false;
    }

    void WindowManagerWinApi::disconnectWindow(uint64_t handle)
    {
        auto lock = CreateLock(m_windowsLock);

        for (auto wnd : m_windows)
            if (wnd->handle() == (HWND)handle)
                return wnd->disconnectWindow();
    }

    void WindowManagerWinApi::finishWindowRendering(uint64_t handle)
    {
        auto lock = CreateLock(m_windowsLock);

        for (auto wnd : m_windows)
            if (wnd->handle() == (HWND)handle)
                return wnd->releaseWindowFromRendering();
    }

    IDriverNativeWindowInterface* WindowManagerWinApi::windowInterface(uint64_t handle)
    {
        auto lock = CreateLock(m_windowsLock);

        for (auto wnd : m_windows)
            if (wnd->handle() == (HWND)handle)
                return wnd; // TODO: this is unsafe!

        return nullptr;
    }

    //--

    void WindowManagerWinApi::cacheDisplayModes()
    {
        uint32_t deviceIndex = 0;
        for (;;)
        {
            DISPLAY_DEVICEA info;
            memzero(&info, sizeof(info));
            info.cb = sizeof(info);

            if (!EnumDisplayDevicesA(NULL, deviceIndex, &info, 0))
                break;

            CachedDisplay displayInfo;
            displayInfo.m_displayInfo.name = info.DeviceString;
            displayInfo.m_displayInfo.primary = (info.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;
            displayInfo.m_displayInfo.attached = (info.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0;
            displayInfo.m_displayInfo.active = (info.StateFlags & DISPLAY_DEVICE_ACTIVE) != 0;
            displayInfo.m_displayInfo.desktopHeight = 0;
            displayInfo.m_displayInfo.desktopWidth = 0;

            uint32_t modeIndex = 0;
            for (;;)
            {
                DEVMODEA mode;
                memzero(&mode, sizeof(mode));
                mode.dmSize = sizeof(DEVMODE);

                if (!EnumDisplaySettingsA(info.DeviceName, modeIndex, &mode))
                    break;

                CachedDisplayMode resolutionInfo;
                resolutionInfo.m_width = (uint16_t)mode.dmPelsWidth;
                resolutionInfo.m_height = (uint16_t)mode.dmPelsHeight;
                resolutionInfo.m_refreshRate = (uint16_t)mode.dmDisplayFrequency;
                displayInfo.m_displayModes.pushBack(resolutionInfo);

                modeIndex += 1;
            }

            std::sort(displayInfo.m_displayModes.begin(), displayInfo.m_displayModes.end(), [](const CachedDisplayMode& a, CachedDisplayMode& b)
                {
                    if (a.m_height != a.m_height)
                        return a.m_height < b.m_height;

                    if (a.m_width != b.m_width)
                        return a.m_width < b.m_width;

                    return a.m_refreshRate < b.m_refreshRate;
                });

            m_cachedDisplayInfos.pushBack(displayInfo);
            deviceIndex += 1;
        }
    }

    static BOOL WINAPI TheEnumFunction(HMONITOR hMonitor, HDC hDC, LPRECT pRect, LPARAM lParam)
    {
        auto outList = (base::Array<base::Rect>*)lParam;

        MONITORINFO info = {};
        info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfoA(hMonitor, &info);

        // TODO: for some applications we may need the whole monitor area
        base::Rect monitorRect(info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom);
        if (info.dwFlags & MONITORINFOF_PRIMARY)
            outList->insert(0, monitorRect);
        else
            outList->pushBack(monitorRect);

        return TRUE;
    }


    void WindowManagerWinApi::enumerateMonitors()
    {
        m_monitorRects.clear();
        EnumDisplayMonitors(NULL, NULL, &TheEnumFunction, (LPARAM)&m_monitorRects);
        ASSERT_EX(!m_monitorRects.empty(), "No monitors enumerated");

        TRACE_INFO("Found {} display monitors", m_monitorRects.size());

        for (auto& rect : m_monitorRects)
            TRACE_INFO("Monitor [{},{}] - [{},{}] (size: {},{})", rect.min.x, rect.min.y, rect.max.x, rect.max.y, rect.width(), rect.height());
    }

    //--

    void WindowManagerWinApi::enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const
    {
        outMonitorAreas = m_monitorRects;
    }

    void WindowManagerWinApi::enumDisplays(base::Array<DriverDisplayInfo>& outDisplayInfos) const
    {
        for (auto& info : m_cachedDisplayInfos)
            outDisplayInfos.pushBack(info.m_displayInfo);
    }

    void WindowManagerWinApi::enumResolutions(uint32_t displayIndex, base::Array<DriverResolutionInfo>& outResolutions) const
    {
        if (displayIndex < m_cachedDisplayInfos.size())
        {
            for (auto& mode : m_cachedDisplayInfos[displayIndex].m_displayModes)
            {
                DriverResolutionInfo resInfo;
                resInfo.width = mode.m_width;
                resInfo.height = mode.m_height;
                outResolutions.pushBackUnique(resInfo);
            }
        }
    }

    void WindowManagerWinApi::enumVSyncModes(uint32_t displayIndex, base::Array<DriverResolutionSyncInfo>& outVSyncModes) const
    {
        DriverResolutionSyncInfo info;
        info.value = 0;
        info.name = "No VSync";
        outVSyncModes.pushBack(info);

        info.value = 1;
        info.name = "VSync";
        outVSyncModes.pushBack(info);
    }

    void WindowManagerWinApi::enumRefreshRates(uint32_t displayIndex, const DriverResolutionInfo& info, base::Array<int>& outRefreshRates) const
    {
        if (displayIndex < m_cachedDisplayInfos.size())
        {
            for (auto& mode : m_cachedDisplayInfos[displayIndex].m_displayModes)
            {
                if (mode.m_width == info.width && mode.m_height == info.height)
                {
                    outRefreshRates.pushBackUnique(mode.m_refreshRate);
                }
            }
        }
    }

    LRESULT CALLBACK WindowManagerWinApi::DummyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    //--

    WindowManager* WindowManager::Create(bool apiNeedsWindowForOutput)
    {
        auto ret = MemNew(WindowManagerWinApi);
        if (ret->initialize(apiNeedsWindowForOutput))
            return ret;
        MemDelete(ret.ptr);
        return nullptr;
    }

} // device

