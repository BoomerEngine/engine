/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"
#include "editorService.h"
#include "managedFileFormat.h"
#include "managedFileNativeResource.h"
#include "resourceEditor.h"
#include "resourceEditorNativeFile.h"
#include "resourceEditorNativeImportAspect.h"
#include "assetFileImportWidget.h"

#include "base/ui/include/uiButton.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource_compiler/include/importFileService.h"
#include "base/resource_compiler/include/importInterface.h"
#include "base/resource_compiler/include/importSaveThread.h"
#include "base/resource_compiler/include/importQueue.h"
#include "base/resource_compiler/include/importFileService.h"
#include "base/resource_compiler/include/importSourceAssetRepository.h"
#include "base/resource/include/resourceLoadingService.h"

BEGIN_BOOMER_NAMESPACE(ed)

//---

RTTI_BEGIN_TYPE_CLASS(ResourceEditorNativeImportAspect);
RTTI_END_TYPE();

ResourceEditorNativeImportAspect::ResourceEditorNativeImportAspect()
{
}

bool ResourceEditorNativeImportAspect::initialize(ResourceEditor* editor)
{
    if (!TBaseClass::initialize(editor))
        return false;

    auto nativeEditor = rtti_cast<ResourceEditorNativeFile>(editor);
    if (!nativeEditor)
        return false;

    auto nativeFile = rtti_cast<ManagedFileNativeResource>(editor->file());
    if (!nativeFile)
        return false; // not a native file

    if (!nativeEditor->resource() || !nativeEditor->resource()->metadata())
        return false; // not imported

    if (!editor->features().test(ResourceEditorFeatureBit::Imported))
        return false; // we don't want the importing business on this file

    const auto metadata = nativeFile->loadMetadata();
    if (!metadata || metadata->importDependencies.empty())
        return false; // file was not imported

    // get source import extension
    const auto& rootSourceAssetPath = metadata->importDependencies[0].importPath;
    const auto rootSourceFileExtension = rootSourceAssetPath.stringAfterLast(".");

    // get required config class, we need the source path to know what we will be importing
    SpecificClassType<res::ResourceConfiguration> configClass;
    res::IResourceImporter::ListImportConfigurationForExtension(rootSourceFileExtension, nativeFile->resourceClass(), configClass);
    if (!configClass)
        return false; // import is not configurable :(

    // build the base import config from source asset directory
    auto baseConfig = GetService<res::ImportFileService>()->compileBaseResourceConfiguration(rootSourceAssetPath, configClass);

    // apply the ASSET SPECIFIC config from a followup-import
    DEBUG_CHECK_EX(metadata->importBaseConfiguration, "No base configuration stored in metadata, very strange");
    if (metadata->importBaseConfiguration && metadata->importBaseConfiguration->is(configClass))
    {
        metadata->importBaseConfiguration->rebase(baseConfig);
        metadata->importBaseConfiguration->parent(nullptr);
        baseConfig = metadata->importBaseConfiguration;
    }

    // use the loaded user configuration
    DEBUG_CHECK_EX(metadata->importUserConfiguration, "No user configuration stored in metadata, very strange");
    DEBUG_CHECK_EX(!metadata->importUserConfiguration || metadata->importUserConfiguration->cls() == configClass, "User configuration stored in metadata has different class that currenyl recommended one");
    if (metadata->importUserConfiguration && metadata->importUserConfiguration->cls() == configClass)
    {
        m_config = metadata->importUserConfiguration;
        metadata->importUserConfiguration->parent(nullptr);
    }
    else
    {
        m_config = configClass.create();
    }

    // rebase the config on the BASE config stuff
    m_config->rebase(baseConfig);

    //--

    {
        auto importPanel = base::RefNew<ui::DockPanel>("[img:import] Import", "ImportSettings");
        importPanel->layoutVertical();

        {
            auto importWidget = importPanel->createChild<AssetFileImportWidget>();
            importWidget->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            importWidget->bindFile(nativeFile, m_config);
            importWidget->bind(EVENT_RESOURCE_REIMPORT_WITH_CONFIG) = [this](res::ResourceConfigurationPtr config)
            {
                inplaceReimport();
            };
        }

        {
            auto properties = importPanel->createChild<ui::DataInspector>();
            properties->expand();
            properties->bindActionHistory(editor->actionHistory());
            properties->bindObject(m_config);
        }

        editor->dockLayout().right().attachPanel(importPanel, false);
    }

    //--

    return true;
}

void ResourceEditorNativeImportAspect::close()
{
    TBaseClass::close();
    m_config.reset();
}

//--

    //--

class AssetImportSingleOutput : public res::IImportOutput
{
public:
    AssetImportSingleOutput(const StringBuf& expectedPath)
        : m_expectedPath(expectedPath)
    {}

    virtual bool scheduleSave(const res::ResourcePtr& data, const StringBuf& depotPath) override
    {
        if (m_expectedPath == depotPath)
            m_resource = data;
        return true;
    }

    INLINE res::ResourcePtr result() const { return m_resource; }

private:
    res::ResourcePtr m_resource;
    StringBuf m_expectedPath;
};

//--

