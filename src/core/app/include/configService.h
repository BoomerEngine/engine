/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

#include "localService.h"

#include "core/io/include/directoryWatcher.h"
#include "core/system/include/timing.h"

BEGIN_BOOMER_NAMESPACE_EX(config)

//----

// config service - manages loading/saving configuration
class CORE_APP_API ConfigService : public app::ILocalService, public IDirectoryWatcherListener
{
    RTTI_DECLARE_VIRTUAL_CLASS(ConfigService, app::ILocalService);

public:
    ConfigService();
    virtual ~ConfigService();

    ///--

    // request to save the user configuration
    // NOTE: configuration is NOT saved right away in case we made a change that will cause a crash
    void requestSave();

    // request config to be reloaded
    // NOTE: configuration is NOT reloaded right away in case we made a change that will cause a crash
    void requestReload();

    ///--

protected:
    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    StringBuf m_userConfigFile;

    DirectoryWatcherPtr m_engineConfigWatcher;
    DirectoryWatcherPtr m_projectConfigWatcher;

    UniquePtr<config::Storage> m_baseConfig;
    bool m_hasValidBase;

    NativeTimePoint m_reloadTime;
    NativeTimePoint m_saveTime;

    bool reloadConfig();
    void saveUserConfig();
    void dumpConfig();

    bool loadBaseConfig(config::Storage& outStorage) const;
    bool loadDirConfig(StringView path, config::Storage& outStorage) const;
    bool loadFileConfig(StringView path, config::Storage& outStorage) const;

    virtual void handleEvent(const DirectoryWatcherEvent& evt) override final;
};

//----

END_BOOMER_NAMESPACE_EX(app)

typedef boomer::config::ConfigService ConfigService;