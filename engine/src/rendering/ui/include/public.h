/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_ui_glue.inl"

namespace rendering
{
    class NativeWindowRenderer;
}

namespace ui
{
    class RenderingPanel;
    typedef base::RefPtr<RenderingPanel> RenderingPanelPtr;

    class RenderingScenePanel;
    typedef base::RefPtr<RenderingScenePanel> RenderingScenePanelPtr;

    class CameraViewportSetup;
}