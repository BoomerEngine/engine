/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: ui #]
***/

#include "build.h"
#include "renderingWindowRenderer.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"
#include "base/input/include/inputContext.h"
#include "base/system/include/scopeLock.h"
#include "base/system/include/thread.h"

#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingObject.h"
#include "rendering/driver/include/renderingShaderLibrary.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/driver/include/renderingOutput.h"
#include "rendering/canvas/include/renderingCanvasRenderingService.h"

namespace rendering
{
    //--

    NativeWindowRenderer::NativeWindowRenderer()
    {}

    NativeWindowRenderer::~NativeWindowRenderer()
    {
        for (auto* value : m_nativeWindowMap.values())
            base::GetService<DeviceService>()->device()->releaseObject(value->output);

        m_nativeWindowMap.clearPtr();
    }

    ui::ElementArea NativeWindowRenderer::adjustArea(const ui::ElementArea& area, ui::WindowInitialPlacementMode placement /*= ui::WindowInitialPlacementMode::ScreenCenter*/) const
    {
        return ui::ElementArea();
    }

    ui::ElementArea NativeWindowRenderer::windowMonitorAtPos(const ui::Position& absoluteWindowPosition)
    {
        base::InplaceArray<base::Rect, 8> monitorAreas;
        base::GetService<DeviceService>()->device()->enumMonitorAreas(monitorAreas);

        base::Point testPoint((int)absoluteWindowPosition.x, (int)absoluteWindowPosition.y);
        for (const auto& area : monitorAreas)
        {
            if (area.contains(testPoint))
                return area;
        }

        auto mousePos = base::Point(mousePosition());
        for (const auto& area : monitorAreas)
        {
            if (area.contains(mousePos))
                return area;
        }

        return monitorAreas[0];
    }

    ui::NativeWindowID NativeWindowRenderer::windowCreate(const ui::NativeWindowSetup& setup)
    {
        auto* device = base::GetService<DeviceService>()->device();
        if (!device)
            return 0;

        DriverOutputInitInfo initInfo;
        initInfo.m_class = DriverOutputClass::NativeWindow;
        initInfo.m_width = setup.size.x;
        initInfo.m_height = setup.size.y;
        initInfo.m_windowActivate = setup.activate;
        initInfo.m_windowTopmost = setup.topmost;
        initInfo.m_windowTitle = setup.title;
        initInfo.m_windowSystemBorder = false; // we never use sytem border for UI windows
        initInfo.m_windowPopup = setup.popup;
        initInfo.m_windowAdjustArea = false;
        initInfo.m_windowCanResize = true;
        initInfo.m_windowMaximized = setup.maximized;
        initInfo.m_windowMinimized = setup.minimized;
        initInfo.m_windowCreateInputContext = true;
        initInfo.m_windowShowOnTaskBar = !setup.popup;
        initInfo.m_windowOpacity = 1.0f;
        initInfo.m_windowPlacementX = setup.position.x;
        initInfo.m_windowPlacementY = setup.position.y;
        initInfo.m_windowShow = false;
        initInfo.m_windowInputContextGameMode = false; // don't use RawInput, etc
        initInfo.m_windowCallback = this;

        ui::NativeWindowID ownerWindowId = 0;
        if (auto* ownerWindow = m_nativeWindowMap.findSafe(setup.owner, nullptr))
        {
            ownerWindowId = ownerWindow->id;
            if (ownerWindow->output)
                initInfo.m_windowNativeParent = ownerWindow->window->windowGetNativeHandle();
        }

        auto output = device->createOutput(initInfo);
        if (!output)
            return 0;

        auto wrapper = MemNew(NativeWindow);
        wrapper->id = m_nextNativeWindowId++;
        wrapper->ownerId = ownerWindowId;
        wrapper->output = output;
        wrapper->lastTitle = setup.title;
        wrapper->firstFramePending = true;
        wrapper->callback = setup.callback;
        wrapper->window = device->queryOutputWindow(output);
        m_nativeWindowMap[wrapper->id] = wrapper;

        return wrapper->id;
    }

