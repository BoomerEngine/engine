/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "projectSettings.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(ProjectSettingsProvideDefaultsMetadata);
RTTI_END_TYPE();

ProjectSettingsProvideDefaultsMetadata::ProjectSettingsProvideDefaultsMetadata()
{}

//---

RTTI_BEGIN_TYPE_CLASS(ProjectSettingsMultipleValuesMetadata);
RTTI_END_TYPE();

ProjectSettingsMultipleValuesMetadata::ProjectSettingsMultipleValuesMetadata()
{}

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IProjectSettings);
RTTI_END_TYPE();

IProjectSettings::IProjectSettings()
{}

IProjectSettings::~IProjectSettings()
{}

//---

END_BOOMER_NAMESPACE()