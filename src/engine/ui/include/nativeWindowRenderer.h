/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: ui #]
***/

#pragma once

#include "uiRenderer.h"

#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/canvasStyle.h"
#include "engine/canvas/include/canvasGeometryBuilder.h"

#include "gpu/device/include/renderingOutput.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// rendering payload for rendering canvas data
class ENGINE_UI_API NativeWindowRenderer : public IRendererNative, public gpu::INativeWindowCallback
{
    RTTI_DECLARE_POOL(POOL_WINDOW)

public:
    NativeWindowRenderer();
    virtual ~NativeWindowRenderer();

    virtual Position mousePosition() const override;
    virtual bool testPosition(const Position& pos) const override;
    virtual bool testArea(const ElementArea& area) const override;
    virtual ElementArea adjustArea(const ElementArea& area, WindowInitialPlacementMode placement = WindowInitialPlacementMode::ScreenCenter) const override;
    virtual void playSound(MessageType type) override;

    virtual NativeWindowID windowCreate(const NativeWindowSetup& setup) override;
    virtual void windowDestroy(NativeWindowID id)  override;
    virtual NativeWindowID windowAtPos(const Position& absoluteWindowPosition) override;
    virtual ElementArea windowMonitorAtPos(const Position& absoluteWindowPosition) override;
    virtual input::EventPtr windowPullInputEvent(NativeWindowID id) override;
    virtual uint64_t windowNativeHandle(NativeWindowID id) override;
    virtual void windowShow(NativeWindowID id, bool visible) override;
    virtual void windowEnable(NativeWindowID id, bool enabled) override;
    virtual void windowSetPos(NativeWindowID id, const Position& pos) override;
    virtual void windowSetSize(NativeWindowID id, const Size& size) override;
    virtual void windowSetPlacement(NativeWindowID id, const Position& pos, const Size& size) override;
    virtual float windowPixelScale(NativeWindowID id) override;
    virtual void windowSetFocus(NativeWindowID id) override;
    virtual bool windowGetFocus(NativeWindowID id) override;
    virtual bool windowGetMinimized(NativeWindowID id) override;
    virtual bool windowGetMaximized(NativeWindowID id) override;
    virtual bool windowGetVisible(NativeWindowID id) override;
    virtual void windowSetTitle(NativeWindowID id, StringView txt) override;
    virtual void windowSetOpacity(NativeWindowID id, float opacity) override;
    virtual void windowUpdate(NativeWindowID id) override;
    virtual void windowRenderContent(NativeWindowID id, const ElementArea& area, bool forcedPaint, const canvas::Canvas& canvas) override;
    virtual void windowSetCapture(NativeWindowID id, int mode) override;
    virtual bool windowGetRenderableArea(NativeWindowID id, ElementArea& outWindowDrawArea) override;
    virtual bool windowHasCloseRequest(NativeWindowID id) override;
    virtual bool windowGetResizable(NativeWindowID id) override;
    virtual bool windowGetMovable(NativeWindowID id) override;
    virtual bool windowGetDefaultPlacement(NativeWindowID id, Rect& outDefaultPlacement) override;
    virtual void windowMinimize(NativeWindowID id) override;
    virtual void windowMaximize(NativeWindowID id) override;

    virtual bool stroreClipboardData(StringView format, const void* data, uint32_t size) override final;
    virtual Buffer loadClipboardData(StringView format) override final;
    virtual bool checkClipboardHasData(StringView format) override final;

    /// INativeWindowCallback
    virtual void onOutputWindowStateChanged(gpu::ObjectID output, bool active) override;
    virtual void onOutputWindowPlacementChanged(gpu::ObjectID output, const Rect& newSize, float pixelScale, bool duringSizeMove) override;
    virtual bool onOutputWindowSelectCursor(gpu::ObjectID output, const Point& absolutePosition, input::CursorType& outCursorType) override;
    virtual bool onOutputWindowHitTestNonClientArea(gpu::ObjectID output, const Point& absolutePosition, input::AreaType& outAreaType) override;

private:
    struct NativeWindow : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_WINDOW)

    public:
        NativeWindowID id;
        NativeWindowID ownerId;
        gpu::OutputObjectPtr output; // window+swapchain
        bool firstFramePending = false;
        bool showOnFirstFrame = false;
        Point lastPaintSize;
        StringBuf lastTitle;
        ui::INativeWindowCallback* callback = nullptr;
    };

    HashMap<NativeWindowID, NativeWindow*> m_nativeWindowMap;
    NativeWindowID m_nextNativeWindowId = 1;

    uint64_t resolveNativeWindowHandle(NativeWindowID id) const;
};

///---

END_BOOMER_NAMESPACE_EX(ui)
