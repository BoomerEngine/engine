/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#pragma once

#include "meshEntity.h"

BEGIN_BOOMER_NAMESPACE(game)

// mesh entity with physics collision
class GAME_WORLD_API SolidMeshEntity : public MeshEntity
{
    RTTI_DECLARE_VIRTUAL_CLASS(SolidMeshEntity, MeshEntity);

public:
    SolidMeshEntity();
    virtual ~SolidMeshEntity();

protected:
};

END_BOOMER_NAMESPACE(game)
