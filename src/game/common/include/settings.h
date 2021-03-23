/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "core/app/include/projectSettings.h"

BEGIN_BOOMER_NAMESPACE()

//--

// project entry screen, if not specified the debug menu is used
class GAME_COMMON_API GameProjectSettingScreens : public IProjectSettings
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameProjectSettingScreens, IProjectSettings);

public:
    GameProjectSettingScreens();

    SpecificClassType<IGameScreen> m_initialScreen;
    SpecificClassType<IGameScreen> m_startScreen;
    SpecificClassType<IGameScreen> m_mainMenuScreen;
    SpecificClassType<IGameScreen> m_ingamePauseScreen;

    SpecificClassType<IGameScreenSinglePlayer> m_singlePlayerGameScreen;

    SpecificClassType<IGameLoadingScreen> m_loadingScreen;
};

//--

// list of debug worlds for the debug load menu
class GAME_COMMON_API GameProjectSettingDebugWorlds : public IProjectSettings
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameProjectSettingDebugWorlds, IProjectSettings);

public:
    GameProjectSettingDebugWorlds();

    Array<ResourceAsyncRef<RawWorldData>> m_worlds;
    Array<StringBuf> m_worldPaths;
};

//--

END_BOOMER_NAMESPACE()
