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

namespace ed
{
    //--

    static res::StaticResource<image::Image> resDirectoryTexture("engine/thumbnails/directory.png");

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedDirectory);
    RTTI_END_TYPE();

    ManagedDirectory::ManagedDirectory(ManagedDepot* dep, ManagedDirectory* parentDirectory, StringView<char> name, StringView<char> depotPath, bool isFileSystemRoot)
        : ManagedItem(dep, parentDirectory, name)
        , m_depotPath(depotPath)
        , m_isFileSystemRoot(isFileSystemRoot)
        , m_fileCount(0)
    {
        m_directories.reserve(32);
        m_files.reserve(512);
        m_dirMap.reserve(32);
        m_fileMap.reserve(512);

        m_directoryIcon = resDirectoryTexture.loadAndGetAsRef();
    }

    ManagedDirectory::~ManagedDirectory()
    {}

    const image::ImageRef& ManagedDirectory::typeThumbnail() const
    {
        return m_directoryIcon;
    }

    bool ManagedDirectory::fetchThumbnailData(uint32_t& versionToken, image::ImageRef& outThumbnailImage, Array<StringBuf>& outComments) const
    {
        if (versionToken != 1)
        {
            versionToken = 1;
            outThumbnailImage = typeThumbnail();
            outComments.reset();

            if (isFileSystemRoot())
            {
                outComments.emplaceBack("Package");
            }
            return true;
        }

        return false;
    }

    void ManagedDirectory::collect(const std::function<bool(const ManagedFile*)>& filter, Array<ManagedFile*>& outFiles) const
    {
        // collect files
        for (auto* ptr : m_files)
            if (filter(ptr))
                outFiles.pushBack(ptr);

        // recurse
        for (auto childDirectory : m_directories)
            childDirectory->collect(filter, outFiles);
    }

    void ManagedDirectory::bookmark(bool state)
    {
        if (m_bookmarked != state)
        {
            m_bookmarked = state;
            m_depot->toogleDirectoryBookmark(this, state);
        }
    }

    void ManagedDirectory::populate()
    {
        refreshContent(true);
    }

    const ManagedDirectory::TFiles& ManagedDirectory::files()
    {
        if (m_filesRequireSorting)
        {
            std::sort(m_files.begin(), m_files.end(), [](const ManagedFile* a, const ManagedFile* b)
                {
                    return a->name() < b->name();
                });
            m_filesRequireSorting = false;
        }
        return m_files;
    }

    const ManagedDirectory::TDirectories& ManagedDirectory::directories()
    {
        if (m_directoriesRequireSortying)
        {
            std::sort(m_directories.begin(), m_directories.end(), [](const ManagedDirectory* a, const ManagedDirectory* b)
                {
                    return a->name() < b->name();
                });
            m_directoriesRequireSortying = false;
        }

        return m_directories;
    }


    void ManagedDirectory::refreshContent(bool isCaller)
    {
        PC_SCOPE_LVL0(RefreshContent);

        // create files
        {
            bool added = false;
            m_depot->loader().enumFilesAtPath(depotPath(), [this, &added](const depot::DepotStructure::FileInfo& info)
                {
                    if (auto existingFile = file(info.name, true))
                    {
                        if (existingFile->m_isDeleted)
                            existingFile->m_isDeleted = false;
                    }
                    else
                    {
                        if (auto fileWrapper = CreateSharedPtr<ManagedFile>(m_depot, this, info.name))
                        {
                            m_fileRefs.pushBack(fileWrapper);
                            m_files.pushBack(fileWrapper);
                            m_fileMap[fileWrapper->name()] = fileWrapper;
                        }
                    }

                    return false;
                });

            if (added)
                m_filesRequireSorting = true;
        }

        // create directories
        {
            bool added = false;
            m_depot->loader().enumDirectoriesAtPath(depotPath(), [this, &added](const depot::DepotStructure::DirectoryInfo& info)
                {
                    if (auto existingDir = directory(info.name, true))
                    {
                        if (existingDir->m_isDeleted)
                            existingDir->m_isDeleted = false;
                    }
                    else
                    {
                        if (auto dirWrapper = CreateSharedPtr<ManagedDirectory>(m_depot, this, info.name, TempString("{}{}/", depotPath(), info.name), info.fileSystemRoot))
                        {
                            m_dirRefs.pushBack(dirWrapper);
                            m_directories.pushBack(dirWrapper);
                            m_dirMap[dirWrapper->name()] = dirWrapper;
                        }
                    }
                    return false;
                });

            if (added)
                m_directoriesRequireSortying = true;
        }

        // scan children directories
        for (const auto& dir : m_directories)
            dir->refreshContent(false);

        // finally, update local counts
        updateFileCount(false);
        updateModifiedContentCount(false);
    }

    ManagedDirectory* ManagedDirectory::directory(StringView<char> dirName, bool allowDeleted /*= false*/) const
    {
        ManagedDirectory* ret = nullptr;
        if (m_dirMap.find(dirName, ret))
            if (allowDeleted || !ret->isDeleted())
                return ret;
        return nullptr;
    }

    ManagedFile* ManagedDirectory::file(StringView<char> fileName, bool allowDeleted /*= false*/) const
    {
        ManagedFile* ret = nullptr;
        if (m_fileMap.find(fileName, ret))
            if (allowDeleted || !ret->isDeleted())
                return ret;
        return nullptr;
    }

    ManagedDirectory* ManagedDirectory::createDirectory(StringView<char> name)
    {
        // return existing sub-directory
        auto curDir = directory(name, true);
        if (curDir && !curDir->isDeleted())
            return curDir;

        // validate file name
        if (!ValidateName(name))
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
        if (curDir)
        {
            curDir->m_isDeleted = false;
        }
        else
        {
            auto childDepotPath = TempString("{}{}/", m_depotPath, name);
            auto newDir = CreateSharedPtr<ManagedDirectory>(m_depot, this, StringBuf(name), childDepotPath, false);

            m_directories.pushBack(newDir);
            m_dirRefs.pushBack(newDir);
            m_dirMap[newDir->name()] = newDir;
            m_directoriesRequireSortying = true;
            curDir = newDir;
        }

        // fill new directory with content, usually such directories are empty
        curDir->populate();

        // refresh file count
        updateFileCount();

        // report events
        m_depot->dispatchEvent(curDir, ManagedDepotEvent::DirCreated);
        return curDir;
    }

    StringBuf ManagedDirectory::adjustFileName(StringView<char> fileName) const
    {
        HashSet<StringBuf> names;
        for (const auto& file : m_files)
            if (!file->isDeleted())
                names.insert(file->name().toLower());

        for (const auto& dir : m_directories)
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
        if (!ValidateName(fileName.beforeFirstOrFull(".")))
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
        if (!m_depot->loader().queryFileMountPoint(fileDepotPath, mountPoint))
        {
            TRACE_ERROR("Unable to create '{}' in '{}': no valid moutin point in depot", fileName, depotPath());
            return nullptr;
        }

        // file already exists, do not overwrite
        auto absFilePath = absolutePath().addFile(StringBuf(fileName).c_str());
        if (IO::GetInstance().fileExists(absFilePath))
        {
            TRACE_ERROR("Unable to create '{}' in '{}': file already exists on disk", fileName, depotPath());
            notifyDepotFileCreated(fileName);
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
        const auto writer = m_depot->loader().createFileWriter(depotFilePath);
        if (!writer)
        {
            TRACE_ERROR("Unable to create '{}' in '{}': filed to open file for writing (out of disk space?)", fileName, depotPath());
            return nullptr;
        }

        // setup saving context
        base::res::FileSavingContext context;
        context.rootObject.pushBack(initialContent);
        context.basePath = mountPoint.path();

        // save the file
        if (!base::res::SaveFile(writer, context))
        {
            TRACE_ERROR("Unable to create '{}' in '{}': filed to write file's content (out of disk space?)", fileName, depotPath());
            writer->discardContent();
            return nullptr;
        }

        // update
        auto managedFile = file(fileName, true);
        if (managedFile)
        {
            if (managedFile->isDeleted())
                managedFile->m_isDeleted = false;
        }
        else
        {
            auto newFile = CreateSharedPtr<ManagedFile>(depot(), this, StringBuf(fileName));
            m_files.pushBack(newFile);
            m_fileMap[newFile->name()] = newFile;
            m_fileRefs.pushBack(newFile);
            m_filesRequireSorting = true;
            managedFile = newFile;
        }

        // update file counts
        updateFileCount();

        // report file event
        m_depot->dispatchEvent(managedFile, ManagedDepotEvent::FileCreated);
        return managedFile;

    }

    ManagedFile* ManagedDirectory::createFile(StringView<char> fileName, const ManagedFileFormat& format)
    {
        // no name
        DEBUG_CHECK_EX(fileName, "File name should be provided");
        if (!fileName)
            return nullptr;

        // validate file name
        if (!ValidateName(fileName.beforeFirstOrFull(".")))
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
        if (!ValidateName(fileName.beforeFirst(".")))
        {
            TRACE_ERROR("Failed to create directory '{}' in '{}': invalid file name", fileName, depotPath());
            return nullptr;
        }

        // file already exists, do not overwrite
        auto absFilePath = absolutePath().addFile(StringBuf(fileName).c_str());
        if (IO::GetInstance().fileExists(absFilePath))
        {
            TRACE_ERROR("Unable to create '{}' in '{}': file already exists on disk", fileName, depotPath());
            notifyDepotFileCreated(fileName);
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
        const auto writer = m_depot->loader().createFileWriter(depotFilePath);
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
            if (managedFile->isDeleted())
                managedFile->m_isDeleted = false;
        }
        else
        {
            auto newFile = CreateSharedPtr<ManagedFile>(depot(), this, StringBuf(fileName));
            m_files.pushBack(newFile);
            m_fileMap[newFile->name()] = newFile;
            m_fileRefs.pushBack(newFile);
            m_filesRequireSorting = true;
            managedFile = newFile;
        }

        // update file counts
        updateFileCount();

        // report file event
        m_depot->dispatchEvent(managedFile, ManagedDepotEvent::FileCreated);
        return managedFile;
    }

    void ManagedDirectory::directoryNames(Array<StringBuf>& outDirectoryNames) const
    {
        if (auto parent  = parentDirectory())
        {
            parent->directoryNames(outDirectoryNames);
            outDirectoryNames.pushBack(m_name);
        }
    }

    void ManagedDirectory::updateModifiedContentCount(bool updateParent)
    {
        bool wasModified = isModified();

        m_modifiedContentCount = 0;

        for (auto& a : m_directories)
            if (!a->isDeleted() && a->isModified())
                m_modifiedContentCount += 1;

        for (auto& a : m_files)
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
        for (auto& a : m_files)
            if (!a->isDeleted())
                m_fileCount += 1;
            
        for (auto& a : m_directories)
            if (!a->isDeleted())
                m_fileCount += a->files().size();

        if (updateParent && m_fileCount != oldFileCount)
        {
            if (auto parent  = parentDirectory())
                parent->updateFileCount();
        }
    }

    bool ManagedDirectory::notifyDepotFileCreated(StringView<char> name)
    {
        bool added = false;

        if (auto existingFile = file(name, true))
        {
            if (existingFile->m_isDeleted)
            {
                existingFile->m_isDeleted = false;
                m_fileCount += 1;
                added = true;
            }
        }
        else
        {
            if (auto fileWrapper = CreateSharedPtr<ManagedFile>(m_depot, this, StringBuf(name)))
            {
                m_files.pushBack(fileWrapper);
                m_fileRefs.pushBack(fileWrapper);
                m_fileMap[fileWrapper->name()] = fileWrapper;
                m_fileCount += 1;
                added = true;
            }
        }    

        if (auto parent = parentDirectory())
            parent->updateFileCount();

        return added;
    }

    bool ManagedDirectory::notifyDepotFileDeleted(StringView<char> name)
    {
        if (auto existingFile = file(name, true))
        {
            if (!existingFile->m_isDeleted)
            {
                existingFile->m_isDeleted = true;
                m_fileCount -= 1;

                if (auto parent = parentDirectory())
                    parent->updateFileCount();

                updateModifiedContentCount();
                return true;
            }
        }

        return false;
    }

    bool ManagedDirectory::notifyDepotDirCreated(StringView<char> name, bool fileSystemRoot)
    {
        bool added = false;

        if (auto existingDir = directory(name, true))
        {
            if (existingDir->m_isDeleted)
            {
                existingDir->m_isDeleted = false;

                updateModifiedContentCount();
                updateFileCount();
                added = true;
            }
        }
        else
        {
            StringBuf childDepotPath = TempString("{}{}/", depotPath(), name);
            if (auto dirWrapper = CreateSharedPtr<ManagedDirectory>(m_depot, this, StringBuf(name), childDepotPath, fileSystemRoot))
            {
                m_directories.pushBack(dirWrapper);
                m_dirRefs.pushBack(dirWrapper);
                m_dirMap[dirWrapper->name()] = dirWrapper;
                added = true;
            }
        }

        return added;
    }

    bool ManagedDirectory::notifyDepotDirDeleted(StringView<char> name)
    {
        if (auto existingDir = directory(name, true))
        {
            if (!existingDir->m_isDeleted)
            {
                existingDir->m_isDeleted = true;

                updateModifiedContentCount();
                updateFileCount();
                return true;
            }
        }

        return false;
    }

    bool ManagedDirectory::visitFiles(bool recursive, StringView<char> fileNamePattern, const std::function<bool(ManagedFile*)>& enumFunc)
    {
        for (auto& file : m_files)
            if (!file->isDeleted())
                if (fileNamePattern.empty() || file->name().view().matchPattern(fileNamePattern))
                    if (enumFunc(file))
                        return true;

        if (recursive)
            for (auto& subDir : m_directories)
                if (!subDir->isDeleted())
                    if (subDir->visitFiles(recursive, fileNamePattern, enumFunc))
                        return true;

        return false;
    }

    bool ManagedDirectory::visitDirectories(bool recursive, const std::function<bool(ManagedDirectory*)>& enumFunc)
    {
        if (recursive)
            for (auto& subDir : m_directories)
                if (subDir->visitDirectories(recursive, enumFunc))
                    return true;

        return enumFunc(this);
    }

    //--
        
} // depot

