/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_texture_glue.inl"

BEGIN_BOOMER_NAMESPACE()

class ITexture;
typedef RefPtr<ITexture> TexturePtr;
typedef res::Ref<ITexture> TextureRef;

class StaticTexture;
typedef RefPtr<StaticTexture> StaticTexturePtr;
typedef res::Ref<StaticTexture> StaticTextureRef;

END_BOOMER_NAMESPACE()

