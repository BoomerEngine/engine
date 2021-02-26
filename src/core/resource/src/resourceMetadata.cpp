/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\metadata #]
***/

#include "build.h"
#include "resourceMetadata.h"
#include "resource.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

//--

RTTI_BEGIN_TYPE_CLASS(ResourceConfiguration);
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

void ResourceConfiguration::changeImportMetadata(StringView by, StringView at, io::TimeStamp time)
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
    changeImportMetadata(GetUserName(), GetHostName(), io::TimeStamp::GetNow());
}

void ResourceConfiguration::computeConfigurationKey(CRC64& crc) const
{
    // ignore most of the "source" settings
    crc << m_sourceAuthor;
    crc << m_sourceLicense;
}

//--

RTTI_BEGIN_TYPE_STRUCT(SourceDependency);
    RTTI_PROPERTY(sourcePath);
    RTTI_PROPERTY(timestamp);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_STRUCT(ImportDependency);
    RTTI_PROPERTY(importPath);
    RTTI_PROPERTY(timestamp);
    RTTI_PROPERTY(crc);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_STRUCT(ImportFollowup);
    RTTI_PROPERTY(depotPath);
    RTTI_PROPERTY(sourceImportPath);
    RTTI_PROPERTY(configuration);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(Metadata);
    RTTI_PROPERTY(sourceDependencies);
    RTTI_PROPERTY(importDependencies);
    RTTI_PROPERTY(importFollowups);
    RTTI_PROPERTY(resourceClassVersion);
    RTTI_PROPERTY(importBaseConfiguration);
    RTTI_PROPERTY(importUserConfiguration);
    RTTI_PROPERTY(importFullConfiguration);
    RTTI_PROPERTY(internalRevision);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(res)
