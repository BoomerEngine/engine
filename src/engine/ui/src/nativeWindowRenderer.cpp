/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: ui #]
***/

#include "build.h"
#include "nativeWindowRenderer.h"

#include "core/image/include/image.h"
#include "core/image/include/imageView.h"
#include "core/image/include/imageUtils.h"
#include "core/input/include/inputContext.h"
#include "core/system/include/scopeLock.h"
#include "core/system/include/thread.h"

#include "gpu/device/include/commandBuffer.h"
#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/object.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/output.h"

#include "engine/canvas/include/service.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

NativeWindowRenderer::NativeWindowRenderer()
{}

NativeWindowRenderer::~NativeWindowRenderer()
{
    m_nativeWindowMap.clearPtr();
}

ElementArea NativeWindowRenderer::adjustArea(const ElementArea& area, WindowInitialPlacementMode placement /*= WindowInitialPlacementMode::ScreenCenter*/) const
{
    return ElementArea();
}

ElementArea NativeWindowRenderer::windowMonitorAtPos(const Position& absoluteWindowPosition)
{
    InplaceArray<Rect, 8> monitorAreas;
    GetService<DeviceService>()->device()->enumMonitorAreas(monitorAreas);

    Point testPoint((int)absoluteWindowPosition.x, (int)absoluteWindowPosition.y);
    for (const auto& area : monitorAreas)
    {
        if (area.contains(testPoint))
            return area;
    }

    auto mousePos = Point(mousePosition());
    for (const auto& area : monitorAreas)
    {
        if (area.contains(mousePos))
            return area;
    }

    return monitorAreas[0];
}

NativeWindowID NativeWindowRenderer::windowCreate(const NativeWindowSetup& setup)
{
    auto* device = GetService<DeviceService>()->device();
    if (!device)
        return 0;

    gpu::OutputInitInfo initInfo;
    initInfo.m_class = gpu::OutputClass::Window;
    initInfo.m_width = setup.size.x;
    initInfo.m_height = setup.size.y;
    initInfo.m_windowActivate = setup.activate;
    initInfo.m_windowTopmost = setup.topmost;
    initInfo.m_windowTitle = setup.title;
    initInfo.m_windowSystemBorder = false; // we never use system border for UI windows
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
	initInfo.m_windowAllowFullscreenToggle = false; // not in UI
    initInfo.m_windowCallback = this;

    NativeWindowID ownerWindowId = 0;
    if (setup.externalParentWindowHandle)
    {
        initInfo.m_windowNativeParent = setup.externalParentWindowHandle;
    }
    else
    {
        if (auto* ownerWindow = m_nativeWindowMap.findSafe(setup.owner, nullptr))
        {
            ownerWindowId = ownerWindow->id;
            if (ownerWindow->output)
                initInfo.m_windowNativeParent = ownerWindow->output->window()->windowGetNativeHandle();
        }
    }

    if (auto output = device->createOutput(initInfo))
    {
        auto wrapper = new NativeWindow;
        wrapper->id = m_nextNativeWindowId++;
        wrapper->ownerId = ownerWindowId;
        wrapper->output = output;
        wrapper->lastTitle = setup.title;
        wrapper->firstFramePending = true;
        wrapper->showOnFirstFrame = setup.visible;
        wrapper->callback = setup.callback;
        m_nativeWindowMap[wrapper->id] = wrapper;
        return wrapper->id;
    }

    return 0;
}

void NativeWindowRenderer::windowDestroy(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
    {
		if (window->output)
		{
			window->output->disconnect();
			window->output.reset();
		}

        delete window;
        m_nativeWindowMap.remove(id);
    }
    else
    {
        TRACE_WARNING("Native window ID {} not found", id);
    }
}

input::EventPtr NativeWindowRenderer::windowPullInputEvent(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        if (auto inputContext = window->output->window()->windowGetInputContext())
            return inputContext->pull();

    return nullptr;
}

void NativeWindowRenderer::windowShow(NativeWindowID id, bool visible)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        if (visible)
            window->output->window()->windowShow(false);
        else
            window->output->window()->windowHide();
}

