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

        static base::res::StaticResource<ShaderLibrary> resSelectionOutline("/engine/shaders/screen/selection_outline.fx");

        struct SelectionOutlineParams
        {
            struct Data
            {
                base::Vector4 colorFront;
                base::Vector4 colorBack;
                int outlineSize = 4;
                float centerOpacity = 0.5f;
            };

            ConstantsView consts;
            ImageView sceneDepth;
            ImageView selectionDepth;
        };

        void VisualizeSelectionOutline(command::CommandWriter& cmd, uint32_t width, uint32_t height, const ImageView& targetColor, const ImageView& sceneDepth, const ImageView& selectionDepth, const FrameParams_SelectionOutline& params)
        {
            auto shader = resSelectionOutline.loadAndGet();

            if (shader)
            {
                FrameBuffer fb;
                fb.color[0].rt = targetColor;
                fb.color[0].loadOp = LoadOp::Keep;
                fb.color[0].storeOp = StoreOp::Store;

                FrameBufferViewportState viewport;
                viewport.minDepthRange = 0.0f;
                viewport.maxDepthRange = 1.0f;
                viewport.viewportRect = base::Rect(0, 0, width, height);

                cmd.opBeingPass(fb, 1, &viewport);
                //cmd.opSetBlendState(0, BlendFactor::SrcAlpha, BlendFactor::One);
                cmd.opSetBlendState(0, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

                SelectionOutlineParams::Data data;
                data.colorFront = params.colorFront.toVectorLinear(); // NOTE: outline usually happens post-tone mapping
                data.colorBack = params.colorBack.toVectorLinear(); 
                data.centerOpacity = params.centerOpacity;
                data.outlineSize = params.outlineWidth;

                SelectionOutlineParams params;
                params.consts = cmd.opUploadConstants(data);
                params.sceneDepth = sceneDepth;
                params.selectionDepth = selectionDepth;
                cmd.opBindParametersInline("SelectionOutlineParams"_id, params);

                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleStrip);
                cmd.opDraw(shader, 0, 4);

                cmd.opEndPass();
            }
        }

        //---

    } // scene
} // rendering

