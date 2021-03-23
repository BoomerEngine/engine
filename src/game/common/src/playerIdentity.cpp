/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "playerIdentity.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGamePlayerIdentity);
RTTI_END_TYPE();

IGamePlayerIdentity::IGamePlayerIdentity()
{}

IGamePlayerIdentity::~IGamePlayerIdentity()
{}

//--

END_BOOMER_NAMESPACE()
