/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "staticTexture2D.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(StaticTexture2D);
    RTTI_OLD_NAME("StaticTexture");
    RTTI_METADATA(ResourceDescriptionMetadata).description("2D Texture");
RTTI_END_TYPE();

StaticTexture2D::StaticTexture2D()
{}

StaticTexture2D::StaticTexture2D(Setup&& setup)
    : IStaticTexture(std::move(setup))
{}

END_BOOMER_NAMESPACE()