    void NativeWindowRenderer::windowDestroy(ui::NativeWindowID id)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        {
            base::GetService<DeviceService>()->device()->releaseObject(window->output);
            MemDelete(window);
            m_nativeWindowMap.remove(id);
        }
    }

    base::input::EventPtr NativeWindowRenderer::windowPullInputEvent(ui::NativeWindowID id)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            if (auto inputContext = window->window->windowGetInputContext())
                return inputContext->pull();

        return nullptr;
    }

    void NativeWindowRenderer::windowShow(ui::NativeWindowID id, bool visible)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            if (visible)
                window->window->windowShow(false);
            else
                window->window->windowHide();
    }

    uint64_t NativeWindowRenderer::windowNativeHandle(ui::NativeWindowID id)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            if (window->window)
                return window->window->windowGetNativeHandle();

        return 0;
    }

    void NativeWindowRenderer::windowSetPos(ui::NativeWindowID id, const ui::Position& pos)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        {
            auto size = window->window->windowGetClientSize();

            base::Rect rect;
            rect.min.x = (int)pos.x;
            rect.min.y = (int)pos.y;
            rect.max.x = rect.min.x + size.x;
            rect.max.y = rect.min.y + size.y;
            window->window->windowAdjustClientPlacement(rect);
        }
    }

    void NativeWindowRenderer::windowSetSize(ui::NativeWindowID id, const ui::Size& size)
    {

    }

    float NativeWindowRenderer::windowPixelScale(ui::NativeWindowID id)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            return window->window->windowGetPixelScale();

        return 1.0f;
    }

    void NativeWindowRenderer::windowSetFocus(ui::NativeWindowID id)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            window->window->windowActivate();
    }

    bool NativeWindowRenderer::windowGetFocus(ui::NativeWindowID id)
    {
/*        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            if (auto windowInterface = window->output->outputWindowInterface())
                return window->window->windowIsActive();*/

        for (auto* window : m_nativeWindowMap.values())
        {
            if (window->window->windowIsActive())
            {
                auto curId = window->id;
                while (curId != 0)
                {
                    if (curId == id)
                        return true;

                    {
                        NativeWindow* info = nullptr;
                        m_nativeWindowMap.find(curId, info);
                        curId = info ? info->ownerId : 0;
                    }
                }
            }
        }

        return false;
    }

    bool NativeWindowRenderer::windowGetMinimized(ui::NativeWindowID id)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            return window->window->windowIsMinimized();

        return false;
    }

    bool NativeWindowRenderer::windowGetResizable(ui::NativeWindowID id)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        {
            if (!window->window->windowIsMaximized())
                return true;
        }

        return false;
    }

    bool NativeWindowRenderer::windowGetMovable(ui::NativeWindowID id)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        {
            if (!window->window->windowIsMaximized())
                return true;
        }

        return false;
    }

    bool NativeWindowRenderer::windowGetMaximized(ui::NativeWindowID id)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            return window->window->windowIsMaximized();

        return false;
    }

    void NativeWindowRenderer::windowSetTitle(ui::NativeWindowID id, base::StringView<char> txt)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            window->window->windowSetTitle(base::StringBuf(txt));
    }

    void NativeWindowRenderer::windowSetOpacity(ui::NativeWindowID id, float opacity)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            window->window->windowSetAlpha(opacity);
    }

    bool NativeWindowRenderer::windowHasCloseRequest(ui::NativeWindowID id)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        {
            if (window->window->windowHasCloseRequest())
            {
                window->window->windowCancelCloseRequest(); // report it only once
                return true;
            }
        }

        return false;
    }

    void NativeWindowRenderer::windowUpdate(ui::NativeWindowID id)
    {
        // ?
    }


    void NativeWindowRenderer::windowSetCapture(ui::NativeWindowID id, int mode)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
            if (auto inputContext = window->window->windowGetInputContext())
                inputContext->requestCapture(mode);
    }

    bool NativeWindowRenderer::windowGetRenderableArea(ui::NativeWindowID id, ui::ElementArea& outWindowDrawArea)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        {
            // do not render if window is minimized
            if (window->window->windowIsMinimized())
                return false;

            // do not rendering if window is not visible, unless it's that first frame...
            if (!window->firstFramePending && !window->window->windowIsVisible())
                return false;

            // use the window placement
            const auto windowPlacement = window->window->windowGetClientPlacement();
            const auto windowSize = window->window->windowGetClientSize();
            outWindowDrawArea = ui::ElementArea(windowPlacement.x, windowPlacement.y, windowPlacement.x + windowSize.x, windowPlacement.y + windowSize.y);
            return true;
        }

        return false;
    }

    void NativeWindowRenderer::windowRenderContent(ui::NativeWindowID id, const ui::Size& size, bool forcedPaint, const base::canvas::Canvas& canvas)
    {
        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        {
            // write commands
            rendering::command::CommandWriter cmd(base::TempString("NativeWindowRender {} - '{}'", window->id, window->lastTitle));

            // acquire back buffer
            base::Rect viewport;
            ImageView colorBackBuffer, depthBackBuffer;
            if (cmd.opAcquireOutput(window->output, viewport, colorBackBuffer, &depthBackBuffer))
            {
                FrameBuffer fb;
                fb.depth.view(depthBackBuffer).clearDepth().clearStencil();
                fb.color[0].view(colorBackBuffer);

                // if canvas requests clearing the window do it - this can be used to save initial background rect in the window
                if (canvas.clearColor().a > 0)
                    fb.color[0].clear(canvas.clearColor());

                // set frame buffer
                cmd.opBeingPass(fb);

                // render canvas to command buffer
                canvas::CanvasRenderingParams renderingParams;
                renderingParams.frameBufferWidth = colorBackBuffer.width();
                renderingParams.frameBufferHeight = colorBackBuffer.height();
                base::GetService<CanvasService>()->render(cmd, canvas, renderingParams);

                // finish pass
                cmd.opEndPass();

                // swap to show in actual window
                cmd.opSwapOutput(window->output);
            }

            // submit work for rendering on GPU
            base::GetService<DeviceService>()->device()->submitWork(cmd.release());

            // make sure rendering is finished (usually rendering runs asynchronously)
            if (forcedPaint || window->firstFramePending)
                base::GetService<DeviceService>()->device()->sync();

            // show the window
            if (window->firstFramePending)
            {
                window->window->windowShow();
                window->firstFramePending = false;
            }

            //base::Sleep(200);
        }
    }

    uint64_t NativeWindowRenderer::resolveNativeWindowHandle(ui::NativeWindowID id) const
    {
        for (auto* info : m_nativeWindowMap.values())
            if (info->id == id)
                return info->window->windowGetNativeHandle();

        return 0;
    }

    //--

    void NativeWindowRenderer::onOutputWindowStateChanged(ObjectID output, bool active)
    {

    }

    void NativeWindowRenderer::onOutputWindowPlacementChanged(ObjectID output, const base::Rect& newSize, float pixelScale, bool duringSizeMove)
    {
        if (duringSizeMove)
        {
            for (auto* info : m_nativeWindowMap.values())
            {
                if (info->output == output)
                {
                    if (info->lastPaintSize != newSize.size())
                    {
                        TRACE_INFO("Has to repain window to [{},{}], last paint size: [{},{}]", newSize.width(), newSize.height(), info->lastPaintSize.x, info->lastPaintSize.y);
                        if (info->callback)
                            info->callback->nativeWindowForceRedraw(info->id);
                    }
                    break;
                }
            }
        }
    }

    bool NativeWindowRenderer::onOutputWindowSelectCursor(ObjectID output, const base::Point& absolutePosition, base::input::CursorType& outCursorType)
    {
        for (auto* info : m_nativeWindowMap.values())
        {
            if (info->output == output)
            {
                if (info->callback)
                    return info->callback->nativeWindowSelectCursor(info->id, absolutePosition.toVector(), outCursorType);
                break;
            }
        }

        return false;
    }

    bool NativeWindowRenderer::onOutputWindowHitTestNonClientArea(ObjectID output, const base::Point& absolutePosition, base::input::AreaType& outAreaType)
    {
        for (auto* info : m_nativeWindowMap.values())
        {
            if (info->output == output)
            {
                if (info->callback)
                    return info->callback->nativeWindowHitTestNonClientArea(info->id, absolutePosition.toVector(), outAreaType);
                break;
            }
        }

        return false;
    }
    
    //--

} // rendering