void NativeWindowRenderer::windowMinimize(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        window->output->window()->windowMinimize();
}

void NativeWindowRenderer::windowMaximize(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        window->output->window()->windowMaximize();
}

void NativeWindowRenderer::windowEnable(NativeWindowID id, bool enabled)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        window->output->window()->windowEnable(enabled);
}

uint64_t NativeWindowRenderer::windowNativeHandle(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        return window->output->window()->windowGetNativeHandle();

    return 0;
}

void NativeWindowRenderer::windowSetPos(NativeWindowID id, const Position& pos)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
    {
        auto size = window->output->window()->windowGetClientSize();

        Rect rect;
        rect.min.x = (int)pos.x;
        rect.min.y = (int)pos.y;
        rect.max.x = rect.min.x + size.x;
        rect.max.y = rect.min.y + size.y;
        window->output->window()->windowAdjustClientPlacement(rect);
    }
}

void NativeWindowRenderer::windowSetSize(NativeWindowID id, const Size& size)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
    {
        auto pos = window->output->window()->windowGetClientPlacement();

        Rect rect;
        rect.min.x = pos.x;
        rect.min.y = pos.y;
        rect.max.x = rect.min.x + size.x;
        rect.max.y = rect.min.y + size.y;
        window->output->window()->windowAdjustClientPlacement(rect);
    }
}

void NativeWindowRenderer::windowSetPlacement(NativeWindowID id, const Position& pos, const Size& size)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
    {
        Rect rect;
        rect.min.x = pos.x;
        rect.min.y = pos.y;
        rect.max.x = rect.min.x + size.x;
        rect.max.y = rect.min.y + size.y;
        window->output->window()->windowAdjustClientPlacement(rect);
    }
}

float NativeWindowRenderer::windowPixelScale(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        return window->output->window()->windowGetPixelScale();

    return 1.0f;
}

void NativeWindowRenderer::windowSetFocus(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        window->output->window()->windowActivate();
}

bool NativeWindowRenderer::windowGetFocus(NativeWindowID id)
{
/*        if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        if (auto windowInterface = window->output->outputWindowInterface())
            return window->window->windowIsActive();*/

    for (auto* window : m_nativeWindowMap.values())
    {
        if (window->output->window()->windowIsActive())
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

bool NativeWindowRenderer::windowGetMinimized(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        return window->output->window()->windowIsMinimized();

    return false;
}

bool NativeWindowRenderer::windowGetResizable(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
    {
        if (!window->output->window()->windowIsMaximized())
            return true;
    }

    return false;
}

bool NativeWindowRenderer::windowGetMovable(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
    {
        if (!window->output->window()->windowIsMaximized())
            return true;
    }

    return false;
}

bool NativeWindowRenderer::windowGetDefaultPlacement(NativeWindowID id, Rect& outPlacement)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        return window->output->window()->windowGetWindowDefaultPlacement(outPlacement);

    return false;
}

bool NativeWindowRenderer::windowGetMaximized(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        return window->output->window()->windowIsMaximized();

    return false;
}

bool NativeWindowRenderer::windowGetVisible(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        return window->output->window()->windowIsVisible();

    return false;
}

void NativeWindowRenderer::windowSetTitle(NativeWindowID id, StringView txt)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        window->output->window()->windowSetTitle(StringBuf(txt));
}

void NativeWindowRenderer::windowSetOpacity(NativeWindowID id, float opacity)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        window->output->window()->windowSetAlpha(opacity);
}

bool NativeWindowRenderer::windowHasCloseRequest(NativeWindowID id)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
    {
        if (window->output->window()->windowHasCloseRequest())
        {
            window->output->window()->windowCancelCloseRequest(); // report it only once
            return true;
        }
    }

    return false;
}

void NativeWindowRenderer::windowUpdate(NativeWindowID id)
{
    // ?
}


void NativeWindowRenderer::windowSetCapture(NativeWindowID id, int mode)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
        if (auto inputContext = window->output->window()->windowGetInputContext())
            inputContext->requestCapture(mode);
}

