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

        static base::res::StaticResource<ShaderLibrary> resLumVis("/engine/shaders/screen/visualize_luminance.fx");
        static base::res::StaticResource<ShaderLibrary> resLumVisMSAA("/engine/shaders/screen/visualize_luminance_msaa.fx");

        struct LumVisParams
        {
            ImageView source;
        };

        void VisualizeLuminance(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& colorSource, const ImageView& targetColor)
        {
            bool msaa = colorSource.numSamples() > 1;
            auto shader = msaa ? resLumVisMSAA.loadAndGet() : resLumVis.loadAndGet();

            if (shader)
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

                LumVisParams params;
                //params.consts = cmd.opUploadConstants(data);
                params.source = colorSource;
                cmd.opBindParametersInline("LumVisParams"_id, params);

                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
                cmd.opDraw(shader, 0, 4);

                cmd.opEndPass();
            }
        }

        //---

    } // scene
} // rendering

