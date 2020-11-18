/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "public.h"

#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)
    #define USE_RENDER_DOC
#endif

#include "rendering/device/include/renderingImageView.h"
#include "rendering/device/include/renderingBufferView.h"
#include "rendering/device/include/renderingImageView.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/scene/include/renderingFrameCamera.h"

namespace rendering
{
    namespace scene
    {


    } // scene
} // rendering