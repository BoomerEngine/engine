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
#include "base/io/include/utils.h"

namespace base
{
    namespace config
    {

        RTTI_BEGIN_TYPE_CLASS(ConfigService);
        RTTI_END_TYPE();

        ConfigService::ConfigService()
            : m_hasValidBase(false)
        {}

        ConfigService::~ConfigService()
        {}

        app::ServiceInitializationResult ConfigService::onInitializeService( const app::CommandLine& cmdLine)
        {
            /*m_engineConfigDir = IO::GetInstance().systemPath(io::PathCategory::).addDir("config");
            TRACE_INFO("Engine config directory: '{}'", m_engineConfigDir);

            m_engineConfigWatcher = IO::GetInstance().createDirectoryWatcher(m_engineConfigDir);
            if (m_engineConfigWatcher)
                m_engineConfigWatcher->attachListener(this);*/

            /*auto& projectDir = IO::GetInstance().systemPath(io::PathCategory::);
            if (!projectDir.empty())
            {
                m_projectConfigDir = projectDir.addDir("config");
                TRACE_INFO("Project config directory: '{}'", m_engineConfigDir);

                m_projectConfigWatcher = IO::GetInstance().createDirectoryWatcher(m_projectConfigDir);
                if (m_projectConfigWatcher)
                    m_projectConfigWatcher->attachListener(this);
            }*/

            m_userConfigFile = IO::GetInstance().systemPath(io::PathCategory::UserConfigDir).addFile("user.ini");
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

        bool ConfigService::loadFileConfig(const io::AbsolutePath& path, config::Storage& outStorage) const
        {
            if (IO::GetInstance().fileExists(path))
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

        bool ConfigService::loadDirConfig(const io::AbsolutePath& path, config::Storage& outStorage) const
        {
            bool ret = true;

            InplaceArray<io::AbsolutePath, 20> configPaths;
            IO::GetInstance().findFiles(path, L"*.ini", configPaths, false);
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

            auto path = IO::GetInstance().systemPath(io::PathCategory::UserConfigDir).addFile("dump.ini");
            io::SaveFileFromString(path, txt.toString());
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
            Config::GetInstance().storage().clear();

            // load again into the config system
            loadBaseConfig(Config::GetInstance().storage());

            // load the user config on top
            loadFileConfig(m_userConfigFile, Config::GetInstance().storage());

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
            config::Storage::Save(txt, Config::GetInstance().storage(), *m_baseConfig);

            // store the user config
            if (!io::SaveFileFromString(m_userConfigFile, txt.toString()))
            {
                TRACE_ERROR("Failed to save user config to '{}'", m_userConfigFile);
            }
        }

        void ConfigService::handleEvent(const io::DirectoryWatcherEvent& evt)
        {
            if (evt.path.toString().endsWith(L".ini"))
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

    } // config
} // base
