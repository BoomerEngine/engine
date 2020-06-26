/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: ui #]
***/

#pragma once

#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasStyle.h"
#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/ui/include/uiRenderer.h"
#include "rendering/driver/include/renderingOutput.h"

namespace rendering
{
    //--

    /// rendering payload for rendering canvas data
    class RENDERING_UI_API NativeWindowRenderer : public ui::IRendererNative, public IDriverNativeWindowCallback
    {
    public:
        NativeWindowRenderer();
        virtual ~NativeWindowRenderer();

        virtual ui::Position mousePosition() const override;
        virtual bool testPosition(const ui::Position& pos) const override;
        virtual bool testArea(const ui::ElementArea& area) const override;
        virtual ui::ElementArea adjustArea(const ui::ElementArea& area, ui::WindowInitialPlacementMode placement = ui::WindowInitialPlacementMode::ScreenCenter) const override;

        virtual ui::NativeWindowID windowCreate(const ui::NativeWindowSetup& setup) override;
        virtual void windowDestroy(ui::NativeWindowID id)  override;
        virtual ui::NativeWindowID windowAtPos(const ui::Position& absoluteWindowPosition) override;
        virtual ui::ElementArea windowMonitorAtPos(const ui::Position& absoluteWindowPosition) override;
        virtual base::input::EventPtr windowPullInputEvent(ui::NativeWindowID id) override;
        virtual void windowShow(ui::NativeWindowID id, bool visible) override;
        virtual void windowSetPos(ui::NativeWindowID id, const ui::Position& pos) override;
        virtual void windowSetSize(ui::NativeWindowID id, const ui::Size& size) override;
        virtual float windowPixelScale(ui::NativeWindowID id) override;
        virtual void windowSetFocus(ui::NativeWindowID id) override;
        virtual bool windowGetFocus(ui::NativeWindowID id) override;
        virtual bool windowGetMinimized(ui::NativeWindowID id) override;
        virtual bool windowGetMaximized(ui::NativeWindowID id) override;
        virtual void windowSetTitle(ui::NativeWindowID id, base::StringView<char> txt) override;
        virtual void windowSetOpacity(ui::NativeWindowID id, float opacity) override;
        virtual void windowUpdate(ui::NativeWindowID id) override;
        virtual void windowRenderContent(ui::NativeWindowID id, const ui::Size& size, bool forcedPaint, const base::canvas::Canvas& canvas) override;
        virtual void windowSetCapture(ui::NativeWindowID id, int mode) override;
        virtual bool windowGetRenderableArea(ui::NativeWindowID id, ui::ElementArea& outWindowDrawArea) override;
        virtual bool windowHasCloseRequest(ui::NativeWindowID id) override;
        virtual bool windowGetResizable(ui::NativeWindowID id) override;
        virtual bool windowGetMovable(ui::NativeWindowID id) override;

        virtual bool stroreClipboardData(base::StringView<char> format, const void* data, uint32_t size) override final;
        virtual base::Buffer loadClipboardData(base::StringView<char> format) override final;

        /// IDriverNativeWindowCallback
        virtual void onOutputWindowStateChanged(ObjectID output, bool active) override;
        virtual void onOutputWindowPlacementChanged(ObjectID output, const base::Rect& newSize, float pixelScale, bool duringSizeMove) override;
        virtual bool onOutputWindowSelectCursor(ObjectID output, const base::Point& absolutePosition, base::input::CursorType& outCursorType) override;
        virtual bool onOutputWindowHitTestNonClientArea(ObjectID output, const base::Point& absolutePosition, base::input::AreaType& outAreaType) override;

    private:
        struct NativeWindow
        {
            ui::NativeWindowID id;
            ui::NativeWindowID ownerId;
            ObjectID output; // window+swapchain
            IDriverNativeWindowInterface* window = nullptr;
            bool firstFramePending = false;
            base::Point lastPaintSize;
            base::StringBuf lastTitle;
            ui::INativeWindowCallback* callback = nullptr;
        };

        base::HashMap<ui::NativeWindowID, NativeWindow*> m_nativeWindowMap;
        ui::NativeWindowID m_nextNativeWindowId = 1;

        uint64_t resolveNativeWindowHandle(ui::NativeWindowID id) const;
    };

    ///---

} // rendering