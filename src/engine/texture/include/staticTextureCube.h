/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "staticTexture.h"

BEGIN_BOOMER_NAMESPACE()

/// static cubemap variant of the texture
class ENGINE_TEXTURE_API StaticTextureCube : public IStaticTexture
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureCube, IStaticTexture);

public:
    StaticTextureCube();
    StaticTextureCube(Setup&& setup);
};

//--

END_BOOMER_NAMESPACE()