class AssetImportSingleDepotFileLoader : public res::IImportDepotLoader
{
public:
    AssetImportSingleDepotFileLoader(const StringBuf& depotPath, const res::ResourcePtr& existingResource)
        : m_depotPath(depotPath)
        , m_existingResource(existingResource)
    {}

    virtual res::MetadataPtr loadExistingMetadata(StringView depotPath) const override final
    {
        if (depotPath == m_depotPath && m_existingResource)
            return m_existingResource->metadata();

        return nullptr;
    }

    virtual res::ResourcePtr loadExistingResource(StringView depotPath) const override final
    {
        if (depotPath == m_depotPath)
            return m_existingResource;

        return nullptr;
    }

    virtual bool depotFileExists(StringView depotPath) const override final
    {
        io::TimeStamp timestamp;
        return GetService<DepotService>()->queryFileTimestamp(depotPath, timestamp);
    }

    virtual bool depotFindFile(StringView depotPath, StringView fileName, uint32_t maxDepth, StringBuf& outFoundFileDepotPath) const override final
    {
        return GetService<DepotService>()->findFile(depotPath, fileName, maxDepth, outFoundFileDepotPath);
    }

private:
    StringBuf m_depotPath;
    res::ResourcePtr m_existingResource;
};

//--

// status pass through
class LocalProgressTrackingImportQueueCallbacks : public res::IImportQueueCallbacks
{
public:
    LocalProgressTrackingImportQueueCallbacks(IProgressTracker& progress)
        : m_progress(progress)
    {}

    virtual void queueJobAdded(const res::ImportJobInfo& info) override {};
    virtual void queueJobStarted(StringView depotPath) override {};

    virtual void queueJobFinished(StringView depotPath, res::ImportStatus status, double timeTaken) override
    {
        m_finished = true;
        m_finalStatus = status;
    }

    virtual void queueJobProgressUpdate(StringView depotPath, uint64_t currentCount, uint64_t totalCount, StringView text) override
    {
        m_progress.reportProgress(currentCount, totalCount, text);
    }

    INLINE bool finished() const
    {
        return m_finished;
    }

    INLINE res::ImportStatus status() const
    {
        return m_finalStatus;
    }

private:
    IProgressTracker& m_progress;
    //--

    bool m_finished = false;
    res::ImportStatus m_finalStatus = res::ImportStatus::Processing;
};

//--

void ResourceEditorNativeImportAspect::inplaceReimport()
{
    inplaceReimportWorker(IProgressTracker::DevNull());
}

bool ResourceEditorNativeImportAspect::inplaceReimportWorker(IProgressTracker& progress)
{
    auto assetSource = GetService<res::ImportFileService>();
    DEBUG_CHECK_RETURN_EX_V(assetSource, "Missing source depot service", false);

    auto editorService = GetEditor();
    DEBUG_CHECK_RETURN_EX_V(editorService, "Missing editor service", false);

    auto nativeEditor = rtti_cast<ResourceEditorNativeFile>(editor());
    DEBUG_CHECK_RETURN_EX_V(nativeEditor, "Missing editor", false);

    auto loadedResource = nativeEditor->resource();
    DEBUG_CHECK_RETURN_EX_V(loadedResource, "No resource loaded", false);
    DEBUG_CHECK_RETURN_EX_V(loadedResource->metadata(), "Resource has no metadata", false);
    DEBUG_CHECK_RETURN_EX_V(!loadedResource->metadata()->importDependencies.empty(), "Resource has no source assets", false);

    // create the inplace output for single resource
    const auto depotPath = nativeEditor->file()->depotPath();
    AssetImportSingleOutput saver(depotPath);

    // create loader
    AssetImportSingleDepotFileLoader loader(depotPath, loadedResource);

    // create asset source cache
    res::SourceAssetRepository repository(assetSource);

    // create the import queue
    LocalProgressTrackingImportQueueCallbacks reporter(progress);
    res::ImportQueue queue(&repository, &loader, &saver, &reporter);

    // add the initial job to the queue
    res::ImportJobInfo info;
    info.assetFilePath = loadedResource->metadata()->importDependencies[0].importPath;
    info.depotFilePath = depotPath;
    info.userConfig = m_config;
    info.forceImport = true;
    info.followImports = false;
    // info.externalConfig <- sucked from existing metadata
    queue.scheduleJob(info);

    // process all the jobs
    while (queue.processNextJob(&progress))
    {
        // placeholder for optional work we may want to do BETWEEN JOBS
    }

    // finished ?
    DEBUG_CHECK_EX(reporter.finished(), "Expected importer to finish");

    // do we have valid resource ?
    if (reporter.status() != res::ImportStatus::NewAssetImported)
        return false;

    auto importedResource = saver.result();
    DEBUG_CHECK_RETURN_EX_V(importedResource, "Importer finished without errors but no resource was written", false);

    // make copy - needed to make sure we have detached objects
    // TODO: "ensure handles loaded" would be probably enough
    auto resourceLoader = GetService<res::LoadingService>()->loader();
    auto clonedImportedResource = rtti_cast<res::IResource>(CloneObject(importedResource, nullptr, resourceLoader));

    // apply
    nativeEditor->applyLocalReimport(clonedImportedResource);
    return true;
}

//--

END_BOOMER_NAMESPACE(ed)

