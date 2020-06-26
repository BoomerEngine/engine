/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"

namespace rendering
{
    namespace scene
    {

        //---

        // resolve MSAA color to non MSAA 
        extern RENDERING_SCENE_API void ResolveMSAAColor(command::CommandWriter& cmd, const FrameView& view, const ImageView& sourceRT, const ImageView& dest);

        // resolve MSAA depth to non MSAA depth 
        extern RENDERING_SCENE_API void ResolveMSAADepth(command::CommandWriter& cmd, const FrameView& view, const ImageView& sourceRT, const ImageView& dest);

        //---

        // copy surface to TARGET surface
        extern RENDERING_SCENE_API void PostFxTargetBlit(command::CommandWriter& cmd, const FrameRenderer& frame, const ImageView& source, const ImageView& target, bool applyGamma = true);

        //--

        // visualize depth buffer
        extern RENDERING_SCENE_API void VisualizeDepthBuffer(command::CommandWriter& cmd, const FrameRenderer& view, const ImageView& depthSource, const ImageView& targetColor);

        // visualize raw luminance (no exposure)
        extern RENDERING_SCENE_API void VisualizeLuminance(command::CommandWriter& cmd, const FrameRenderer& view, const ImageView& colorSource, const ImageView& targetColor);

        //--

    } // scene
} // rendering

