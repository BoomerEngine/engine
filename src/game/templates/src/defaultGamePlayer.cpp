/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "defaultGamePlayer.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(DefaultPlayer);
RTTI_END_TYPE();

DefaultPlayer::DefaultPlayer(IGamePlayerIdentity* identity, Vector3 spawnPosition, Angles spawnRotation)
    : IGamePlayerLocal(identity, nullptr)
    , m_spawnPosition(spawnPosition)
    , m_spawnRotation(spawnRotation)
{}

//--

END_BOOMER_NAMESPACE()
