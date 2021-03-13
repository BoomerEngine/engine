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

/// 2D variant of the static texture
class ENGINE_TEXTURE_API StaticTexture2D : public IStaticTexture
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTexture2D, IStaticTexture);

public:
    StaticTexture2D();
    StaticTexture2D(Setup&& setup);
};

//--

END_BOOMER_NAMESPACE()
