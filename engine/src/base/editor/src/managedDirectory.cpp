/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "editorService.h"
#include "managedDirectory.h"
#include "managedFile.h"
#include "managedDepot.h"

#include "base/containers/include/clipboard.h"
#include "base/containers/include/inplaceArray.h"
#include "base/io/include/ioSystem.h"
#include "base/system/include/thread.h"
#include "base/image/include/image.h"
#include "managedFileFormat.h"
#include "base/io/include/ioFileHandle.h"
#include "base/resource/include/resourceFileSaver.h"
#include "managedFileNativeResource.h"
#include "managedFileRawResource.h"
#include "managedItemCollection.h"

namespace ed
{
    //--

    static res::StaticResource<image::Image> resDirectoryTexture("/engine/thumbnails/directory.png", true);

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedDirectory);
    RTTI_END_TYPE();

    ManagedDirectory::ManagedDirectory(ManagedDepot* dep, ManagedDirectory* parentDirectory, StringView name, StringView depotPath)
        : ManagedItem(dep, parentDirectory, name)
        , m_depotPath(depotPath)
        , m_fileCount(0)
    {
        m_directoryIcon = resDirectoryTexture.loadAndGetAsRef();
    }

    ManagedDirectory::~ManagedDirectory()
    {}

    const image::ImageRef& ManagedDirectory::typeThumbnail() const
    {
        return m_directoryIcon;
    }

    void ManagedDirectory::collect(const std::function<bool(const ManagedFile*)>& filter, Array<ManagedFile*>& outFiles) const
    {
        // collect files
        for (auto* ptr : m_files.list())
            if (filter(ptr))
                outFiles.pushBack(ptr);

        // recurse
        for (auto childDirectory : m_directories.list())
            childDirectory->collect(filter, outFiles);
    }

    void ManagedDirectory::bookmark(bool state)
    {
        if (m_bookmarked != state)
        {
            m_bookmarked = state;
            depot()->toogleDirectoryBookmark(this, state);
        }
    }

    void ManagedDirectory::deleted(bool flag)
    {
        if (flag != isDeleted())
        {
            m_isDeleted = flag;

            if (flag)
            {
                DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_DELETED, ManagedDirectoryPtr(AddRef(this)));
                DispatchGlobalEvent(eventKey(), EVENT_MANAGED_DIRECTORY_DELETED);
            }
            else
            {
                DispatchGlobalEvent(eventKey(), EVENT_MANAGED_DIRECTORY_CREATED);
                DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_CREATED, ManagedDirectoryPtr(AddRef(this)));
            }
        }
    }

    void ManagedDirectory::populate()
    {
        refreshContent();
    }

    static ManagedFilePtr CreateManagedFile(ManagedDepot* depot, ManagedDirectory* dir, StringView name)
    {
        const auto fileExt = name.afterFirst(".");
        if (fileExt)
        {
            if (const auto format = ManagedFileFormatRegistry::GetInstance().format(fileExt))
            {
                if (format->nativeResourceClass())
                    return RefNew<ManagedFileNativeResource>(depot, dir, name);
                else
                    return RefNew<ManagedFileRawResource>(depot, dir, name);
            }
        }

        return nullptr;
    }

    void ManagedDirectory::refreshContent()
    {
        PC_SCOPE_LVL0(RefreshContent);

        // create files
        {
            HashSet<ManagedFile*> visitedFiles;
            depot()->depot().enumFilesAtPath(depotPath(), [this, &visitedFiles](const depot::DepotStructure::FileInfo& info)
                {
                    if (auto existingFile = file(info.name, true))
                    {
                        existingFile->deleted(false);
                        visitedFiles.insert(existingFile);
                    }
                    else
                    {
                        if (auto fileWrapper = CreateManagedFile(depot(), this, info.name))
                        {
                            m_files.add(fileWrapper);
                            visitedFiles.insert(fileWrapper);

                            DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_FILE_CREATED, fileWrapper);
                        }
                    }

                    return false;
                });

            for (auto& file : m_files.list())
                if (!visitedFiles.contains(file))
                    file->deleted(true);
        }

        // create directories
        Array<ManagedDirectory*> newDirectories;
        {
            HashSet<ManagedDirectory*> visitedDirs;
            depot()->depot().enumDirectoriesAtPath(depotPath(), [this, &visitedDirs, &newDirectories](const depot::DepotStructure::DirectoryInfo& info)
                {
                    if (auto existingDir = directory(info.name, true))
                    {
                        existingDir->deleted(false);
                        visitedDirs.insert(existingDir);
                    }
                    else
                    {
                        if (auto dirWrapper = RefNew<ManagedDirectory>(depot(), this, info.name, TempString("{}{}/", depotPath(), info.name)))
                        {
                            m_directories.add(dirWrapper);
                            newDirectories.pushBack(dirWrapper);
                            visitedDirs.insert(dirWrapper);

                            DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_CREATED, dirWrapper);
                        }
                    }
                    return false;
                });

            for (auto& dir : m_directories.list())
                if (!visitedDirs.contains(dir))
                    dir->deleted(true);
        }

        // scan children directories
        for (const auto& dir : newDirectories)
            dir->refreshContent();

        // finally, update local counts
        updateFileCount(false);
        updateModifiedContentCount(false);
    }

    ManagedDirectory* ManagedDirectory::directory(StringView dirName, bool allowDeleted /*= false*/) const
    {
        if (auto ret = m_directories.find(dirName))
            if (allowDeleted || !ret->isDeleted())
                return ret;

        return nullptr;
    }

    ManagedFile* ManagedDirectory::file(StringView fileName, bool allowDeleted /*= false*/) const
    {
        if (auto ret = m_files.find(fileName))
            if (allowDeleted || !ret->isDeleted())
                return ret;
        return nullptr;
    }

    ManagedDirectory* ManagedDirectory::createDirectory(StringView name)
    {
        // return existing sub-directory
        auto curDir = directory(name, true);
        if (curDir)
        {
            // "undelete" deleted directories
            if (curDir->isDeleted())
            {
                curDir->deleted(false);

                base::io::CreatePath(TempString("{}name/", absolutePath()));

                curDir->populate(); // just in case it was not really deleted
            }
            
            return curDir;
        }

        // validate file name
        if (!ValidateDirectoryName(name))
        {
            TRACE_ERROR("Failed to create directory '{}' in '{}': invalid directory name", name, depotPath());
            return nullptr;
        }

        // no path
        if (absolutePath().empty())
        {
            TRACE_ERROR("Failed to create directory '{}' in '{}': parent directory has no physical locataion", name, depotPath());
            return nullptr;
        }

        // create the physical path in file system
        if (!base::io::CreatePath(TempString("{}name/", absolutePath(), name)))
        {
            TRACE_ERROR("Failed to create directory '{}' in '{}': failed to create physical directory on disk", name, depotPath());
            return nullptr;
        }

        // create wrapper
        auto newDir = RefNew<ManagedDirectory>(depot(), this, StringBuf(name), TempString("{}{}/", m_depotPath, name));
        m_directories.add(newDir);
        curDir = newDir;

        // fill new directory with content, usually such directories are empty
        curDir->populate();

        // refresh file count
        updateFileCount();

        // report events
        DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_CREATED, ManagedDirectoryPtr(AddRef(curDir)));
        return curDir;
    }

    StringBuf ManagedDirectory::adjustFileName(StringView fileName) const
    {
        HashSet<StringBuf> names;
        for (const auto& file : m_files.list())
            if (!file->isDeleted())
                names.insert(file->name().toLower());

        for (const auto& dir : m_directories.list())
            if (!dir->isDeleted())
                names.insert(dir->name().toLower());

        if (!names.contains(fileName))
            return StringBuf(fileName);

        uint32_t startCounter = 0;
        auto coreName = fileName.beforeFirstOrFull(".").trimTailNumbers(&startCounter);
        auto extPart = fileName.afterFirst(".");

        if (!names.contains(TempString("{}.{}", coreName, extPart).c_str()))
            return StringBuf(TempString("{}.{}", coreName, extPart));

        for (;;)
        {
            startCounter += 1;
            if (!names.contains(TempString("{}{}.{}", coreName, startCounter, extPart).c_str()))
                break;
        }

        return StringBuf(TempString("{}{}.{}", coreName, startCounter, extPart));
    }

    ManagedFile* ManagedDirectory::createFile(StringView fileName, const res::ResourceHandle& initialContent)
    {
        // no name
        DEBUG_CHECK_EX(fileName, "File name should be provided");
        if (!fileName)
            return nullptr;

        // validate file name
        if (!ValidateFileName(fileName.beforeLastOrFull(".")))
        {
            TRACE_ERROR("Failed to create directory '{}' in '{}': invalid file name", fileName, depotPath());
            return nullptr;
        }

        // we need the initial content as well
        if (!initialContent)
        {
            TRACE_ERROR("Unable to create '{}' in '{}': no content provided", fileName, depotPath());
            return nullptr;
        }

        auto existingExtension = fileName.afterFirst(".");
        auto expectedExtension = res::IResource::GetResourceExtensionForClass(initialContent->cls());
        if (existingExtension && existingExtension != expectedExtension)
        {
            TRACE_ERROR("Unable to create '{}' in '{}': file extension '{}' does not match required by class '{}': '{}'",
                fileName, depotPath(), existingExtension, initialContent->cls()->name(), expectedExtension);
            return nullptr;
        }

        StringBuf fileDepotPath, fixedName;
        if (existingExtension)
        {
            fixedName = StringBuf(fileName);
            fileDepotPath = TempString("{}{}", depotPath(), fileName);
            fileName = fixedName;
        }
        else
        {
            fixedName = TempString("{}.{}", fileName, expectedExtension);
            fileDepotPath = TempString("{}{}.{}", depotPath(), fileName, expectedExtension);
            fileName = fixedName;
        }

        // file already exists, do not overwrite
        if (base::io::FileExists(TempString("{}{}", absolutePath(), fileName)))
        {
            TRACE_ERROR("Unable to create '{}' in '{}': file already exists on disk", fileName, depotPath());
            return file(fileName, true);
        }

        // we already have the file
        if (auto existingFile = file(fileName, true))
        {
            if (!existingFile->isDeleted())
            {
                TRACE_ERROR("Unable to create '{}' in '{}': file already exists in file system", fileName, depotPath());
                return existingFile;
            }
        }

        // store content for the file
        StringBuf depotFilePath = TempString("{}{}", depotPath(), fileName);
        const auto writer = depot()->depot().createFileWriter(depotFilePath);
        if (!writer)
        {
            TRACE_ERROR("Unable to create '{}' in '{}': filed to open file for writing (out of disk space?)", fileName, depotPath());
            return nullptr;
        }

        // setup saving context
        res::FileSavingContext context;
        context.rootObject.pushBack(initialContent);

        // save the file
        if (!res::SaveFile(writer, context))
        {
            TRACE_ERROR("Unable to create '{}' in '{}': filed to write file's content (out of disk space?)", fileName, depotPath());
            writer->discardContent();
            return nullptr;
        }

        // update
        auto managedFile = file(fileName, true);
        if (managedFile)
        {
            managedFile->deleted(false);
        }
        else if (auto newFile = CreateManagedFile(depot(), this, fileName))
        {
            m_files.add(newFile);
            managedFile = newFile;
        }

        // update file counts
        updateFileCount();

        // report file event
        DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_FILE_CREATED, ManagedFilePtr(AddRef(managedFile)));
        return managedFile;

    }

    ManagedFile* ManagedDirectory::createFile(StringView fileName, const ManagedFileFormat& format)
    {
        // no name
        DEBUG_CHECK_EX(fileName, "File name should be provided");
        if (!fileName)
            return nullptr;

        // validate file name
        if (!ValidateFileName(fileName.beforeFirstOrFull(".")))
        {
            TRACE_ERROR("Failed to create directory '{}' in '{}': invalid file name", fileName, depotPath());
            return nullptr;
        }

        // create initial content
        auto resourceObject = format.createEmpty();
        if (!resourceObject)
        {
            TRACE_ERROR("Unable to create '{}' in '{}': failed to create object of class '{}'", fileName, depotPath(), format.description());
            return nullptr;
        }

        // save initial content
        return createFile(fileName, resourceObject);
    }

    ManagedFile* ManagedDirectory::createFile(StringView fileName, Buffer initialContent)
    {
        // no name
        DEBUG_CHECK_EX(fileName, "File name should be provided");
        if (!fileName)
            return nullptr;

        // validate file name
        if (!ValidateFileName(fileName.beforeLast(".")))
        {
            TRACE_ERROR("Failed to create directory '{}' in '{}': invalid file name", fileName, depotPath());
            return nullptr;
        }

        // file already exists, do not overwrite
        if (base::io::FileExists(TempString("{}{}", absolutePath(), fileName)))
        {
            TRACE_ERROR("Unable to create '{}' in '{}': file already exists on disk", fileName, depotPath());
            return file(fileName, true);
        }

        // we already have the file
        if (auto existingFile = file(fileName, true))
        {
            if (!existingFile->isDeleted())
            {
                TRACE_ERROR("Unable to create '{}' in '{}': file already exists in file system", fileName, depotPath());
                return existingFile;
            }
        }

        // store content for the file
        StringBuf depotFilePath = TempString("{}{}", depotPath(), fileName);
        const auto writer = depot()->depot().createFileWriter(depotFilePath);
        if (!writer)
        {
            TRACE_ERROR("Unable to create '{}' in '{}': filed to open file for writing (out of disk space?)", fileName, depotPath());
            return nullptr;
        }

        // write content
        if (writer->writeSync(initialContent.data(), initialContent.size()) != initialContent.size())
        {
            TRACE_ERROR("Unable to create '{}' in '{}': filed to write file's content (out of disk space?)", fileName, depotPath());
            writer->discardContent();
            return nullptr;
        }

        // update
        auto managedFile = file(fileName, true);
        if (managedFile)
        {
            managedFile->deleted(false);
        }
        else
        {
            auto newFile = CreateManagedFile(depot(), this, fileName);
            m_files.add(newFile);
            managedFile = newFile;
        }

        // update file counts
        updateFileCount();

        // report file event
        DispatchGlobalEvent(depot()->eventKey(), EVENT_MANAGED_DEPOT_FILE_CREATED, ManagedFilePtr(AddRef(managedFile)));
        return managedFile;
    }


    ManagedFile* ManagedDirectory::createFileCopy(StringView name, const ManagedFile* sourceFile)
    {
        DEBUG_CHECK_RETURN_EX_V(sourceFile, "No source data to load", nullptr);
        DEBUG_CHECK_RETURN_EX_V(!sourceFile->isDeleted(), "Cannot duplicate deleted file", nullptr);

        Buffer buffer;

        {
            // open source file
            const auto reader = depot()->depot().createFileReader(sourceFile->depotPath());
            if (!reader)
            {
                TRACE_ERROR("Unable to open file '{}'", sourceFile->depotPath());
                return nullptr;
            }

            // allocate buffer
            auto size = reader->size();
            buffer = Buffer::Create(POOL_MANAGED_DEPOT, size);
            if (!buffer)
            {
                TRACE_ERROR("Unable to load source content from '{}'", sourceFile->depotPath());
                return nullptr;
            }

            // load content
            if (size != reader->readSync(buffer.data(), size))
            {
                TRACE_ERROR("Unable to load source content from '{}', size {}", sourceFile->depotPath(), size);
                return nullptr;
            }
        }

        // store loaded content as new file
        return createFile(name, buffer);
    }

    void ManagedDirectory::directoryNames(Array<StringBuf>& outDirectoryNames) const
    {
        if (auto parent  = parentDirectory())
        {
            parent->directoryNames(outDirectoryNames);
            outDirectoryNames.pushBack(name());
        }
    }

    void ManagedDirectory::updateModifiedContentCount(bool updateParent)
    {
        bool wasModified = isModified();

        m_modifiedContentCount = 0;

        for (auto& a : m_directories.list())
            if (!a->isDeleted() && a->isModified())
                m_modifiedContentCount += 1;

        for (auto& a : m_files.list())
            if (!a->isDeleted() && a->isModified())
                m_modifiedContentCount += 1;

        if (isModified() != wasModified && updateParent)
        {
            //m_depot->mod notifyDirModified(depotPath(), isModified());

            if (auto parentDir = parentDirectory())
                parentDir->updateModifiedContentCount();
        }
    }

    void ManagedDirectory::updateFileCount(bool updateParent)
    {
        auto oldFileCount = m_fileCount;

        m_fileCount = 0;
        for (auto& a : m_files.list())
            if (!a->isDeleted())
                m_fileCount += 1;
            
        for (auto& a : m_directories.list())
            if (!a->isDeleted())
                m_fileCount += a->files().size();

        if (updateParent && m_fileCount != oldFileCount)
        {
            if (auto parent  = parentDirectory())
                parent->updateFileCount();
        }
    }

    bool ManagedDirectory::visitFiles(bool recursive, StringView fileNamePattern, const std::function<bool(ManagedFile*)>& enumFunc)
    {
        for (auto& file : m_files.list())
            if (!file->isDeleted())
                if (fileNamePattern.empty() || file->name().view().matchPattern(fileNamePattern))
                    if (enumFunc(file))
                        return true;

        if (recursive)
            for (auto& subDir : m_directories.list())
                if (!subDir->isDeleted())
                    if (subDir->visitFiles(recursive, fileNamePattern, enumFunc))
                        return true;

        return false;
    }

    bool ManagedDirectory::visitDirectories(bool recursive, const std::function<bool(ManagedDirectory*)>& enumFunc)
    {
        if (recursive)
            for (auto& subDir : m_directories.list())
                if (subDir->visitDirectories(recursive, enumFunc))
                    return true;

        return enumFunc(this);
    }

    //--

    bool ManagedDirectory::collectFiles(CollectionContext& context, bool recursive, ManagedFileCollection& outFiles) const
    {
        bool ret = false;

        if (context.visitedDirectories.insert(this))
        {
            for (auto* file : m_files.list())
                if (!context.filterFilesFunc || context.filterFilesFunc(file))
                    ret |= outFiles.collectFile(file);

            if (recursive)
            {
                for (auto* dir : m_directories.list())
                    if (!context.filterDirectoriesFunc || context.filterDirectoriesFunc(dir))
                        ret |= dir->collectFiles(context, recursive, outFiles);
            }
        }

        return ret;
    }
        
} // depot

