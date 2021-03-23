/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "projectSettings.h"
#include "projectSettingsFile.h"
#include "projectSettingsService.h"
#include "commandline.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(ProjectSettingsService);
RTTI_END_TYPE();

ProjectSettingsService::ProjectSettingsService()
{}

void ProjectSettingsService::loadSettings()
{
    if (auto doc = LoadObjectFromXMLFile<ProjectSettingsFile>(m_projectFilePath))
    {
        // copy settings
        m_settings = doc->settings();

        // reset the settings mapping
        m_settingsPerClass.clear();
        m_settingsPerClass.allocateWith(m_classList.size(), {});

        // assign settings to matching classes
        for (const auto& setting : m_settings)
        {
            setting->parent(nullptr);

            for (auto i : m_classList.indexRange())
            {
                auto cls = m_classList[i];
                if (setting->is(cls))
                    m_settingsPerClass[i].pushBack(setting);
            }
        }
    }
}

bool ProjectSettingsService::onInitializeService(const CommandLine& cmdLine)
{
    // enumerate ALL classes and index them
    RTTI::GetInstance().enumClasses(m_classList, nullptr, true, true);

    // create default objects
    {
        InplaceArray<SpecificClassType<IProjectSettings>, 100> projectSettingsClasses;
        RTTI::GetInstance().enumClasses(projectSettingsClasses);

        m_defaultSettingsPerClass.allocateWith(m_classList.size(), nullptr);

        for (auto cls : projectSettingsClasses)
        {
            if (cls->findMetadata<ProjectSettingsProvideDefaultsMetadata>())
            {
                auto defaultObject = cls->create<IProjectSettings>();

                for (auto i : m_classList.indexRange())
                {
                    auto cls = m_classList[i];
                    if (defaultObject->is(cls))
                        m_defaultSettingsPerClass[i] = defaultObject;
                }
            }
        }
    }

    // try to use the actual project file
    auto projectDir = cmdLine.singleValue("projectDir");
    if (!projectDir.empty())
    {
        const auto path = StringBuf(TempString("{}data/project.xml", projectDir));
        if (FileExists(path))
        {
            TRACE_INFO("Using actual project settings file at '{}'", path);
            m_projectFilePath = path;
        }
        else
        {
            TRACE_WARNING("Project directory is specified but there's no project settings file");
        }
    }

    // use the engine project file
    if (m_projectFilePath.empty())
    {
        const auto engineDir = SystemPath(PathCategory::EngineDir);
        m_projectFilePath = StringBuf(TempString("{}data/project.xml", engineDir));

        if (FileExists(m_projectFilePath))
        {
            TRACE_INFO("Using engine project settings file at '{}'", m_projectFilePath);
        }
        else
        {
            TRACE_WARNING("Engine project settings does not exist, creating one");

            auto tempFile = RefNew<ProjectSettingsFile>();
            SaveObjectToXMLFile(m_projectFilePath, tempFile);
        }
    }

    // load the config
    loadSettings();
    return true;
}

void ProjectSettingsService::onShutdownService()
{}

void ProjectSettingsService::onSyncUpdate()
{}

//---

END_BOOMER_NAMESPACE()