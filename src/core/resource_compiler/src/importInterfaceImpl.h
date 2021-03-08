/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

#include "fingerprint.h"
#include "core/io/include/timestamp.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// asset import interface - implementation
class CORE_RESOURCE_COMPILER_API LocalImporterInterface : public IResourceImporterInterface
{
public:
    LocalImporterInterface(const IImportDepotLoader* depot, const StringBuf& importPath, const StringBuf& depotPath, IProgressTracker* externalProgressTracker, const ResourceConfiguration* importConfiguration);
    virtual ~LocalImporterInterface();

    /// IResourceImporterInterface
    virtual const IResource* existingData() const override final;
    virtual const StringBuf& queryResourcePath() const override final;
    virtual const StringBuf& queryImportPath() const override final;
    virtual const ResourceConfiguration* queryConfigrationTypeless() const override final;
    virtual ResourceID queryResourceID(StringView depotPath) const override final;

    virtual Buffer loadSourceFileContent(StringView assetImportPath, bool reportAsDependency/*= true*/) override final;
    virtual SourceAssetPtr loadSourceAsset(StringView assetImportPath, bool reportAsDependency/*= true*/, bool allowCaching /*= true*/) override final;

    virtual bool findSourceFile(StringView assetImportPath, StringView inputPath, StringBuf& outImportPath, uint32_t maxScanDepth = 2) const override final;
    virtual bool findDepotFile(StringView depotReferencePath, StringView depotSearchPath, StringView searchFileName, StringBuf& outDepotPath, ResourceID* outID = nullptr, uint32_t maxScanDepth = 2) const override final;
    virtual bool checkDepotFile(StringView depotPath) const override final;
    virtual bool checkSourceFile(StringView assetImportPath) const override final;

    virtual ResourceID followupImport(StringView assetImportPath, StringView depotPath, ResourceClass cls, const ResourceConfiguration* config = nullptr)  override final;

    // IProgressTracker
    virtual bool checkCancelation() const override final;
    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final;

    //--

    struct FollowupImport
    {
        StringBuf assetPath;
        StringBuf depotPath;
        ResourceID id;
        ResourceClass resourceClass;
        ResourceConfigurationPtr config;
    };

    INLINE const Array<FollowupImport>& followupImports() const { return m_followupImports; }

    //--

    struct Dependency
    {
        StringBuf assetPath;
        TimeStamp timestamp;
        SourceAssetFingerprint fingerprint;
    };

    INLINE const Array<Dependency>& dependencies() const { return m_dependencies; }

    //--

private:
    const IImportDepotLoader* m_depot = nullptr;

    StringBuf m_importPath;
    StringBuf m_depotPath;
            
    IProgressTracker* m_externalProgressTracker = nullptr;

    ResourceConfigurationPtr m_configuration; // merged import configuration

    //--

    SpinLock m_loadedOriginalDataLock;
    mutable ResourcePtr m_loadedOriginalData;

    //--

    
    SpinLock m_dependenciesLock;
    Array<Dependency> m_dependencies;
    HashSet<StringBuf> m_dependenciesSet;

    //--

    SpinLock m_followupImportsLock;
    Array<FollowupImport> m_followupImports;
    HashMap<StringBuf, ResourceID> m_followupImportsSet;

    //--

    void reportImportDependency(StringView assetImportPath, const TimeStamp& timestamp, const SourceAssetFingerprint& fingerprint);
};

//--

END_BOOMER_NAMESPACE()
