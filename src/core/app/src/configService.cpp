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
#include "core/config/include/storage.h"
#include "core/config/include/group.h"
#include "core/config/include/entry.h"
#include "core/containers/include/inplaceArray.h"
#include "core/app/include/commandline.h"
#include "core/io/include/directoryWatcher.h"
#include "core/io/include/io.h"

BEGIN_BOOMER_NAMESPACE()

RTTI_BEGIN_TYPE_CLASS(ConfigService);
RTTI_END_TYPE();

ConfigService::ConfigService()
    : m_hasValidBase(false)
{}

ConfigService::~ConfigService()
{}

bool ConfigService::onInitializeService(const CommandLine& cmdLine)
{
    /*m_engineConfigDir = SystemPath(PathCategory::).addDir("config");
    TRACE_INFO("Engine config directory: '{}'", m_engineConfigDir);

    m_engineConfigWatcher = CreateDirectoryWatcher(m_engineConfigDir);
    if (m_engineConfigWatcher)
        m_engineConfigWatcher->attachListener(this);*/

    /*auto& projectDir = SystemPath(PathCategory::);
    if (!projectDir.empty())
    {
        m_projectConfigDir = projectDir.addDir("config");
        TRACE_INFO("Project config directory: '{}'", m_engineConfigDir);

        m_projectConfigWatcher = CreateDirectoryWatcher(m_projectConfigDir);
        if (m_projectConfigWatcher)
            m_projectConfigWatcher->attachListener(this);
    }*/

    m_userConfigFile = TempString("{}user.ini", SystemPath(PathCategory::UserConfigDir));
    TRACE_INFO("User config file: '{}'", m_userConfigFile);

    if (!reloadConfig())
    {
        TRACE_ERROR("Failed to load base configuration for engine and project. Check your config files for errors.");
        return false;
    }

    if (cmdLine.hasParam("dumpConfig"))
        dumpConfig();

    return true;
}

bool ConfigService::loadFileConfig(StringView path, ConfigStorage& outStorage) const
{
    if (FileExists(path))
    {
        StringBuf txt;
        if (!LoadFileToString(path, txt))
        {
            TRACE_ERROR("Failed to open config file '{}'", path);
            return false;
        }

        if (!ConfigStorage::Load(txt, outStorage))
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

bool ConfigService::loadDirConfig(StringView path, ConfigStorage& outStorage) const
{
    bool ret = true;

    InplaceArray<StringBuf, 20> configPaths;
    FindFiles(path, "*.ini", configPaths, false);
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

bool ConfigService::loadBaseConfig(ConfigStorage& outStorage) const
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

    const auto& configPath = SystemPath(PathCategory::UserConfigDir);
    SaveFileFromString(TempString("{}dump.ini", configPath), txt.toString());
}

bool ConfigService::reloadConfig()
{
    // if we have a valid base than save the user config first
    if (m_hasValidBase)
        saveUserConfig();

    // load a new base
    auto newBase = CreateUniquePtr<ConfigStorage>();
    if (!loadBaseConfig(*newBase))
    {
        TRACE_ERROR("Failed to load base configuration");
        return false;
    }

    // store as new base
    m_baseConfig = std::move(newBase);
    m_hasValidBase = true;

    // reset all stored values
    ConfigRawStorageData().clear();

    // load again into the config system
    loadBaseConfig(ConfigRawStorageData());

    // load the user config on top
    loadFileConfig(m_userConfigFile, ConfigRawStorageData());

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
    ConfigStorage::Save(txt, ConfigRawStorageData(), *m_baseConfig);

    // store the user config
    if (!SaveFileFromString(m_userConfigFile, txt.toString()))
    {
        TRACE_ERROR("Failed to save user config to '{}'", m_userConfigFile);
    }
}

void ConfigService::handleEvent(const DirectoryWatcherEvent& evt)
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

END_BOOMER_NAMESPACE()
