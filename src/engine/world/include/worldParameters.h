/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "core/memory/include/structurePool.h"
#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// editable world parameters 
class ENGINE_WORLD_API IWorldParameters : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IWorldParameters, IObject);

public:
    virtual ~IWorldParameters();

    // render parameter specific fragments
    virtual void handleDebugRender(const World* world, rendering::FrameParams& params) const;
};

//---

END_BOOMER_NAMESPACE()
