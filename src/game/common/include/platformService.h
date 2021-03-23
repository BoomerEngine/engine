/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "core/app/include/localService.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// Game platform service
class GAME_COMMON_API GamePlatformService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(GamePlatformService, IService);

public:
    GamePlatformService();
    virtual ~GamePlatformService();

    //--

    /// get the platform
    INLINE const GamePlatformPtr& platform() const { return m_platform; }

    //--

private:
    virtual bool onInitializeService(const CommandLine& cmdLine) override;
    virtual void onShutdownService() override;
    virtual void onSyncUpdate() override;

    GamePlatformPtr m_platform;
};

//--

END_BOOMER_NAMESPACE()
