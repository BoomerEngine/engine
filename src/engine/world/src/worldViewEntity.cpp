/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#include "build.h"
#include "worldViewEntity.h"
#include "engine/rendering/include/cameraContext.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IWorldViewEntity);
RTTI_END_TYPE();

IWorldViewEntity::IWorldViewEntity()
{}

IWorldViewEntity::~IWorldViewEntity()
{}

//--

END_BOOMER_NAMESPACE()
