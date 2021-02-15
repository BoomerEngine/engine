/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#include "build.h"
#include "solidMeshEntity.h"

#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneObjects.h"

namespace game
{

    //--

    RTTI_BEGIN_TYPE_CLASS(SolidMeshEntity);
    RTTI_END_TYPE();

    SolidMeshEntity::SolidMeshEntity()
    {}

    SolidMeshEntity::~SolidMeshEntity()
    {}

    //--
        
} // rendering