/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once
#include "core/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// single user sourced import job
struct CORE_RESOURCE_COMPILER_API ImportJobInfo
{
    // new ID to assign to resource
    ResourceID id;

    // file to import
    StringBuf assetFilePath;

    // reference resource path where we are saving stuff (.xdata)
    StringBuf depotFilePath;

    // resource class to import
    ResourceClass resourceClass;

    // base configuration
    ResourceConfigurationPtr baseConfig;

    // user given configuration or NULL if we want to keep existing one
    ResourceConfigurationPtr userConfig;

    // force import even if files are the same
    bool force = false;

    // should we followup with importing other resources ? (ie. import material for mesh, etc)
    bool recurse = true;
};

//--

/// followup import job
struct CORE_RESOURCE_COMPILER_API ImportJobFollowup
{
    // ID expected of the resource
    ResourceID id;

    // file to import
    StringBuf assetFilePath;

    // reference resource path where we are saving stuff (.xdata)
    StringBuf depotFilePath;

    // resource class to import
    ResourceClass resourceClass;

    // external import configuration (not the user one)
    ResourceConfigurationPtr externalConfig;
};

//--

/// general importer warning/error/info message that should be displayed to the user
struct CORE_RESOURCE_COMPILER_API ImportJobMessage
{
    logging::OutputLevel type = logging::OutputLevel::Info;
    StringBuf message; // message content
};

//--

/// result of a single job import
struct CORE_RESOURCE_COMPILER_API ImportJobResult
{
    // depot path
    StringBuf depotPath;

    // imported resource
    ResourcePtr resource;

    // assembled new resource metadata (can be based on one loaded from the disk)
    ResourceMetadataPtr metadata;

    // importer class used for the import
    SpecificClassType<IResourceImporter> importerClass;

    // follow up imports to perform
    Array<ImportJobFollowup> followupImports;

    // import log
    Array<ImportJobMessage> messages;

    // result flags
    ImportExtendedStatusFlags extendedStatusFlags;
};

//--

/// helper class that can import resources from source files
class CORE_RESOURCE_COMPILER_API Importer : public NoCopy
{
public:
    Importer(const IImportDepotLoader* customDepot = nullptr);
    ~Importer();

    //--

    /// compile an import stub

    /// check if given resource is up to date
    /// NOTE: we need the metadata loaded to check this
    void checkStatus(ImportExtendedStatusFlags& outStatusFlags, const ResourceMetadata* metadata, const ResourceConfiguration* newUserConfiguration = nullptr, IProgressTracker* progress = nullptr) const;

    /// import single resource, produces imported resource (with meta data)
    bool importResource(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress=nullptr) const;

    //--
    
    // find best importer class for importing given resource from given source file
    // e.g. Mesh from "fbx" -> FBXMeshImporter
    SpecificClassType<IResourceImporter> findImporterClass(StringView sourceAssetReferencePath, ResourceClass importedResourceClass) const;

    // find configuration class for importing from given source asset type 
    // e.g. Mesh from "fbx" -> FBXMeshImportConfig
    SpecificClassType<ResourceConfiguration> findConfigurationClass(StringView sourceAssetReferencePath, ResourceClass importedResourceClass) const;

    // given a source asset path and base and user config compile a final import configuration
    ResourceConfigurationPtr compileFinalImportConfiguration(StringView sourceAssetPath, ResourceClass importedResourceClass, const ResourceConfiguration* baseConfiguration=nullptr, const ResourceConfiguration* userConfiguration=nullptr) const;

    //--

private:
    struct ImportableClass
    {
        SpecificClassType<IResource> targetResourceClass = nullptr;
        SpecificClassType<IResourceImporter> importerClass = nullptr;
    };

    HashMap<StringBuf, Array<ImportableClass>> m_importableClassesPerExtension; // list of importable formats (per extension)

    void buildClassMap();

    bool findBestImporter(StringView assetFilePath, SpecificClassType<IResource>, SpecificClassType<IResourceImporter>& outImporterClass) const;


    //--

    const IImportDepotLoader* m_depot = nullptr;

    //--

    bool import_checkUpToDate(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress, bool& outUpToDate) const;
    bool import_assignID(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress) const;
    bool import_findImporter(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress) const;
    bool import_updateConfig(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress) const;
    bool import_doActualImport(const ImportJobInfo& info, ImportJobResult& outResult, IProgressTracker* progress) const;    
};

//--

END_BOOMER_NAMESPACE()

