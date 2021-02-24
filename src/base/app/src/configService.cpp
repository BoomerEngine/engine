/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "configService.h"
#include "configProperty.h"
#include "base/config/include/configStorage.h"
#include "base/config/include/configGroup.h"
#include "base/config/include/configEntry.h"
#include "base/containers/include/inplaceArray.h"
#include "base/app/include/commandline.h"
#include "base/io/include/ioDirectoryWatcher.h"
#include "base/io/include/ioSystem.h"

BEGIN_BOOMER_NAMESPACE(base::config)

RTTI_BEGIN_TYPE_CLASS(ConfigService);
RTTI_END_TYPE();

ConfigService::ConfigService()
    : m_hasValidBase(false)
{}

ConfigService::~ConfigService()
{}

app::ServiceInitializationResult ConfigService::onInitializeService( const app::CommandLine& cmdLine)
{
    /*m_engineConfigDir = base::io::SystemPath(io::PathCategory::).addDir("config");
    TRACE_INFO("Engine config directory: '{}'", m_engineConfigDir);

    m_engineConfigWatcher = base::io::CreateDirectoryWatcher(m_engineConfigDir);
    if (m_engineConfigWatcher)
        m_engineConfigWatcher->attachListener(this);*/

    /*auto& projectDir = base::io::SystemPath(io::PathCategory::);
    if (!projectDir.empty())
    {
        m_projectConfigDir = projectDir.addDir("config");
        TRACE_INFO("Project config directory: '{}'", m_engineConfigDir);

        m_projectConfigWatcher = base::io::CreateDirectoryWatcher(m_projectConfigDir);
        if (m_projectConfigWatcher)
            m_projectConfigWatcher->attachListener(this);
    }*/

    m_userConfigFile = TempString("{}user.ini", base::io::SystemPath(io::PathCategory::UserConfigDir));
    TRACE_INFO("User config file: '{}'", m_userConfigFile);

    if (!reloadConfig())
    {
        TRACE_ERROR("Failed to load base configuration for engine and project. Check your config files for errors.");
        return app::ServiceInitializationResult::FatalError;
    }

    if (cmdLine.hasParam("dumpConfig"))
        dumpConfig();

    return app::ServiceInitializationResult::Finished;
}

bool ConfigService::loadFileConfig(StringView path, config::Storage& outStorage) const
{
    if (base::io::FileExists(path))
    {
        StringBuf txt;
        if (!io::LoadFileToString(path, txt))
        {
            TRACE_ERROR("Failed to open config file '{}'", path);
            return false;
        }

        if (!config::Storage::Load(txt, outStorage))
        {
            TRACE_ERROR("Failed to parse config file '{}', see log for details", path);
            return false;
        }
    }
    else
    {
        TRACE_INFO("User configuration file does not exist (yet) and will not be loaded");
    }

    return true;
}

bool ConfigService::loadDirConfig(StringView path, config::Storage& outStorage) const
{
    bool ret = true;

    InplaceArray<StringBuf, 20> configPaths;
    base::io::FindFiles(path, "*.ini", configPaths, false);
    if (!configPaths.empty())
    {
        TRACE_INFO("Found {} config file(s) at '{}'", configPaths.size(), path);

        // make sure we are loading files in deterministic order
        std::sort(configPaths.begin(), configPaths.end());

        // load files
        for (auto& filePath : configPaths)
            ret &= loadFileConfig(filePath, outStorage);
    }

    return ret;
}

bool ConfigService::loadBaseConfig(config::Storage& outStorage) const
{
    bool ret = true;

    /*// load engine config
    ret &= loadDirConfig(m_engineConfigDir, outStorage);

    // load project config
    if (!m_projectConfigDir.empty())
        ret &= loadDirConfig(m_projectConfigDir, outStorage);*/

    return ret;
}

void ConfigService::requestSave()
{
    if (!m_saveTime)
        m_saveTime = NativeTimePoint::Now() + 1.0;
}

void ConfigService::requestReload()
{
    if (!m_reloadTime)
        m_reloadTime = NativeTimePoint::Now() + 0.5;
}

void ConfigService::dumpConfig()
{
    StringBuilder txt;
    ConfigPropertyBase::PrintAll(txt);

    const auto& configPath = base::io::SystemPath(io::PathCategory::UserConfigDir);
    io::SaveFileFromString(TempString("{}dump.ini", configPath), txt.toString());
}

bool ConfigService::reloadConfig()
{
    // if we have a valid base than save the user config first
    if (m_hasValidBase)
        saveUserConfig();

    // load a new base
    auto newBase = CreateUniquePtr<config::Storage>();
    if (!loadBaseConfig(*newBase))
    {
        TRACE_ERROR("Failed to load base configuration");
        return false;
    }

    // store as new base
    m_baseConfig = std::move(newBase);
    m_hasValidBase = true;

    // reset all stored values
    config::RawStorageData().clear();

    // load again into the config system
    loadBaseConfig(config::RawStorageData());

    // load the user config on top
    loadFileConfig(m_userConfigFile, config::RawStorageData());

    // apply the loaded configuration to config properties
    ConfigPropertyBase::PullAll();
    return true;
}

void ConfigService::saveUserConfig()
{
    ASSERT(m_hasValidBase);

    // reset the save timer
    m_saveTime.clear();

    // get the difference between base and
    StringBuilder txt;
    config::Storage::Save(txt, config::RawStorageData(), *m_baseConfig);

    // store the user config
    if (!io::SaveFileFromString(m_userConfigFile, txt.toString()))
    {
        TRACE_ERROR("Failed to save user config to '{}'", m_userConfigFile);
    }
}

void ConfigService::handleEvent(const io::DirectoryWatcherEvent& evt)
{
    if (evt.path.endsWith(".ini"))
    {
        TRACE_INFO("Config file '{}' changed", evt.path.c_str());
        requestReload();
    }
}

void ConfigService::onShutdownService()
{
    m_engineConfigWatcher.reset();
    m_projectConfigWatcher.reset();
}

void ConfigService::onSyncUpdate()
{
    if (m_reloadTime && m_reloadTime.reached())
    {
        m_reloadTime.clear();
        reloadConfig();
    }

    if (m_saveTime && m_saveTime.reached())
    {
        m_saveTime.clear();
        saveUserConfig();
    }
}

END_BOOMER_NAMESPACE(base::config)
