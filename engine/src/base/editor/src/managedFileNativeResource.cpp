/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedFileNativeResource.h"
#include "managedFileFormat.h"
#include "resourceEditorNativeFile.h"

#include "base/resource/include/resourceFileLoader.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource/include/resourceFileSaver.h"
#include "base/io/include/ioFileHandle.h"
#include "base/resource_compiler/include/importFileService.h"
#include "base/resource/include/resourceLoadingService.h"

namespace ed
{

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

    void ManagedFileNativeResource::discardContent()
    {
        if (isModified())
            modify(false);
        m_loadedResource.reset();
        m_modifiedResource.reset();
    }

    void ManagedFileNativeResource::onResourceReloadFinished(base::res::IResource* currentResource, base::res::IResource* newResource)
    {
        if (m_loadedResource == newResource)
        {
            if (auto nativeEditor = rtti_cast<ResourceEditorNativeFile>(editor()))
                nativeEditor->bindResource(AddRef(newResource));
        }
    }

    bool ManagedFileNativeResource::onResourceReloading(base::res::IResource* currentResource, base::res::IResource* newResource)
    {
        if (m_loadedResource == currentResource)
        {
            if (isModified())
                modify(false);

            m_loadedResource = newResource;
            m_modifiedResource.reset();

            m_fileEvents.clear();
            m_fileEvents.bind(newResource->eventKey(), EVENT_RESOURCE_MODIFIED) = [this]() {
                modify(true);
            };

            return true;
        }

        return TBaseClass::onResourceReloading(currentResource, newResource);
    }

    bool ManagedFileNativeResource::inUse() const
    {
        if (m_loadedResource.lock())
            return true;

        auto loadingService = GetService<base::res::LoadingService>();
        if (!loadingService)
            return false;

        base::res::ResourcePtr loadedResource;
        return loadingService->acquireLoadedResource(MakePath(depotPath(), m_resourceNativeClass), loadedResource);
    }

    void ManagedFileNativeResource::modify(bool flag)
    {
        if (flag != isModified())
        {
            if (flag)
            {
                DEBUG_CHECK_EX(!m_modifiedResource, "File already marked as modified");
                DEBUG_CHECK_EX(!m_loadedResource.empty(), "Trying to modify resource that was never loaded");

                auto modified = m_loadedResource.lock();
                DEBUG_CHECK_EX(modified, "Trying to modify resource that was unloaded");

                if (modified)
                {
                    m_modifiedResource = modified;
                    ManagedFile::modify(true);
                }
            }
            else
            {
                DEBUG_CHECK_EX(m_modifiedResource, "File was not modified");
                m_modifiedResource.reset();

                ManagedFile::modify(false);
            }
        }
    }

    res::ResourcePtr ManagedFileNativeResource::loadContent()
    {
        // use existing resource
        if (auto loaded = m_loadedResource.lock())
            return loaded;

        // we can't be deleted
        DEBUG_CHECK_EX(!isDeleted(), "Trying to load contet from deleted file");
        if (!isDeleted())
        {
            // cleanup any currently bound resource
            DEBUG_CHECK_EX(!isModified(), "File is marked as modified yet it failed to reuse data");
            DEBUG_CHECK_EX(!m_modifiedResource, "File has modified data yet it failed to reuse it");
            discardContent();

            // load as a normal resource from repository
            if (const auto loaded = LoadResource(MakePath(depotPath(), m_resourceNativeClass)).acquire())
            {
                // wait for the "modified" event in the resource
                m_fileEvents.clear();
                m_fileEvents.bind(loaded->eventKey(), EVENT_RESOURCE_MODIFIED) = [this]() {
                    modify(true);
                };

                // keep around
                m_loadedResource = loaded;
                return loaded;
            }

            // fail
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
        if (!isDeleted())
        {
            if (auto file = depot()->depot().createFileAsyncReader(depotPath()))
            {
                res::FileLoadingContext loadingContext;
                loadingContext.loadSpecificClass = res::Metadata::GetStaticClass();

                if (res::LoadFile(file, loadingContext))
                    return loadingContext.root<res::Metadata>();
            }
        }

        return nullptr;
    }

    bool ManagedFileNativeResource::storeContent()
    {
        bool saved = false;

        // we can only save the bound resource
        DEBUG_CHECK_EX(!isDeleted(), "Trying to save contet to deleted file");
        if (!isDeleted())
        {
            if (auto loaded = m_loadedResource.lock())
            {
                res::ResourceMountPoint mountPoint;
                if (depot()->depot().queryFileMountPoint(depotPath(), mountPoint))
                {
                    if (auto writer = depot()->depot().createFileWriter(depotPath()))
                    {
                        res::FileSavingContext context;
                        context.basePath = mountPoint.path();
                        context.rootObject.pushBack(loaded);

                        if (res::SaveFile(writer, context))
                            saved = true;
                        else
                            writer->discardContent();
                    }
                }
            }
            // nothing loaded, saving is not possible
            else
            {
                DEBUG_CHECK_EX(!isModified(), "File is marked as modified yet it failed to reuse data");
                DEBUG_CHECK_EX(!m_modifiedResource, "File has modified data yet it failed to reuse it");
            }
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

} // depot

