/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#include "build.h"
#include "entitySolidMesh.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(SolidMeshEntity);
    RTTI_CATEGORY("Collision");
    RTTI_PROPERTY(m_collisionEnabled).editable("Enable collision on this mesh").overriddable();
RTTI_END_TYPE();

SolidMeshEntity::SolidMeshEntity()
{}

SolidMeshEntity::~SolidMeshEntity()
{}

//--

END_BOOMER_NAMESPACE()
