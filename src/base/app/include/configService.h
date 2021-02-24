/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#pragma once

#include "localService.h"

#include "base/io/include/ioDirectoryWatcher.h"
#include "base/system/include/timing.h"

BEGIN_BOOMER_NAMESPACE(base::config)

//----

// config service - manages loading/saving configuration
class BASE_APP_API ConfigService : public app::ILocalService, public io::IDirectoryWatcherListener
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

    io::DirectoryWatcherPtr m_engineConfigWatcher;
    io::DirectoryWatcherPtr m_projectConfigWatcher;

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

    virtual void handleEvent(const io::DirectoryWatcherEvent& evt) override final;
};

//----

END_BOOMER_NAMESPACE()

typedef base::config::ConfigService ConfigService;