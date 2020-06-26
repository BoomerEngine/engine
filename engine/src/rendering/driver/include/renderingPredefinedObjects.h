/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\objects #]
***/

#pragma once

namespace rendering
{
    /// some commonly used stuff has predefined IDs in order to allow for much faster and simpler

    //static const uint8_t ID_BackBufferColor = 0x1; // last bound back buffer color (in the OpBeginFrame)
    //static const uint8_t ID_BackBufferDepth = 0x2; // last bound back buffer depth (in the OpBeginFrame)

    static const uint8_t ID_WhiteTexture = 0x10; // white RGBA8 16x16 texture
    static const uint8_t ID_BlackTexture = 0x11; // black RGBA8 16x16 texture
    static const uint8_t ID_GrayLinearTexture = 0x12; // linear gray RGBA8 16x16 texture
    static const uint8_t ID_GraySRGBTexture = 0x13; // sRGB gray RGBA8 16x16 texture
    static const uint8_t ID_NormZTexture = 0x14; // [0.5,0.5,1] RGBA8 16x16 texture (Z-up normal map)

    static const uint8_t ID_SamplerClampPoint = 0x20; // true point sampler
    static const uint8_t ID_SamplerClampBiLinear = 0x21; // bi-linear sampler with no mipmap filtering
    static const uint8_t ID_SamplerClampTriLinear = 0x22; // tri-linear sampler with linear mipmap filtering
    static const uint8_t ID_SamplerClampAniso = 0x23; // standard anisotropy sampler, limited by global renderer settings
    static const uint8_t ID_SamplerWrapPoint = 0x24; // true point sampler
    static const uint8_t ID_SamplerWrapBiLinear = 0x25; // bi-linear sampler with no mipmap filtering
    static const uint8_t ID_SamplerWrapTriLinear = 0x26; // tri-linear sampler with linear mipmap filtering
    static const uint8_t ID_SamplerWrapAniso = 0x27; // standard anisotropy sampler, limited by global renderer settings

    static const uint8_t ID_SamplerPointDepthLE = 0x30; // standard depth sampler with point filtering implementing LessEqual depth test
    static const uint8_t ID_SamplerPointDepthGE = 0x31; // standard depth sampler with point filtering implementing GreaterEqual depth test
    static const uint8_t ID_SamplerBiLinearDepthLE = 0x32; // standard depth sampler with point filtering implementing LessEqual depth test
    static const uint8_t ID_SamplerBiLinearDepthGE = 0x33; // standard depth sampler with point filtering implementing GreaterEqual depth test

} // rendering