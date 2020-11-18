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

        static base::res::StaticResource<ShaderLibrary> resChannelVis("/engine/shaders/screen/visualize_channel.fx");
        static base::res::StaticResource<ShaderLibrary> resChannelVisMSAA("/engine/shaders/screen/visualize_channel_msaa.fx");

        struct ChannelVisParams
        {
            struct Data
            {
                base::Vector4 colorDot;
                base::Vector4 colorMul;

                uint32_t flag = 0;
                uint32_t _pad0;
                uint32_t _pad1;
                uint32_t _pad2;
            };

            ConstantsView consts;
            ImageView source;
        };

        void VisualizeTexture(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& colorSource, const ImageView& targetColor, const base::Vector4& dot, const base::Vector4& mul)
        {
            command::CommandWriterBlock block(cmd, "VisualizeTexture");

            bool msaa = colorSource.numSamples() > 1;
            auto shader = msaa ? resChannelVisMSAA.loadAndGet() : resChannelVis.loadAndGet();

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

                ChannelVisParams::Data data;
                data.colorDot = dot;
                data.colorMul = mul;
                data.flag = (dot == base::Vector4::ZERO());

                ChannelVisParams params;
                params.consts = cmd.opUploadConstants(data);
                params.source = colorSource;
                cmd.opBindParametersInline("ChannelVisParams"_id, params);

                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
                cmd.opDraw(shader, 0, 4);

                cmd.opEndPass();
            }
        }

        //---

    } // scene
} // rendering

