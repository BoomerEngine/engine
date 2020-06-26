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

#include "rendering/driver/include/renderingImageView.h"
#include "rendering/driver/include/renderingBufferView.h"
#include "rendering/driver/include/renderingImageView.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/scene/include/renderingFrameCamera.h"

namespace rendering
{
    namespace scene
    {

        //-- memory pools

        extern base::mem::PoolID POOL_RENDERING_FRAME;

        //--

    } // scene
} // rendering