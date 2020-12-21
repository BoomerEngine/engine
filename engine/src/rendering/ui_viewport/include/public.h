/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_ui_viewport_glue.inl"

namespace ui
{
    class RenderingScenePanel;
    typedef base::RefPtr<RenderingScenePanel> RenderingScenePanelPtr;

	class RenderingFullScenePanel;
	typedef base::RefPtr<RenderingFullScenePanel> RenderingFullScenePanelPtr;

    class CameraViewportSetup;
}