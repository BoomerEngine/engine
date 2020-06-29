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

        void FinalComposition(command::CommandWriter& cmd, uint32_t sourceWidth, uint32_t sourceHeight, const ImageView& sourceColor, uint32_t targetWidth, uint32_t targetHeight, const ImageView& target, const FrameParams_ToneMapping& toneMapping, const FrameParams_ColorGrading& colorGrading)
        {
            FinalCopy(cmd, sourceWidth, sourceHeight, sourceColor, targetWidth, targetHeight, target, 2.2f);
        }

        //---

    } // scene
} // rendering

