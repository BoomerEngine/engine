/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedDepot.h"
#include "managedFileNativeResource.h"
#include "managedFileFormat.h"
#include "resourceEditorNativeFile.h"

#include "core/resource/include/resourceFileLoader.h"
#include "core/resource/include/resourceMetadata.h"
#include "core/resource/include/resourceFileSaver.h"
#include "core/io/include/fileHandle.h"
#include "core/resource_compiler/include/importFileService.h"
#include "core/resource/include/resourceLoadingService.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedFileNativeResource);
RTTI_END_TYPE();

ManagedFileNativeResource::ManagedFileNativeResource(ManagedDepot* depot, ManagedDirectory* parentDir, StringView fileName)
    : ManagedFile(depot, parentDir, fileName)
    , m_fileEvents(this)
{
    m_resourceNativeClass = fileFormat().nativeResourceClass();
    DEBUG_CHECK_EX(m_resourceNativeClass, "No native resource class in native file");
}

ManagedFileNativeResource::~ManagedFileNativeResource()
{
}

res::ResourcePtr ManagedFileNativeResource::loadContent() const
{
    // cannot load content of deleted file
    DEBUG_CHECK_RETURN_EX_V(!isDeleted(), TempString("Cannot load content of deleted file '{}'", depotPath()), nullptr);

    // open depot file 
    if (auto file = depot()->depot().createFileAsyncReader(depotPath()))
    {
        res::FileLoadingContext context;
        context.resourceLoadPath = res::ResourcePath(depotPath());
        context.resourceLoader = GetService<res::LoadingService>()->loader();

        if (LoadFile(file, context))
            return context.root<res::IResource>();

        TRACE_ERROR("ManagedFile: Failed to load content of '{}'", depotPath());
    }
    else
    {
        TRACE_WARNING("ManagedFile: Can't load from deleted file '{}'", depotPath());
    }

    return nullptr;
}
    
res::MetadataPtr ManagedFileNativeResource::loadMetadata() const
{
    // cannot load content of deleted file
    DEBUG_CHECK_RETURN_EX_V(!isDeleted(), TempString("Cannot load content of deleted file '{}'", depotPath()), nullptr);

    if (auto file = depot()->depot().createFileAsyncReader(depotPath()))
    {
        res::FileLoadingContext loadingContext;
        loadingContext.loadSpecificClass = res::Metadata::GetStaticClass();

        if (res::LoadFile(file, loadingContext))
            return loadingContext.root<res::Metadata>();
    }

    return nullptr;
}

void ManagedFileNativeResource::discardContent()
{
    modify(false);
}

bool ManagedFileNativeResource::storeContent(const res::ResourcePtr& content)
{
    DEBUG_CHECK_RETURN_EX_V(!isDeleted(), TempString("Cannot save content to deleted file '{}'", depotPath()), false);
    DEBUG_CHECK_RETURN_EX_V(content, "Nothing to save", false);

    bool saved = false;

    if (auto writer = depot()->depot().createFileWriter(depotPath()))
    {
        res::FileSavingContext context;
        context.rootObject.pushBack(content);

        if (res::SaveFile(writer, context))
            saved = true;
        else
            writer->discardContent();
    }
    else
    {
        TRACE_WARNING("ManagedFile: Can't save into deleted file '{}'", depotPath());
    }

    // if file was saved change it's status
    if (saved)
    {
        modify(false);

        auto filePtr = ManagedFilePtr(AddRef(static_cast<ManagedFile*>(this)));
        DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_FILE_SAVED, filePtr);
        DispatchGlobalEvent(eventKey(), EVENT_MANAGED_FILE_SAVED);
    }

    return saved;
}

//--

bool ManagedFileNativeResource::canReimport() const
{
    // load the metadata for the file, if it's gone/not accessible file can't be reimported
    const auto metadata = loadMetadata();
    if (!metadata || metadata->importDependencies.empty())
        return false;

    // does source exist ?
    const auto& sourcePath = metadata->importDependencies[0].importPath;
    if (!GetService<res::ImportFileService>()->fileExists(sourcePath))
        return false;

    // reimporting in principle should be possible
    return true;
}

//--

END_BOOMER_NAMESPACE_EX(ed)

