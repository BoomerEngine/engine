/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "projectSettings.h"
#include "projectSettingsFile.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(ProjectSettingsFile);
    RTTI_PROPERTY(m_settings);
RTTI_END_TYPE();

ProjectSettingsFile::ProjectSettingsFile()
{}

ProjectSettingsFile::~ProjectSettingsFile()
{}

void ProjectSettingsFile::onPostLoad()
{
    TBaseClass::onPostLoad();

    m_settings.removeAll(nullptr); // purge deleted settings
}

void ProjectSettingsFile::attachSettings(IProjectSettings* settings)
{
    DEBUG_CHECK_RETURN_EX(settings, "Invalid settings");
    DEBUG_CHECK_RETURN_EX(!settings->parent(), "Settings already parented");
    DEBUG_CHECK_RETURN_EX(!m_settings.contains(settings), "Settings already used");

    m_settings.pushBack(AddRef(settings));
    settings->parent(this);
}

void ProjectSettingsFile::detachSettings(IProjectSettings* settings)
{
    DEBUG_CHECK_RETURN_EX(settings, "Invalid settings");
    DEBUG_CHECK_RETURN_EX(settings->parent() == this, "Settings not parented");
    DEBUG_CHECK_RETURN_EX(m_settings.contains(settings), "Settings not used");

    settings->parent(nullptr);
    m_settings.remove(settings);
}

//---

END_BOOMER_NAMESPACE()