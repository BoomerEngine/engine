/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\compute #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameSurfaceCache.h"
#include "renderingFrameParams.h"
#include "renderingFrameView.h"

#include "base/containers/include/stringBuilder.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingShaderLibrary.h"

namespace rendering
{
    namespace scene
    {

        //---

        static base::res::StaticResource<ShaderLibrary> resLinearDepthVis("/engine/shaders/screen/visualize_linear_depth.fx");

        struct LinearDepthVisParams
        {
            ImageView source;
        };

        void VisualizeLinearDepthBuffer(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& depthSource, const ImageView& targetColor)
        {
            if (auto shader = resLinearDepthVis.loadAndGet())
            {
                FrameBuffer fb;
                fb.color[0].rt = targetColor;
                fb.color[0].loadOp = LoadOp::DontCare;
                fb.color[0].storeOp = StoreOp::Store;

                FrameBufferViewportState viewport;
                viewport.minDepthRange = 0.0f;
                viewport.maxDepthRange = 1.0f;
                viewport.viewportRect = base::Rect(0, 0, width, height);

                cmd.opBeingPass(fb, 1, &viewport);

                LinearDepthVisParams params;
                params.source = depthSource;
                cmd.opBindParametersInline("LinearDepthVisParams"_id, params);

                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
                cmd.opDraw(shader, 0, 4);

                cmd.opEndPass();
            }
        }

        //---

    } // scene
} // rendering

