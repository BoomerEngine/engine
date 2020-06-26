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
        //--

        // render debug fragments
        extern RENDERING_SCENE_API void RenderDebugFragments(command::CommandWriter& cmd, const FrameView& view, const FrameViewCamera& camera, const DebugGeometry& geom);

        // render scene fragments
        extern RENDERING_SCENE_API void RenderViewFragments(command::CommandWriter& cmd, const FrameView& view, const FrameViewCamera& camera, const FragmentRenderContext& context, const std::initializer_list<FragmentDrawBucket>& buckets);

        //--

    } // scene
} // rendering