bool NativeWindowRenderer::windowGetRenderableArea(NativeWindowID id, ElementArea& outWindowDrawArea)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
    {
        // do not render if window is minimized
        if (window->output->window()->windowIsMinimized())
            return false;

        // do not rendering if window is not visible, unless it's that first frame...
        if (!window->firstFramePending && !window->output->window()->windowIsVisible())
            return false;

        // use the window placement
        const auto windowPlacement = window->output->window()->windowGetClientPlacement();
        const auto windowSize = window->output->window()->windowGetClientSize();
        outWindowDrawArea = ElementArea(windowPlacement.x, windowPlacement.y, windowPlacement.x + windowSize.x, windowPlacement.y + windowSize.y);
        return true;
    }

    return false;
}

void NativeWindowRenderer::windowRenderContent(NativeWindowID id, const ElementArea& area, bool forcedPaint, const canvas::Canvas& canvas)
{
    if (auto* window = m_nativeWindowMap.findSafe(id, nullptr))
    {
        // write commands
        gpu::CommandWriter cmd(TempString("NativeWindowRender {} - '{}'", window->id, window->lastTitle));

        // acquire back buffer
        if (auto output = cmd.opAcquireOutput(window->output))
        {
			// remember the paint size
			window->lastPaintSize.x = output.width;
			window->lastPaintSize.y = output.height;

			// prepare frame buffer
            gpu::FrameBuffer fb;
			fb.depth.view(output.depth).clearDepth().clearStencil();
			fb.color[0].view(output.color);

			// if canvas requests clearing the window do it - this can be used to save initial background rect in the window
			if (canvas.clearColor().a > 0)
				fb.color[0].clear(canvas.clearColor());

			// set frame buffer
			cmd.opBeingPass(fb);

			// render canvas to command buffer
			GetService<canvas::CanvasService>()->render(cmd, canvas);

			// finish pass
			cmd.opEndPass();

			// swap to show in actual window
			cmd.opSwapOutput(window->output);
        }

        // submit work for rendering on GPU
        GetService<DeviceService>()->device()->submitWork(cmd.release());

		// make sure rendering is finished (usually rendering runs asynchronously)
		// this also finishes displaying any old frames for this window (at old size)
		if (forcedPaint || window->firstFramePending)
			GetService<DeviceService>()->device()->sync(true);

        // show the window
        if (window->firstFramePending)
        {
            if (window->showOnFirstFrame)
                window->output->window()->windowShow();

            window->firstFramePending = false;
        }

        //Sleep(200);
    }
}

uint64_t NativeWindowRenderer::resolveNativeWindowHandle(NativeWindowID id) const
{
    for (auto* info : m_nativeWindowMap.values())
        if (info->id == id)
            return info->output->window()->windowGetNativeHandle();

    return 0;
}

//--

void NativeWindowRenderer::onOutputWindowStateChanged(gpu::ObjectID output, bool active)
{

}

void NativeWindowRenderer::onOutputWindowPlacementChanged(gpu::ObjectID output, const Rect& newSize, float pixelScale, bool duringSizeMove)
{
    if (duringSizeMove)
    {
        for (auto* info : m_nativeWindowMap.values())
        {
            if (info->output->id() == output)
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

bool NativeWindowRenderer::onOutputWindowSelectCursor(gpu::ObjectID output, const Point& absolutePosition, input::CursorType& outCursorType)
{
    for (auto* info : m_nativeWindowMap.values())
    {
        if (info->output->id() == output)
        {
            if (info->callback)
                return info->callback->nativeWindowSelectCursor(info->id, absolutePosition.toVector(), outCursorType);
            break;
        }
    }

    return false;
}

bool NativeWindowRenderer::onOutputWindowHitTestNonClientArea(gpu::ObjectID output, const Point& absolutePosition, input::AreaType& outAreaType)
{
    for (auto* info : m_nativeWindowMap.values())
    {
        if (info->output->id() == output)
        {
            if (info->callback)
                return info->callback->nativeWindowHitTestNonClientArea(info->id, absolutePosition.toVector(), outAreaType);
            break;
        }
    }

    return false;
}
    
//--

END_BOOMER_NAMESPACE_EX(ui)
