/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
***/

#pragma once

#include "core/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// Uncooked scene
/// Layer is composed of a list of node templates
class ENGINE_WORLD_API RawScene : public IResource
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(RawScene, IResource);

public:
    RawScene();

private:
};

//---

END_BOOMER_NAMESPACE()
