/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: app #]
***/

#pragma once

#include "localService.h"

BEGIN_BOOMER_NAMESPACE()

// service that encapsulates all loaded project settings
class CORE_APP_API ProjectSettingsService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(ProjectSettingsService, IService);

public:
    ProjectSettingsService();

    //--

    void loadSettings();

    //--

    /// path to project file
    INLINE const StringBuf& projectFilePath() const { return m_projectFilePath; }

    /// get the setting, returns the 
    template< typename T >
    INLINE const T* settings() const
    {
        static auto index = T::GetStaticClass()->userIndex();
        const auto& settings = m_settingsPerClass[index];
        return !settings.empty() ? static_cast<const T*>(settings.back()) : static_cast<const T*>(m_defaultSettingsPerClass[index].get());
    }

    /// get all settings for given class
    template< typename T >
    INLINE const Array<const T*>& allSetting() const
    {
        static auto index = T::GetStaticClass()->userIndex();
        return *(const Array<const T*>*) &m_settingsPerClass[index];
    }

    //--

protected:
    Array<ProjectSettingsPtr> m_settings;

    Array<Array<const IProjectSettings*>> m_settingsPerClass;
    Array<ProjectSettingsPtr> m_defaultSettingsPerClass;

    Array<SpecificClassType<IProjectSettings>> m_classList;

    StringBuf m_projectFilePath;

    //--

    virtual bool onInitializeService(const CommandLine& cmdLine) override;
    virtual void onShutdownService() override;
    virtual void onSyncUpdate() override;
};

//--

/// get the setting, returns the 
template< typename T >
INLINE const T* GetSettings()
{
    return GetService<ProjectSettingsService>()->settings<T>();
}

/// get all settings for given class
template< typename T >
INLINE const Array<const T*>& GetAllSettings()
{
    return GetService<ProjectSettingsService>()->allSetting<T>();
}

//--

END_BOOMER_NAMESPACE()
