/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: app #]
***/

#pragma once

#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

// file (resource) with all project settings
// NOTE: not actual resource since we don't have the IResource here yet and we want to have DepotService configurable by project settings
class CORE_APP_API ProjectSettingsFile : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(ProjectSettingsFile, IObject);

public:
    ProjectSettingsFile();
    virtual ~ProjectSettingsFile();

    INLINE const Array<ProjectSettingsPtr>& settings() const { return m_settings; }

    void attachSettings(IProjectSettings* settings);
    void detachSettings(IProjectSettings* settings);
    
private:
    Array<ProjectSettingsPtr> m_settings;

    virtual void onPostLoad() override;
};

END_BOOMER_NAMESPACE()
