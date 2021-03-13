/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\metadata #]
***/

#pragma once

#include "directTemplate.h"

#include "core/resource/include/resource.h"
#include "core/reflection/include/variantTable.h"
#include "core/io/include/timestamp.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// persistent resource "configuration" - carried over from one reimport to other reimport (all other data is lost)
class CORE_RESOURCE_API ResourceConfiguration : public IObjectDirectTemplate
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceConfiguration, IObjectDirectTemplate);

public:
    virtual ~ResourceConfiguration();

    //--

    INLINE const StringBuf& sourceAuthor() const { return m_sourceAuthor; };
    INLINE const StringBuf& sourceImportedBy() const { return m_sourceImportedBy; }
    INLINE const StringBuf& sourceLicense() const { return m_sourceLicense; };

    //--

    // compare with other configuration
    virtual bool compare(const ResourceConfiguration* other) const;

    //--

    // change the imported source settings
    void changeImportMetadata(StringView by, StringView at, TimeStamp time);

    // set the default "imported" settings - computer name, host, and time
    void setupDefaultImportMetadata();

protected:
    StringBuf m_sourceAuthor;
    StringBuf m_sourceLicense;

    StringBuf m_sourceImportedBy;
    StringBuf m_sourceImportedAt;
    TimeStamp m_sourceImportedOn;
};

///---

/// resource EXTERNAL (asset) dependency information
struct CORE_RESOURCE_API ResourceImportDependency
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ResourceImportDependency);

public:
    StringBuf importPath; // full absolute path (saved as UTF-8) to source import path
    TimeStamp timestamp; // timestamp of the file at the time of last import, used to filter out crc checks
    uint64_t crc = 0; // CRC of the source import file
};

///---

/// metadata of the resource, saved in .meta file alongside the .cache file
/// NOTE: metadata can contain information that can help to detect that file does not require full cooking
class CORE_RESOURCE_API ResourceMetadata : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceMetadata, IObject);

public:
    ResourceMetadata();

    // assigned resource ID
    Array<ResourceID> ids;

    // internal revision count - each time file is reimported this number is incremented
    uint32_t internalRevision = 0;

    // what is the type of the resource
    ResourceClass resourceClassType;

    // resource data version, if this does not match we need to recook as well
    uint32_t resourceClassVersion = 0;

    // importer class used to import the resource
    ClassType importerClassType; // IResourceImporter

    // version of the importer used to import the resource
    uint32_t importerClassVersion = 0;

    // custom load file extension (only for custom loaded types, the generic one is not stored)
    StringBuf loadExtension;

    //--

    // import dependencies of the file (for imported files only)
    Array<ResourceImportDependency> importDependencies;

    //--

    // Merged non-user import configuration that was used to import this asset, this is the config given by the follow-up import or automatic import
    // NOTE: this configuration can change when parent resource is reimported and is not directly editable by user
    ResourceConfigurationPtr importBaseConfiguration;

    // user specific (and editable) configuration for this resource, user has taken active part in setting import values for the resource
    ResourceConfigurationPtr importUserConfiguration;

    // merged final configuration that was actually used when importing resource, built in following order
    //  - source directory configuration (top to bottom)
    //  - base configuration
    //  - user configuration
    ResourceConfigurationPtr importFullConfiguration;

    //--

    static inline StringView FILE_EXTENSION = "xmeta";

    //--
};

///---

END_BOOMER_NAMESPACE()
