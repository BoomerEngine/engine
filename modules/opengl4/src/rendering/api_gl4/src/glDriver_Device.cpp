/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver #]
***/

#include "build.h"
#include "glDriver.h"

#if defined(PLATFORM_LINUX)
    #define _XTYPEDEF_BOOL
    #include <GL/gl.h>
    #include <GL/glx.h>
    #include <GL/glxext.h>
    #include <X11/Xatom.h>
    #include <X11/extensions/Xrender.h>
    #include <X11/Xutil.h>

#undef None
    #undef Always
    #undef bool
#elif defined(PLATFORM_WINAPI)
    #include <windows.h>
#endif

namespace rendering
{
    namespace gl4
    {

        void Driver::createPredefinedImages()
        {
            //--

            createPredefinedImageFromColor(ID_BlackTexture, base::Color(0, 0, 0, 0), ImageFormat::RGBA8_UNORM, "DefaultBlack");
            createPredefinedImageFromColor(ID_WhiteTexture, base::Color(255,255,255,255), ImageFormat::RGBA8_UNORM, "DefaultWhite");
            createPredefinedImageFromColor(ID_GrayLinearTexture, base::Color(127, 127, 127, 255), ImageFormat::RGBA8_UNORM, "DefaultLinearGray");
            createPredefinedImageFromColor(ID_GraySRGBTexture, base::Color(170, 170, 170, 255), ImageFormat::RGBA8_UNORM, "DefaultGammaGray");
            createPredefinedImageFromColor(ID_NormZTexture, base::Color(127, 127, 255, 255), ImageFormat::RGBA8_UNORM, "DefaultNormal");

            //--

            createPredefinedRenderTarget(ID_DefaultDepthRT, ImageFormat::D24S8, 1, "DefaultDepthRT");
            createPredefinedRenderTarget(ID_DefaultColorRT, ImageFormat::RGBA16F, 1, "DefaultColorRT");
            createPredefinedRenderTarget(ID_DefaultDepthArrayRT, ImageFormat::D24S8, 4, "DefaultDepthArrayRT");

            //--
        }

        void Driver::createPredefinedSamplers()
        {
            {
                SamplerState info;
                info.magFilter = FilterMode::Nearest;
                info.minFilter = FilterMode::Nearest;
                info.mipmapMode = MipmapFilterMode::Nearest;
                createPredefinedSampler(ID_SamplerClampPoint, info, "ClampPoint");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapPoint, info, "WrapPoint");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Nearest;
                createPredefinedSampler(ID_SamplerClampBiLinear, info, "ClampBilinear");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapBiLinear, info, "WrapBilinear");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Linear;
                createPredefinedSampler(ID_SamplerClampTriLinear, info, "ClampTrilinear");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapTriLinear, info, "WrapTrilinear");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Linear;
                info.maxAnisotropy = 16; // TODO: limit by driver settings
                createPredefinedSampler(ID_SamplerClampAniso, info, "ClampAniso");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapAniso, info, "WrapAniso");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Nearest;
                info.minFilter = FilterMode::Nearest;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::LessEqual;
                createPredefinedSampler(ID_SamplerPointDepthLE, info, "PointDepthLT");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Nearest;
                info.minFilter = FilterMode::Nearest;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::GreaterEqual;
                createPredefinedSampler(ID_SamplerPointDepthGE, info, "PointDepthGT");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::LessEqual;
                createPredefinedSampler(ID_SamplerBiLinearDepthLE, info, "BilinearDepthLT");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::GreaterEqual;
                createPredefinedSampler(ID_SamplerBiLinearDepthGE, info, "BilinearDepthGT");
            }
        }

    } // gl4
} // driver
