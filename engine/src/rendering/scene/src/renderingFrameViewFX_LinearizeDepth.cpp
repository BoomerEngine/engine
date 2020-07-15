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

        base::res::StaticResource<ShaderLibrary> resLinearizeDepth("engine/shaders/screen/linearize_depth.fx");

        //---

        struct LinearizeDepthParams
        {
            struct Constants
            {
                int sizeX = 1;
                int sizeY = 1;
                float linearizeZ = 0.0f;
                float linearizeW = 0.0f;
            };

            ConstantsView consts;

            ImageView depthBuffer;
            ImageView linearizedDepthBuffer;
        };

        void LinearizeDepth(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const Camera& viewCamera, const ImageView& sourceDepth, const ImageView& targetLinearDepth)
        {
            command::CommandWriterBlock block(cmd, "LinearizeDepth");
            
            if (auto shader = resLinearizeDepth.loadAndGet())
            {
                LinearizeDepthParams::Constants data;
                data.sizeX = sourceWidth;
                data.sizeY = sourceHeight;
                data.linearizeZ = viewCamera.viewToScreen().m[2][2];
                data.linearizeW = viewCamera.viewToScreen().m[2][3];

                LinearizeDepthParams params;
                params.consts = cmd.opUploadConstants(data);
                params.depthBuffer = sourceDepth;
                params.linearizedDepthBuffer = targetLinearDepth;
                cmd.opBindParametersInline("LinearizeDepthParams"_id, params);

                const auto groupsX = base::GroupCount(sourceWidth, 8);
                const auto groupsY = base::GroupCount(sourceHeight, 8);
                cmd.opDispatch(shader, groupsX, groupsY);
            }
        }

        //---

    } // scene
} // rendering

