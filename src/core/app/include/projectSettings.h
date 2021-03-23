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

//--

// metadata for project setting that indicates that we should aways provide a default values, even if the data is not defined
class CORE_APP_API ProjectSettingsProvideDefaultsMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(ProjectSettingsProvideDefaultsMetadata, IMetadata);

public:
    ProjectSettingsProvideDefaultsMetadata();
};

//--

// metadata for project setting that indicates (to the editor) that we are allowed to have more than one setting of this kind
class CORE_APP_API ProjectSettingsMultipleValuesMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(ProjectSettingsMultipleValuesMetadata, IMetadata);

public:
    ProjectSettingsMultipleValuesMetadata();
};

//--

// base class for a project-global settings (can be anything that we want to set globally)
class CORE_APP_API IProjectSettings : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IProjectSettings, IObject);

public:
    IProjectSettings();
    virtual ~IProjectSettings();

    // should this configuration object be included in the cooked game
    virtual bool shouldDeploy() const { return false; }
};

//--

END_BOOMER_NAMESPACE()
