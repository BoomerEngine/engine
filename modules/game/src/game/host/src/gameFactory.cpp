/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "game.h"
#include "gameFactory.h"

namespace game
{
    //--

    RTTI_BEGIN_TYPE_CLASS(GameInitData)
        RTTI_PROPERTY(spawnPositionOverride);
        RTTI_PROPERTY(spawnRotationOverride);
        RTTI_PROPERTY(spawnPositionOverrideEnabled);
        RTTI_PROPERTY(spawnRotationOverrideEnabled);
        RTTI_PROPERTY(worldPathOverride);
    RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGameFactory);
    RTTI_END_TYPE();

    IGameFactory::IGameFactory()
    {}

    IGameFactory::~IGameFactory()
    {}

    //--

} // game


