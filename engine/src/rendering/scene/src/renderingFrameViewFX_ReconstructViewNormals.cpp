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
#include "base/math/include/randomMersenne.h"

namespace rendering
{
    namespace scene
    {

        //---

        base::res::StaticResource<ShaderLibrary> resReconstructViewNormals("engine/shaders/screen/reconstruct_view_normal.fx");

        //---

        struct ReconstructViewNormalsParams
        {
            struct Constants
            {
                int sizeX = 1;
                int sizeY = 1;
                base::Vector2 invSize;
                base::Vector4 projInfo;
            };

            ConstantsView consts;

            ImageView sourceLinearDepth;
            ImageView targetReconstructedNormals;
        };

        static base::Vector4 CalcProjectionInfo(const Camera& viewCamera)
        {
            const auto& viewToScreen = viewCamera.viewToScreen();

            base::Vector4 ret;
            ret.x = 2.0f / viewToScreen.m[0][0]; // (x) * (R - L)/N;
            ret.y = 2.0f / viewToScreen.m[1][1]; // (y) * (R - L)/N;
            ret.z = -(1.0f - viewToScreen.m[0][2]) / viewToScreen.m[0][0]; // L/N
            ret.w = -(1.0f - viewToScreen.m[1][2]) / viewToScreen.m[1][1]; // B/N
            return  ret;
        }

        void ReconstructViewNormals(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const Camera& viewCamera, const ImageView& sourceLinearDepth, const ImageView& targetReconstructedNormals)
        {
            command::CommandWriterBlock block(cmd, "ReconstructViewNormals");
            
            if (auto shader = resReconstructViewNormals.loadAndGet())
            {
                ReconstructViewNormalsParams::Constants data;
                data.sizeX = sourceWidth;
                data.sizeY = sourceHeight;
                data.invSize.x = 1.0f / sourceWidth;
                data.invSize.y = 1.0f / sourceHeight;
                data.projInfo = CalcProjectionInfo(viewCamera);

                ReconstructViewNormalsParams params;
                params.consts = cmd.opUploadConstants(data);
                params.sourceLinearDepth = sourceLinearDepth;
                params.targetReconstructedNormals = targetReconstructedNormals;
                cmd.opBindParametersInline("ReconstructNormalParams"_id, params);

                const auto groupsX = base::GroupCount(sourceWidth, 8);
                const auto groupsY = base::GroupCount(sourceHeight, 8);
                cmd.opDispatch(shader, groupsX, groupsY);
            }
        }

        //---

    } // scene
} // rendering

