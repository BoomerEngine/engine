/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\windows #]
* [# platform: winapi #]
***/

#include "build.h"
#include "winErrorWindow.h"

#include "../res/resources.h"
#include "core/containers/include/stringBuilder.h"

extern void* GCurrentModuleHandle;

BEGIN_BOOMER_NAMESPACE()

namespace win
{

    ErrorHandlerDlg::ErrorHandlerDlg()
        : m_canContinue(false)
        , m_canDisable(false)
    {}

    ErrorHandlerDlg::~ErrorHandlerDlg()
    {}

    void ErrorHandlerDlg::message(const char* txt)
    {
        ConvertRF(txt, m_message);
    }

    void ErrorHandlerDlg::assertMessage(const char* file, uint32_t line, const char* expr, const char* msg)
    {
        if (file && *file)
            m_message.appendf("Assertion failed at {}[{}]\r\n", file, line);

        m_message.appendf("Asserted expression: {}\r\n", expr);

        if (msg && *msg)
            m_message.appendf("Additional information: {}\r\n", msg);
    }

    void ErrorHandlerDlg::callstack(const char   * txt)
    {
        ConvertRF(txt, m_callstack);
    }

    void ErrorHandlerDlg::callstack(const debug::Callstack& callstack)
    {
		callstack.print(m_callstack, "\r\n");
    }

    ErrorHandlerDlg::EResult ErrorHandlerDlg::showDialog()
    {
        HINSTANCE hModule = (HINSTANCE)GCurrentModuleHandle;
        auto retVal = DialogBoxParam(hModule, MAKEINTRESOURCE(IDD_ASSERT), NULL, &DlgProc, reinterpret_cast<LPARAM>(this));

        // Not shown - missing dialog
        if (retVal == -1)
        {
            auto errCode = GetLastError();
            TRACE_ERROR("Failed to display assert dialog, error code ={}", errCode);
            return EResult::Break;
        }

        return (EResult)retVal;
    }

    INT_PTR ErrorHandlerDlg::DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_INITDIALOG:
            {
                ErrorHandlerDlg* params = reinterpret_cast<ErrorHandlerDlg*>(lParam);

                HWND hExpression = GetDlgItem(hwndDlg, IDC_ERROR_TEXT);
                SetWindowTextA(hExpression, params->m_message.c_str());

                HWND hCallstack = GetDlgItem(hwndDlg, IDC_ERROR_CALLSTACK);
                SetWindowTextA(hCallstack, params->m_callstack.c_str());

                if (!params->m_canContinue)
                {
                    HWND hBreakBtn = GetDlgItem(hwndDlg, IDC_IGNORE);
                    EnableWindow(hBreakBtn, FALSE);
                }

                if (!params->m_canContinue || !params->m_canDisable)
                {
                    HWND hBreakBtn = GetDlgItem(hwndDlg, IDC_DISABLE);
                    EnableWindow(hBreakBtn, FALSE);
                }

                ShowWindow(hwndDlg, SW_SHOW);
                SetForegroundWindow(hwndDlg);
                return TRUE;
            }

            case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDC_IGNORE: EndDialog(hwndDlg, (int)EResult::Ignore); return TRUE;
                    case IDC_DISABLE: EndDialog(hwndDlg, (int)EResult::Disable); return TRUE;
                    case IDC_BREAK: EndDialog(hwndDlg, (int)EResult::Break); return TRUE;
                    case IDC_EXIT: EndDialog(hwndDlg, (int)EResult::Exit); return TRUE;
                }
            }
        }

        return FALSE;
    }

    void ErrorHandlerDlg::ConvertRF(const char *read, StringBuilder& str)
    {
        while (*read)
        {
            auto ch = *read++;
			if (ch == '\n')
			{
				str.append("\xd\xa");
			}
			else
			{
				char temp[2] = { ch, 0 };
				str.append(temp);
			}
        }
    }
            
} // win

END_BOOMER_NAMESPACE()
