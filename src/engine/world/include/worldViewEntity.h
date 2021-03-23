/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "worldEntity.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// special entity that provides a viewpoint into the world
class ENGINE_WORLD_API IWorldViewEntity : public Entity
{
    RTTI_DECLARE_VIRTUAL_CLASS(IWorldViewEntity, Entity);

public:
    IWorldViewEntity();
    virtual ~IWorldViewEntity();

    //---

    virtual void evaluateCamera(CameraSetup& outSetup) const = 0;

    //---
};

//---

END_BOOMER_NAMESPACE()
