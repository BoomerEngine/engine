/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "platformLocal.h"
#include "platformService.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(GamePlatformService);
RTTI_END_TYPE();

GamePlatformService::GamePlatformService()
{}

GamePlatformService::~GamePlatformService()
{}

bool GamePlatformService::onInitializeService(const CommandLine& cmdLine)
{
    // TODO: steam initialization

    m_platform = RefNew<GamePlatformLocal>();
    return true;
}

void GamePlatformService::onShutdownService()
{
    // TODO: flush saving, etc

    m_platform.reset();
}

void GamePlatformService::onSyncUpdate()
{
    m_platform->update();
}

//--
    
END_BOOMER_NAMESPACE()