/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
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

        void ResolveMSAAColor(command::CommandWriter& cmd, const FrameView& view, const ImageView& sourceRT, const ImageView& dest)
        {
            DEBUG_CHECK(sourceRT.renderTarget());

            // TODO: copy in case no resolve ?

            command::CommandWriterBlock block(cmd, "ResolveMSAAColor");
            cmd.opResolve(sourceRT, dest);
        }

        void ResolveMSAADepth(command::CommandWriter& cmd, const FrameView& view, const ImageView& sourceRT, const ImageView& dest)
        {
            DEBUG_CHECK(sourceRT.renderTargetDepth());

            // TODO: copy in case no resolve ?

            command::CommandWriterBlock block(cmd, "ResolveMSAADepth");
            cmd.opResolve(sourceRT, dest);
        }

        //---

        base::res::StaticResource<ShaderLibrary> resBlitShader("engine/shaders/postfx/postfx_blit.fx");

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

                float gamma = 1.0f;
            };

            ConstantsView consts;
            ImageView source;
        };

        void PostFxTargetBlit(command::CommandWriter& cmd, const FrameRenderer& frame, const ImageView& source, const ImageView& dest, bool applyGamma)
        {
            command::CommandWriterBlock block(cmd, "PostFxTargetBlit");

            // TODO: compute based blit ? tests show that PS if still faster in 2019

            if (auto shader = resBlitShader.loadAndGet())
            {
                FrameBuffer fb;
                fb.color[0].rt = dest;
                fb.color[0].loadOp = LoadOp::DontCare;
                fb.color[0].storeOp = StoreOp::Store;

                FrameBufferViewportState viewport;
                viewport.minDepthRange = 0.0f;
                viewport.maxDepthRange = 1.0f;
                viewport.viewportRect = base::Rect(0, 0, frame.frame().resolution.finalCompositionWidth, frame.frame().resolution.finalCompositionHeight);

                cmd.opBeingPass(fb, 1, &viewport);

                BlitParams::Constants data;
                data.targetOffsetX = 0;
                data.targetSizeX = frame.frame().resolution.finalCompositionWidth;

                if (dest.flippedY())
                {
                    data.targetOffsetY = frame.frame().resolution.finalCompositionHeight - 1;
                    data.targetSizeY = -(int)frame.frame().resolution.finalCompositionHeight;
                }
                else
                {
                    data.targetOffsetY = 0;
                    data.targetSizeY = frame.frame().resolution.finalCompositionHeight;
                }

                data.targetToSourceScaleX = frame.frame().resolution.width / (float)data.targetSizeX;
                data.targetToSourceScaleY = frame.frame().resolution.height / (float)data.targetSizeY;
                data.sourceInvSizeX = 1.0f / source.width();
                data.sourceInvSizeY = 1.0f / source.height();
                data.gamma = applyGamma ? 1.0f / 2.2f : 1.0f;

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

        base::res::StaticResource<ShaderLibrary> resDepthVis("engine/shaders/postfx/postfx_depth_vis.fx");
        base::res::StaticResource<ShaderLibrary> resDepthVisMSAA("engine/shaders/postfx/postfx_depth_vis_msaa.fx");

        struct DepthVisParams
        {
            ImageView source;
        };

        void VisualizeDepthBuffer(command::CommandWriter& cmd, const FrameRenderer& view, const ImageView& depthSource, const ImageView& targetColor)
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
                viewport.viewportRect = base::Rect(0, 0, view.frame().resolution.width, view.frame().resolution.height);

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

        base::res::StaticResource<ShaderLibrary> resLumVis("engine/shaders/postfx/postfx_lum_vis.fx");
        base::res::StaticResource<ShaderLibrary> resLumVisMSAA("engine/shaders/postfx/postfx_lum_vis_msaa.fx");

        struct LumVisParams
        {
            ImageView source;
        };

        void VisualizeLuminance(command::CommandWriter& cmd, const FrameRenderer& view, const ImageView& colorSource, const ImageView& targetColor)
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
                viewport.viewportRect = base::Rect(0, 0, view.frame().resolution.width, view.frame().resolution.height);

                cmd.opBeingPass(fb, 1, &viewport);

                DepthVisParams params;
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

