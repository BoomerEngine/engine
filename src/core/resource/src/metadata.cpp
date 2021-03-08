/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\metadata #]
***/

#include "build.h"
#include "metadata.h"
#include "resource.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(ResourceConfiguration);
    RTTI_OLD_NAME("res::ResourceConfiguration");
    RTTI_CATEGORY("Author");
    RTTI_PROPERTY(m_sourceAuthor).editable("Name of the asset's author (if known)").overriddable().noReset();
    RTTI_PROPERTY(m_sourceLicense).editable("Any specific license information for the asset").overriddable().noReset();
    RTTI_CATEGORY("Source");
    RTTI_PROPERTY(m_sourceImportedBy).editable("Name of the person this asset was imported on").overriddable().noReset();
    RTTI_PROPERTY(m_sourceImportedAt).editable("Name of the computer this asset was imported on").overriddable().noReset();
    RTTI_PROPERTY(m_sourceImportedOn).editable("Time and date this asset was imported on").overriddable().readonly().noReset();
RTTI_END_TYPE();

ResourceConfiguration::~ResourceConfiguration()
{}

bool ResourceConfiguration::compare(const ResourceConfiguration* other) const
{
    if (!other || other->cls() != cls())
        return false;

    for (const auto* prop : cls()->allProperties())
    {
        const auto* thisData = prop->offsetPtr(this);
        const auto* otherData = prop->offsetPtr(other);
        if (!prop->type()->compare(thisData, otherData))
            return false;
    }

    return true;
}

void ResourceConfiguration::changeImportMetadata(StringView by, StringView at, TimeStamp time)
{
    if (m_sourceImportedBy != by)
    {
        m_sourceImportedBy = StringBuf(by);
        onPropertyChanged("sourceImportedBy");
    }

    if (m_sourceImportedAt != by)
    {
        m_sourceImportedAt = StringBuf(at);
        onPropertyChanged("sourceImportedAt");
    }

    if (m_sourceImportedOn != time)
    {
        m_sourceImportedOn = time;
        onPropertyChanged("sourceImportedOn");
    }
}

void ResourceConfiguration::setupDefaultImportMetadata()
{
    changeImportMetadata(GetUserName(), GetHostName(), TimeStamp::GetNow());
}

void ResourceConfiguration::computeConfigurationKey(CRC64& crc) const
{
    // ignore most of the "source" settings
    crc << m_sourceAuthor;
    crc << m_sourceLicense;
}

//--

RTTI_BEGIN_TYPE_STRUCT(ResourceImportDependency);
    RTTI_OLD_NAME("res::ImportDependency");
    RTTI_PROPERTY(importPath);
    RTTI_PROPERTY(timestamp);
    RTTI_PROPERTY(crc);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(ResourceMetadata);
    RTTI_OLD_NAME("res::Metadata");
    RTTI_PROPERTY(ids);
    RTTI_PROPERTY(importDependencies);
    RTTI_PROPERTY(resourceClassType);
    RTTI_PROPERTY(resourceClassVersion);
    RTTI_PROPERTY(importerClassType);
    RTTI_PROPERTY(importerClassVersion);
    RTTI_PROPERTY(importBaseConfiguration);
    RTTI_PROPERTY(importUserConfiguration);
    RTTI_PROPERTY(importFullConfiguration);
    RTTI_PROPERTY(internalRevision);
RTTI_END_TYPE();

ResourceMetadata::ResourceMetadata()
{}

//--

END_BOOMER_NAMESPACE()
