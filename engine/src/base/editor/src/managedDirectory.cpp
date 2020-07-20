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

namespace ed
{
    //--

    static res::StaticResource<image::Image> resDirectoryTexture("engine/thumbnails/directory.png");

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedDirectory);
    RTTI_END_TYPE();

    ManagedDirectory::ManagedDirectory(ManagedDepot* dep, ManagedDirectory* parentDirectory, StringView<char> name, StringView<char> depotPath)
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

    static ManagedFilePtr CreateManagedFile(ManagedDepot* depot, ManagedDirectory* dir, StringView<char> name)
    {
        const auto fileExt = name.afterFirst(".");
        if (fileExt)
        {
            if (const auto format = ManagedFileFormatRegistry::GetInstance().format(fileExt))
            {
                if (format->nativeResourceClass())
                    return CreateSharedPtr<ManagedFileNativeResource>(depot, dir, name);
                else
                    return CreateSharedPtr<ManagedFileRawResource>(depot, dir, name);
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
                        if (auto dirWrapper = CreateSharedPtr<ManagedDirectory>(depot(), this, info.name, TempString("{}{}/", depotPath(), info.name)))
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

    ManagedDirectory* ManagedDirectory::directory(StringView<char> dirName, bool allowDeleted /*= false*/) const
    {
        if (auto ret = m_directories.find(dirName))
            if (allowDeleted || !ret->isDeleted())
                return ret;

        return nullptr;
    }

    ManagedFile* ManagedDirectory::file(StringView<char> fileName, bool allowDeleted /*= false*/) const
    {
        if (auto ret = m_files.find(fileName))
            if (allowDeleted || !ret->isDeleted())
                return ret;
        return nullptr;
    }

    ManagedDirectory* ManagedDirectory::createDirectory(StringView<char> name)
    {
        // return existing sub-directory
        auto curDir = directory(name, true);
        if (curDir)
        {
            // "undelete" deleted directories
            if (curDir->isDeleted())
            {
                curDir->deleted(false);

                auto dirPath = absolutePath().addDir(StringBuf(name).c_str());
                IO::GetInstance().createPath(dirPath);

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
        auto dirPath = absolutePath().addDir(StringBuf(name).c_str());
        if (!IO::GetInstance().createPath(dirPath))
        {
            TRACE_ERROR("Failed to create directory '{}' in '{}': failed to create physical directory on disk", name, depotPath());
            return nullptr;
        }

        // create wrapper
        auto childDepotPath = TempString("{}{}/", m_depotPath, name);
        auto newDir = CreateSharedPtr<ManagedDirectory>(depot(), this, StringBuf(name), childDepotPath);
        m_directories.add(newDir);
        curDir = newDir;

        // fill new directory with content, usually such directories are empty
        curDir->populate();

        // refresh file count
        updateFileCount();

        // report events
        DispatchGlobalEvent(m_eventKey, EVENT_MANAGED_DEPOT_DIRECTORY_CREATED, curDir);
        return curDir;
    }

    StringBuf ManagedDirectory::adjustFileName(StringView<char> fileName) const
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

    ManagedFile* ManagedDirectory::createFile(StringView<char> fileName, const res::ResourceHandle& initialContent)
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

        res::ResourceMountPoint mountPoint;
        if (!depot()->depot().queryFileMountPoint(fileDepotPath, mountPoint))
        {
            TRACE_ERROR("Unable to create '{}' in '{}': no valid moutin point in depot", fileName, depotPath());
            return nullptr;
        }

        // file already exists, do not overwrite
        auto absFilePath = absolutePath().addFile(StringBuf(fileName).c_str());
        if (IO::GetInstance().fileExists(absFilePath))
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
        context.basePath = mountPoint.path();

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
        DispatchGlobalEvent(m_eventKey, EVENT_MANAGED_DEPOT_FILE_CREATED, managedFile);
        return managedFile;

    }

    ManagedFile* ManagedDirectory::createFile(StringView<char> fileName, const ManagedFileFormat& format)
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

    ManagedFile* ManagedDirectory::createFile(StringView<char> fileName, Buffer initialContent)
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
        auto absFilePath = absolutePath().addFile(StringBuf(fileName).c_str());
        if (IO::GetInstance().fileExists(absFilePath))
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
        }

        // update file counts
        updateFileCount();

        // report file event
        DispatchGlobalEvent(m_eventKey, EVENT_MANAGED_DEPOT_FILE_CREATED, managedFile);
        return managedFile;
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

    bool ManagedDirectory::visitFiles(bool recursive, StringView<char> fileNamePattern, const std::function<bool(ManagedFile*)>& enumFunc)
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
        
} // depot

