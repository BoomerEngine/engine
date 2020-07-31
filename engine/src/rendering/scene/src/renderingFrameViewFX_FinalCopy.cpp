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

        static base::res::StaticResource<ShaderLibrary> resBlitShader("/engine/shaders/screen/final_copy.fx");

        struct BlitParams
        {
            struct Constants
            {
                int targetOffsetX = 0;
                int targetOffsetY = 0;
                int targetSizeX = 1;
                int targetSizeY = 1;

                float targetToSourceScaleX = 1.0f;
                float targetToSourceScaleY = 1.0f;
                float sourceInvSizeX = 1.0f;
                float sourceInvSizeY = 1.0f;

                float gamma;
            };

            ConstantsView consts;
            ImageView source;
        };

        void FinalCopy(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& source, uint32_t targetWidth, uint32_t targetHeight, const ImageView& target, float gamma)
        {
            command::CommandWriterBlock block(cmd, "FinalCopy");

            // TODO: compute based blit ? tests show that PS if still faster in 2019

            if (auto shader = resBlitShader.loadAndGet())
            {
                FrameBuffer fb;
                fb.color[0].rt = target;
                fb.color[0].loadOp = LoadOp::DontCare;
                fb.color[0].storeOp = StoreOp::Store;

                FrameBufferViewportState viewport;
                viewport.minDepthRange = 0.0f;
                viewport.maxDepthRange = 1.0f;
                viewport.viewportRect = base::Rect(0, 0, targetWidth, targetHeight);

                cmd.opBeingPass(fb, 1, &viewport);

                BlitParams::Constants data;
                data.targetOffsetX = 0;
                data.targetSizeX = targetWidth;

                if (target.flippedY())
                {
                    data.targetOffsetY = targetHeight - 1;
                    data.targetSizeY = -(int)targetHeight;
                }
                else
                {
                    data.targetOffsetY = 0;
                    data.targetSizeY = targetHeight;
                }

                data.targetToSourceScaleX = sourceWidth / (float)data.targetSizeX;
                data.targetToSourceScaleY = sourceHeight / (float)data.targetSizeY;
                data.sourceInvSizeX = 1.0f / source.width();
                data.sourceInvSizeY = 1.0f / source.height();
                data.gamma = gamma;

                BlitParams params;
                params.consts = cmd.opUploadConstants(data);
                params.source = source;
                cmd.opBindParametersInline("BlitParams"_id, params);

                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
                cmd.opDraw(shader, 0, 4);

                cmd.opEndPass();
            }
        }

        //---

    } // scene
} // rendering

