/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\windows #]
* [# platform: winapi #]
***/

#pragma once

#include <Windows.h>

#include "core/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE()

namespace win
{

    // Error handler window
    class ErrorHandlerDlg
    {
    public:
        bool m_canContinue;
        bool m_canDisable;

        ErrorHandlerDlg();
        ~ErrorHandlerDlg();

        void message(const char* txt);
        void assertMessage(const char* file, uint32_t line, const char* expr, const char* msg);

        void callstack(const char* txt);
        void callstack(const debug::Callstack& callstack);

        enum class EResult : uint8_t
        {
            Ignore,
            Disable,
            Break,
            Exit,
        };

        EResult showDialog();

    private:
        StringBuilder   m_message;
        StringBuilder   m_callstack;

        static INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static void ConvertRF(const char *read, StringBuilder& str);
    };

} // win

END_BOOMER_NAMESPACE()
