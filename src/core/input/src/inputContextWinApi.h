/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once

#include "inputContext.h"
#include "inputGenericKeyboard.h"
#include "inputGenericMouse.h"

#include <Windows.h>

BEGIN_BOOMER_NAMESPACE_EX(input)

class ContextWinApi;

///---

/// RawInput keyboard
class RawKeyboard : public GenericKeyboard
{
public:
    RawKeyboard(IContext* context);
    ~RawKeyboard();

    void interpretChar(HWND hWnd, WPARAM charCode);
    void interpretRawInput(HWND hWnd, const RID_DEVICE_INFO* deviceInfo, const RAWINPUT* inputData);
    void interpretKeyDown(HWND hWnd, WPARAM keyCode, LPARAM flags);
    void interpretKeyUp(HWND hWnd, WPARAM keyCode, LPARAM flags);

    KeyCode mapKeyCode(USHORT virtualKey, bool extendedKey) const;

    KeyCode m_windowsKeyMapping[256];
};

///---

/// RawInput mouse implementation
class RawMouse : public GenericMouse
{
public:
    RawMouse(ContextWinApi* context, RawKeyboard* keyboard);
    ~RawMouse();

    void interpretRawInput(HWND hWnd, const RID_DEVICE_INFO* deviceInfo, const RAWINPUT* inputData);

private:
    static Point GetMousePositionInWindow(HWND hWnd);
    static Point GetMousePositionAbsolute();
    static Rect GetWindowClientRect(HWND hWnd);
    static bool IsCaptureWindow(HWND hWnd);

    bool m_isCaptured;
    ContextWinApi* m_context;
};

///---

/// WinAPI input context
class CORE_INPUT_API ContextWinApi : public IContext
{
    RTTI_DECLARE_VIRTUAL_CLASS(ContextWinApi, IContext);

public:
    ContextWinApi(uint64_t nativeWindow, uint64_t nativeDisplay, bool gameMode);

    INLINE HWND hWND() const { return m_hWnd; }

    //--

    virtual void resetInput() override final;
    virtual void processState() override final;
    virtual void processMessage(const void* msg) override final;
    virtual void requestCapture(int captureMode) override final;

protected:
    HWND m_hWnd;

    POINT m_activeCaptureInitialMousePos = { 0,0 };
    POINT m_activeCaptureLastMousePos = { 0,0 };
    int m_activeCaptureMode = 0;

    RawKeyboard m_keyboard;
    RawMouse m_mouse;
    bool m_useRawInput;

    void releaseCapture();

    uint64_t windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& processed);
    void processRawInput(HWND hWnd, HRAWINPUT hRawInput);
};

//--

END_BOOMER_NAMESPACE_EX(input)
