/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "game/common/include/player.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// a simple player for default game
class GAME_TEMPLATES_API DefaultPlayer : public IGamePlayerLocal
{
    RTTI_DECLARE_VIRTUAL_CLASS(DefaultPlayer, IGamePlayerLocal);

public:
    DefaultPlayer(IGamePlayerIdentity* identity, Vector3 spawnPosition, Angles spawnRotation);

    Vector3 m_spawnPosition;
    Angles m_spawnRotation;
};

//--

END_BOOMER_NAMESPACE()
