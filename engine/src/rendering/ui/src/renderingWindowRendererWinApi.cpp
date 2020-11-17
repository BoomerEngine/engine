/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: ui #]
* [#platform: windows #]
***/

#include "build.h"
#include "renderingWindowRenderer.h"

#include <Windows.h>

namespace rendering
{
    //--

    ui::Position NativeWindowRenderer::mousePosition() const
    {
        POINT p;
        ::GetCursorPos(&p);
        return base::Vector2(p.x, p.y);
    }

    bool NativeWindowRenderer::testPosition(const ui::Position& pos) const
    {
        return false;
    }

    bool NativeWindowRenderer::testArea(const ui::ElementArea& area) const
    {
        return false;
    }

    void NativeWindowRenderer::playSound(ui::MessageType type)
    {
        switch (type)
        {
        case ui::MessageType::Error: 
            ::MessageBeep(MB_ICONHAND);
            break;
        case ui::MessageType::Warning:
            ::MessageBeep(MB_ICONEXCLAMATION);
            break;
        case ui::MessageType::Info:
            ::MessageBeep(MB_ICONASTERISK);
            break;
        case ui::MessageType::Question:
            ::MessageBeep(MB_ICONQUESTION);
            break;
        }

    }

    ui::NativeWindowID NativeWindowRenderer::windowAtPos(const ui::Position& absoluteWindowPosition)
    {
        POINT p;
        p.x = (int)absoluteWindowPosition.x;
        p.y = (int)absoluteWindowPosition.y;

        HWND hWnd = ::WindowFromPoint(p);
        for (auto* window : m_nativeWindowMap.values())
            if (window->window->windowGetNativeHandle() == (uint64_t)hWnd)
                return window->id;

        return 0;
    }

    static DWORD GetClipboardFormatID(base::StringView format)
    {
        if (format.empty())
            return 0;

        if (format == "UNITEXT")
            return CF_UNICODETEXT;

        if (format == "TEXT")
            return CF_TEXT;

        return RegisterClipboardFormatA(base::TempString("BoomerEngine:{}", format));
    }

    static HGLOBAL CopyBufferToGlobalMemory(const void* data, uint32_t size)
    {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hMem == NULL)
        {
            TRACE_ERROR("Failed to allocate global memory: {}", GetLastError());
            return NULL;
        }

        void* ptr = GlobalLock(hMem);
        if (!ptr)
        {
            TRACE_ERROR("Failed to lock global memory: {}", GetLastError());
            GlobalFree(hMem);
            return NULL;
        }

        memcpy(ptr, data, size);
        GlobalUnlock(ptr);

        return hMem;
    }

    bool NativeWindowRenderer::checkClipboardHasData(base::StringView format)
    {
        const auto formatId = GetClipboardFormatID(format);
        if (!formatId)
        {
            TRACE_ERROR("Invalid clipboard data format '{}'", format);
            return false;
        }

        return ::IsClipboardFormatAvailable(formatId);
    }

    bool NativeWindowRenderer::stroreClipboardData(base::StringView format, const void* data, uint32_t size)
    {
        const auto formatId = GetClipboardFormatID(format);
        if (!formatId)
        {
            TRACE_ERROR("Invalid clipboard data format '{}'", format);
            return false;
        }

        auto hMem = CopyBufferToGlobalMemory(data, size);
        if (!hMem)
            return false;

        bool saved = false;
        if (OpenClipboard(NULL))
        {
            if (SetClipboardData(formatId, hMem))
            {
                TRACE_INFO("Stored {} in clipboard, format '{}'", MemSize(size), format);
                saved = true;
            }
            else
            {
                TRACE_ERROR("Failed to store data in clipboard, format '{}'", format);
                GlobalFree(hMem);
            }

            CloseClipboard();
        }
        else
        {
            TRACE_ERROR("Unable to open clipboard");
            GlobalFree(hMem);
        }

        return saved;
    }

    base::Buffer NativeWindowRenderer::loadClipboardData(base::StringView format)
    {
        const auto formatId = GetClipboardFormatID(format);
        if (!formatId)
        {
            TRACE_ERROR("Invalid clipboard data format '{}'", format);
            return nullptr;
        }

        if (!IsClipboardFormatAvailable(formatId))
            return nullptr;

        if (!OpenClipboard(NULL))
        {
            TRACE_ERROR("Unable to open clipboard");
            return nullptr;
        }

        base::Buffer ret;
        if (HGLOBAL hMem = GetClipboardData(formatId))
        {
            const auto size = GlobalSize(hMem);
            if (size)
            {
                if (ret.initWithZeros(POOL_TEMP, size, 16))
                {
                    if (void* ptr = GlobalLock(hMem))
                    {
                        memcpy(ret.data(), ptr, size);
                        GlobalUnlock(hMem);
                        TRACE_INFO("Loaded {} from clipboard, format '{}'", MemSize(size), format);
                    }
                    else
                    {
                        TRACE_ERROR("Unable to accecss clipboard data of size '{}'", size);
                        ret.reset();
                    }
                }
                else
                {
                    TRACE_ERROR("Unable to initialize buffer for clipboard data of size '{}'", size);
                }
            }
            else
            {
                TRACE_ERROR("Unable to determine size of clipboard data for format '{}'", format);
            }
        }
        else
        {
            TRACE_ERROR("Unable to get clipboard data for format '{}'", format);
        }

        CloseClipboard();
        return ret;
    }

    //--

} // rendering