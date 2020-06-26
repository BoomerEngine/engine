/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "managedItem.h"
#include "managedFile.h"

#include "base/io/include/absolutePath.h"
#include "base/system/include/spinLock.h"
#include "base/io/include/ioDirectoryWatcher.h"

namespace ed
{
    /// a special lower-case only key
    struct ManagedDirectoryLowerCaseKey
    {
        ManagedDirectoryLowerCaseKey(base::StringView<char> view)
        {
            m_data = base::StringBuf(view).toLower();
        }

        ManagedDirectoryLowerCaseKey(const base::StringBuf& txt)
        {
            m_data = txt.toLower();
        }

        static uint32_t CalcHash(const ManagedDirectoryLowerCaseKey& key)
        {
            const auto& txt = key.m_data.view();
            return base::prv::BaseHelper::StringHashNoCase(txt.data(), txt.data() + txt.length());
        }

        static uint32_t CalcHash(base::StringView<char> txt)
        {
            return base::prv::BaseHelper::StringHashNoCase(txt.data(), txt.data() + txt.length());
        }

        INLINE bool operator==(const ManagedDirectoryLowerCaseKey& key) const
        {
            return m_data == key.m_data;
        }

        INLINE bool operator==(base::StringView<char> txt) const
        {
            return 0 == m_data.view().caseCmp(txt);
        }

        base::StringBuf m_data;
    };


    /// directory in the managed depot
    class BASE_EDITOR_API ManagedDirectory : public ManagedItem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ManagedDirectory, ManagedItem);

    public:
        /// is this a root depot directory ?
        INLINE bool isRoot() const { return parentDirectory() == nullptr; }

        /// is this a file system mounting directory ?
        INLINE bool isFileSystemRoot() const { return m_isFileSystemRoot; }

        /// is this directory modified ? (has modified files/directories)
        INLINE bool isModified() const { return m_modifiedContentCount > 0; }

        /// is this directory bookmarked ?
        INLINE bool isBookmarked() const { return m_bookmarked; }

        /// get number of files in this directory and below
        INLINE uint32_t fileCount() const { return m_fileCount; }

        /// get depot path
        virtual StringBuf depotPath() const override { return m_depotPath; };

        //--

        /// get list of managed files in this directory
        typedef Array< ManagedFile* > TFiles;
        const TFiles& files();

        /// get list of managed directories in this directory
        typedef Array< ManagedDirectory* > TDirectories;
        const TDirectories& directories();

        //--

        ManagedDirectory(ManagedDepot* dep, ManagedDirectory* parentDirectory, StringView<char> name, StringView<char> depotPath, bool isFileSystemRoot);
        virtual ~ManagedDirectory();

        /// fill directory with content by scanning actual depot
        /// NOTE: no events are called
        void populate();

        /// collect files from this directory and below that match given filter
        void collect(const std::function<bool(const ManagedFile*)>& filter, Array<ManagedFile*>& outFiles) const;
            
        ///---

        // find child directory by name, returns null if not found
        ManagedDirectory* directory(StringView<char> dirName, bool allowDeleted=false) const;

        // find file by name with extension, returns null if not found
        ManagedFile* file(StringView<char> fileName, bool allowDeleted = false) const;

        ///---

        /// toggle bookmark
        void bookmark(bool state);

        ///---

        /// Adjust file name not to be duplicated
        StringBuf adjustFileName(StringView<char> fileName) const;

        /// Get list of directory names (ie- from meshes\test\crap\dupa.file gets array with strings: "meshes", "test", "crap")
        void directoryNames(Array<StringBuf>& outDirectoryNames) const;

        /// Update file count of this directory
        void updateFileCount(bool updateParent=true);

        /// Update list of modified files/directories
        void updateModifiedContentCount(bool updateParent = true);

        ///---

        // visit all files
        bool visitFiles(bool recursive, StringView<char> fileNamePattern, const std::function<bool(ManagedFile*)>& enumFunc);

        // visit all directories
        bool visitDirectories(bool recursive, const std::function<bool(ManagedDirectory*)>& enumFunc);

        ///---

        /// create a directory in the file system, if it worked we return the created directory
        ManagedDirectory* createDirectory(StringView<char> name);

        /// create a file in the file system, we must store some content for this to work
        ManagedFile* createFile(StringView<char> name, Buffer initialContent);

        /// create an file of given type in the file system, use provided resource as initial content
        ManagedFile* createFile(StringView<char> name, const res::ResourceHandle& initialContent);

        /// create an empty file of given type in the file system
        ManagedFile* createFile(StringView<char> name, const ManagedFileFormat& format);
            
        ///---

        // fetch directory thumbnail
        virtual bool fetchThumbnailData(uint32_t& versionToken, image::ImageRef& outThumbnailImage, Array<StringBuf>& outComments) const override;

        /// Get type (resource type) thumbnail, can be used when file thumbnail is not loaded
        virtual const image::ImageRef& typeThumbnail() const;

    protected:
        StringBuf m_depotPath;

        base::HashMap<ManagedDirectoryLowerCaseKey, ManagedFile*> m_fileMap;
        base::HashMap<ManagedDirectoryLowerCaseKey, ManagedDirectory*> m_dirMap;

        TFiles m_files;
        TDirectories m_directories;

        base::Array<base::RefPtr<ManagedDirectory>> m_dirRefs;
        base::Array<base::RefPtr<ManagedFile>> m_fileRefs;

        base::image::ImageRef m_directoryIcon;

        bool m_isPopulated = false;
        bool m_isFileSystemRoot = false;
        bool m_bookmarked = false;
        bool m_filesRequireSorting = false;
        bool m_directoriesRequireSortying = false;

        uint32_t m_fileCount = 0;
        uint32_t m_modifiedContentCount = 0;

        SpinLock m_lock;

        bool notifyDepotFileCreated(StringView<char> name);
        bool notifyDepotFileDeleted(StringView<char> name);
        bool notifyDepotDirCreated(StringView<char> name, bool fileSystemRoot=false);
        bool notifyDepotDirDeleted(StringView<char> name);

        void refreshContent(bool isCaller);

        void refreshFiles(bool reportEvents);
        void refreshDirectories(bool reportEvents, Array<ManagedDirectory*>* outNewDirectories = nullptr);

        friend class ManagedDepot;
    };

    //--

} // ed
