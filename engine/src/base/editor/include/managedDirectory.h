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
    //---

    /// a special lower-case only key
    struct ManagedDirectoryLowerCaseKey
    {
        ManagedDirectoryLowerCaseKey(StringView<char> view)
        {
            m_data = StringBuf(view).toLower();
        }

        ManagedDirectoryLowerCaseKey(const StringBuf& txt)
        {
            m_data = txt.toLower();
        }

        static uint32_t CalcHash(const ManagedDirectoryLowerCaseKey& key)
        {
            const auto& txt = key.m_data.view();
            return prv::BaseHelper::StringHashNoCase(txt.data(), txt.data() + txt.length());
        }

        static uint32_t CalcHash(StringView<char> txt)
        {
            return prv::BaseHelper::StringHashNoCase(txt.data(), txt.data() + txt.length());
        }

        INLINE bool operator==(const ManagedDirectoryLowerCaseKey& key) const
        {
            return m_data == key.m_data;
        }

        INLINE bool operator==(StringView<char> txt) const
        {
            return 0 == m_data.view().caseCmp(txt);
        }

        StringBuf m_data;
    };

    //--

    /// helper "container"
    template< typename T >
    struct ManagedDirectoryItemList
    {
        void add(T* ptr)
        {
            elemRefs.emplaceBack(AddRef(ptr));
            elemList.emplaceBack(ptr);
            elemMap[ptr->name()] = ptr;
        }

        T* find(StringView<char> name) const
        {
            T* ret = nullptr;
            elemMap.find(name, ret);
            return ret;
        }

        const Array<T*>& list() const
        {
            return elemList;
        }

    private:
        HashMap<ManagedDirectoryLowerCaseKey, T*> elemMap;
        Array<T*> elemList;
        Array<RefPtr<T>> elemRefs;
    };

    //--

    /// directory in the managed depot
    class BASE_EDITOR_API ManagedDirectory : public ManagedItem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ManagedDirectory, ManagedItem);

    public:
        /// is this a root depot directory ?
        INLINE bool isRoot() const { return parentDirectory() == nullptr; }

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
        INLINE const Array<ManagedFile*>& files() const { return m_files.list(); }

        /// get list of managed directories in this directory
        INLINE const Array<ManagedDirectory*>& directories() const { return m_directories.list(); }

        //--

        ManagedDirectory(ManagedDepot* dep, ManagedDirectory* parentDirectory, StringView<char> name, StringView<char> depotPath);
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

        /// toggle the "deleted" flag
        void deleted(bool flag);

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

        /// Get type (resource type) thumbnail, can be used when file thumbnail is not loaded
        virtual const image::ImageRef& typeThumbnail() const;

        ///---

    protected:
        StringBuf m_depotPath;

        ManagedDirectoryItemList<ManagedFile> m_files;
        ManagedDirectoryItemList<ManagedDirectory> m_directories;

        image::ImageRef m_directoryIcon;

        bool m_bookmarked = false;

        uint32_t m_fileCount = 0;
        uint32_t m_modifiedContentCount = 0;

        SpinLock m_lock;

        void refreshContent();

        friend class ManagedDepot;
    };

    //--

} // ed
