/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "staticTextureCube.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(StaticTextureCube);
    RTTI_METADATA(ResourceDescriptionMetadata).description("Cube Texture");
RTTI_END_TYPE();

StaticTextureCube::StaticTextureCube()
{}

StaticTextureCube::StaticTextureCube(Setup&& setup)
    : IStaticTexture(std::move(setup))
{}

END_BOOMER_NAMESPACE()
