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

        base::res::StaticResource<ShaderLibrary> resShadowMaskShader("/engine/shaders/screen/global_shadow_mask.fx");

        struct ShadowMaskParams
        {
            struct Constants
            {
                int sizeX = 1;
                int sizeY = 1;
            };

            ConstantsView consts;
            ImageView depthBuffer;
            ImageView shadowMask;
        };

        void GlobalShadowMask(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& sourceDepth, const ImageView& targetSSAOMask)
        {
            command::CommandWriterBlock block(cmd, "GlobalShadowMask");

            if (auto shader = resShadowMaskShader.loadAndGet())
            {
                ShadowMaskParams::Constants data;
                data.sizeX = sourceWidth;
                data.sizeY = sourceHeight;

                ShadowMaskParams params;
                params.consts = cmd.opUploadConstants(data);
                params.shadowMask = targetSSAOMask;
                params.depthBuffer = sourceDepth;
                cmd.opBindParametersInline("ShadowMaskParams"_id, params);

                const auto groupsX = base::GroupCount(sourceWidth, 8);
                const auto groupsY = base::GroupCount(sourceHeight, 8);
                cmd.opDispatch(shader, groupsX, groupsY);
            }
        }

        //---

    } // scene
} // rendering

