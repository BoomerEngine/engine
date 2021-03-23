/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
*
***/

#include "build.h"
#include "world.h"
#include "worldParameters.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IWorldParameters);
RTTI_END_TYPE();

IWorldParameters::~IWorldParameters()
{}

void IWorldParameters::handleDebugRender(const World* world, rendering::FrameParams& params) const
{}

//---

END_BOOMER_NAMESPACE()
