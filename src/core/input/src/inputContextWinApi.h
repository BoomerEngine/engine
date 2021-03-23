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

BEGIN_BOOMER_NAMESPACE()

class InputContextWinApi;

///---

/// RawInput keyboard
class RawKeyboard : public GenericKeyboard
{
public:
    RawKeyboard(IInputContext* context);
    ~RawKeyboard();

    void interpretChar(HWND hWnd, WPARAM charCode);
    void interpretRawInput(HWND hWnd, const RID_DEVICE_INFO* deviceInfo, const RAWINPUT* inputData);
    void interpretKeyDown(HWND hWnd, WPARAM keyCode, LPARAM flags);
    void interpretKeyUp(HWND hWnd, WPARAM keyCode, LPARAM flags);

    InputKey mapKeyCode(USHORT virtualKey, bool extendedKey) const;

    InputKey m_windowsKeyMapping[256];
};

///---

/// RawInput mouse implementation
class RawMouse : public GenericMouse
{
public:
    RawMouse(InputContextWinApi* context, RawKeyboard* keyboard);
    ~RawMouse();

    void interpretRawInput(HWND hWnd, const RID_DEVICE_INFO* deviceInfo, const RAWINPUT* inputData, bool& outGotClick);

private:
    static Point GetMousePositionInWindow(HWND hWnd);
    static Point GetMousePositionAbsolute();
    static Rect GetWindowClientRect(HWND hWnd);
    static bool IsCaptureWindow(HWND hWnd);

    bool m_isCaptured;
    InputContextWinApi* m_context;
};

///---

/// WinAPI input context
class CORE_INPUT_API InputContextWinApi : public IInputContext
{
    RTTI_DECLARE_VIRTUAL_CLASS(InputContextWinApi, IInputContext);

public:
    InputContextWinApi(uint64_t nativeWindow, uint64_t nativeDisplay, bool gameMode);

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

    bool m_captureWaitingForClick = false;

    RawKeyboard m_keyboard;
    RawMouse m_mouse;
    bool m_useRawInput;

    void releaseCapture();

    uint64_t windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& processed);
    void processRawInput(HWND hWnd, HRAWINPUT hRawInput);
};

//--

END_BOOMER_NAMESPACE()
