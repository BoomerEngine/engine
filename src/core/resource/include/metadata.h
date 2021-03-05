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

    // compute the hash from configuration parameters
    // NOTE: hash is never stored but instead used to compare configurations when detecting if resource should be reimproted or not
    virtual void computeConfigurationKey(CRC64& crc) const;

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

/// resource source dependency information
/// contains information of a file used to import given resource
struct CORE_RESOURCE_API ResourceSourceDependency
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ResourceSourceDependency);

public:
    StringBuf sourcePath; // depot file used during resource cooking
    uint64_t timestamp = 0; // timestamp of the file at the time of cooking, if still valid files are assumed to be ok
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

/// List of files that have to be imported as well for this resource to work fine
/// usually mesh requires materials, materials require textures
/// This list is used so even when the original file was up to date we will know what other files we need to check
struct CORE_RESOURCE_API ResourceImportFollowup
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ResourceImportFollowup);

    StringBuf depotPath; // where to import, extension encodes type
    StringBuf sourceImportPath; // from where to import
    ResourceConfigurationPtr configuration; // explicit configuration entries, not used if resource already exists
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

    // resource data version, if this does not match we need to recook as well
    uint32_t resourceClassVersion = 0;

    // internal revision count - each time file is reimported this number is incremented
    uint32_t internalRevision = 0;

    // source dependencies of the file
    // dependency 0 is the root file
    Array<ResourceSourceDependency> sourceDependencies;

    // import dependencies of the file (for imported files only)
    Array<ResourceImportDependency> importDependencies;

    // other assets that were imported due to import of this file
    Array<ResourceImportFollowup> importFollowups;

    // Merged non-user import configuration that was used to import this asset, this is the config given by the follow-up import or automatic import
    // NOTE: this configuration can change without user input - ie. the folder configuration was changed, etc
    ResourceConfigurationPtr importBaseConfiguration;

    // user specific configuration for this resource, user has taken active part in setting import values for the resource
    // NOTE: rebase the user on base to get full set of values to use
    ResourceConfigurationPtr importUserConfiguration;

    // final FULL merged configuration (not based on ANYTHING)
    // this is what we used for importing, can be used to compare if we need to reimport stuff because of changed settings
    ResourceConfigurationPtr importFullConfiguration;

    //--
};

///---

END_BOOMER_NAMESPACE()
