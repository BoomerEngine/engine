/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_texture_glue.inl"

namespace rendering
{

    class ITexture;
    typedef base::RefPtr<ITexture> TexturePtr;
    typedef base::res::Ref<ITexture> TextureRef;

    class StaticTexture;
    typedef base::RefPtr<StaticTexture> StaticTexturePtr;
    typedef base::res::Ref<StaticTexture> StaticTextureRef;

} // rendering

