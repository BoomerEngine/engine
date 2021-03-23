/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "game.h"
#include "settings.h"
#include "screenDebugMenu.h"
#include "screenLoadingScreen.h"
#include "screenSinglePlayerGame.h"

#include "engine/world/include/rawWorldData.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(GameProjectSettingScreens);
    RTTI_METADATA(ProjectSettingsProvideDefaultsMetadata);
    RTTI_CATEGORY("Main screens");
    RTTI_PROPERTY(m_initialScreen).editable("Initial screen to show (splash screen), can be used to play launch videos");
    RTTI_PROPERTY(m_startScreen).editable("Start screen, should show game title");
    RTTI_PROPERTY(m_mainMenuScreen).editable("Main menu screen");
    RTTI_PROPERTY(m_ingamePauseScreen).editable("In game pause menu");
    RTTI_CATEGORY("Game screens");
    RTTI_PROPERTY(m_singlePlayerGameScreen).editable("Screen to load custom world");
RTTI_END_TYPE();

GameProjectSettingScreens::GameProjectSettingScreens()
{
    if (!IsDefaultObjectCreation())
    {
        m_mainMenuScreen = GameScreenDebugMainMenu::GetStaticClass();
        m_loadingScreen = GameLoadingScreenSimple::GetStaticClass();
        m_ingamePauseScreen = GameScreenDebugInGamePause::GetStaticClass();

        m_singlePlayerGameScreen = RTTI::GetInstance().findClass("DefaultSinglePlayerGameScreen"_id).cast<IGameScreenSinglePlayer>();
    }
}

//--

RTTI_BEGIN_TYPE_CLASS(GameProjectSettingDebugWorlds);
    RTTI_METADATA(ProjectSettingsMultipleValuesMetadata);
    RTTI_PROPERTY(m_worldPaths);
    RTTI_PROPERTY(m_worlds).editable("World that should be loadable by the debug menu");
RTTI_END_TYPE();

GameProjectSettingDebugWorlds::GameProjectSettingDebugWorlds()
{}

//--

END_BOOMER_NAMESPACE()
