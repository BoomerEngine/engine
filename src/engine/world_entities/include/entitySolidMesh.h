/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#pragma once

#include "entityMeshBase.h"

BEGIN_BOOMER_NAMESPACE()

// mesh entity with physics collision
class ENGINE_WORLD_ENTITIES_API SolidMeshEntity : public MeshEntity
{
    RTTI_DECLARE_VIRTUAL_CLASS(SolidMeshEntity, MeshEntity);

public:
    SolidMeshEntity();
    virtual ~SolidMeshEntity();

protected:
    bool m_collisionEnabled = true;
};

END_BOOMER_NAMESPACE()
