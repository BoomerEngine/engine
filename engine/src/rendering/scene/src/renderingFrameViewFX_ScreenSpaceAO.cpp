/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\compute #]
***/

/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

        base::res::StaticResource<ShaderLibrary> resHBAOCalcShader("engine/shaders/screen/hbao_calc.fx");

        #define AO_RANDOMTEX_SIZE 4

        struct HBAOConstants
        {
            float RadiusToScreen;        // radius
            float R2;     // 1/radius
            float NegInvR2;     // radius * radius
            float NDotVBias;

            base::Vector2 InvFullResolution;
            base::Vector2 InvQuarterResolution;

            float AOMultiplier;
            float PowExponent;
            uint32_t ScreenWidth;
            uint32_t ScreenHeight;

            base::Vector4 projInfo;
            base::Vector2 projScale;
            int projOrtho;
            int _pad1;

            base::Vector4 float2Offsets[AO_RANDOMTEX_SIZE * AO_RANDOMTEX_SIZE];
            base::Vector4 jitters[AO_RANDOMTEX_SIZE * AO_RANDOMTEX_SIZE];
        };

        struct HBAOCalcParams
        {
            ConstantsView consts;
            ImageView linearizedDepthBuffer;
            ImageView targetShadowMaskAO;
        };

        static const int HBAO_RANDOM_SIZE = AO_RANDOMTEX_SIZE;
        static const int HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE * HBAO_RANDOM_SIZE;

        static base::Vector4 HBAO_RANDOM[HBAO_RANDOM_ELEMENTS];

        static const base::Vector4* HBAO_RANDOM_INIT()
        {
            base::MTRandState rng;

            float numDir = 8.0f; // keep in sync to glsl
            for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++)
            {
                float Rand1 = base::RandOne(rng);
                float Rand2 = base::RandOne(rng);

                // Use random rotation angles in [0,2PI/NUM_DIRECTIONS)
                float angle = 2.f * PI * Rand1 / numDir;
                HBAO_RANDOM[i].x = cosf(angle);
                HBAO_RANDOM[i].y = sinf(angle);
                HBAO_RANDOM[i].z = Rand2;
                HBAO_RANDOM[i].w = 0;
            }

            return HBAO_RANDOM;
        }

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

        static void ComputeHBAOConstants(uint32_t sourceWidth, uint32_t sourceHeight, const Camera& viewCamera, const FrameParams_AmbientOcclusion& setup, HBAOConstants& outData)
        {
            const auto projScale = float(sourceHeight) / (tanf(viewCamera.fOV() * 0.5f) * 2.0f);

            float meters2viewspace = 1.0f;
            float R = setup.radius * meters2viewspace;
            outData.R2 = R * R;
            outData.NegInvR2 = -1.0f / outData.R2;
            outData.RadiusToScreen = R * 0.5f * projScale;

            // ao
            outData.PowExponent = std::max(setup.intensity, 0.0f);
            outData.NDotVBias = std::min(std::max(0.0f, setup.bias), 1.0f);
            outData.AOMultiplier = 1.0f / (1.0f - outData.NDotVBias);

            // resolution
            int quarterWidth = ((sourceWidth + 3) / 4);
            int quarterHeight = ((sourceHeight + 3) / 4);
            outData.InvQuarterResolution = base::Vector2(1.0f / float(quarterWidth), 1.0f / float(quarterHeight));
            outData.InvFullResolution = base::Vector2(1.0f / float(sourceWidth), 1.0f / float(sourceHeight));
            outData.ScreenWidth = sourceWidth;
            outData.ScreenHeight = sourceHeight;

            // projection params
            outData.projInfo = CalcProjectionInfo(viewCamera);
            outData.projOrtho = 0;

            static const auto* randomData = HBAO_RANDOM_INIT();
            for (int i = 0; i < (AO_RANDOMTEX_SIZE * AO_RANDOMTEX_SIZE); i++)
            {
                outData.float2Offsets[i] = base::Vector4(float(i % 4) + 0.5f, float(i / 4) + 0.5f, 0.0f, 0.0f);
                outData.jitters[i] = randomData[i];
            }
        }

        void HBAOIntoShadowMask(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& sourceLinearDepth, const ImageView& targetSSAOMask, const Camera& viewCamera, const FrameParams_AmbientOcclusion& setup)
        {
            command::CommandWriterBlock block(cmd, "HBAO");

            HBAOConstants data;
            ComputeHBAOConstants(sourceWidth, sourceHeight, viewCamera, setup, data);

            // calculate
            if (auto shader = resHBAOCalcShader.loadAndGet())
            {
                HBAOCalcParams params;
                params.consts = cmd.opUploadConstants(data);
                params.linearizedDepthBuffer = sourceLinearDepth;
                params.targetShadowMaskAO = targetSSAOMask;
                cmd.opBindParametersInline("HBAOCalcParams"_id, params);

                const auto groupsX = base::GroupCount(sourceWidth, 8);
                const auto groupsY = base::GroupCount(sourceHeight, 8);
                cmd.opDispatch(shader, groupsX, groupsY);
            }
        }

        //---

    } // scene
} // rendering

