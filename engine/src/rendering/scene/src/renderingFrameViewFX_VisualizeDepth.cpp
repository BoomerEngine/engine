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
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingShaderLibrary.h"

namespace rendering
{
    namespace scene
    {

        //---

        static base::res::StaticResource<ShaderLibrary> resDepthVis("/engine/shaders/screen/visualize_depth.fx");
        static base::res::StaticResource<ShaderLibrary> resDepthVisMSAA("/engine/shaders/screen/visualize_depth_msaa.fx");

        struct DepthVisParams
        {
            ImageView source;
        };

        void VisualizeDepthBuffer(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& depthSource, const ImageView& targetColor)
        {
            bool msaa = depthSource.numSamples() > 1;
            auto shader = msaa ? resDepthVisMSAA.loadAndGet() : resDepthVis.loadAndGet();

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

                DepthVisParams params;
                //params.consts = cmd.opUploadConstants(data);
                params.source = depthSource;
                cmd.opBindParametersInline("DepthVisParams"_id, params);

                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
                cmd.opDraw(shader, 0, 4);

                cmd.opEndPass();
            }
        }

        //---

    } // scene
} // rendering

