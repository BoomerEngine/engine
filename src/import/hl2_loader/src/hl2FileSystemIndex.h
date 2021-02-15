/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#pragma once

namespace HLLib
{
    class CPackage;
    class CVPKFile;
    class CDirectoryItem;
}

namespace hl2
{

    class FileSystemDataBuilder;

    // index file for the HL2 file system, helps with faster loading of the files
    class FileSystemIndex : public base::NoCopy
    {
    public:
        FileSystemIndex();
        ~FileSystemIndex();

#pragma pack(push)
#pragma pack(1)
        struct PackageInfo
        {
            uint32_t m_name;
            uint32_t m_numFiles; // 0 for physical files
            uint64_t m_timeStamp;
        };

        struct FileInfo
        {
            uint32_t m_name;
            uint32_t m_packageIndex;
            uint32_t m_parent;
            uint64_t m_pathHash;
            uint64_t m_dataCRC;
            uint32_t m_dataOffset;
            uint32_t m_dataSize;
            int m_nextFile;
        };

        struct DirInfo
        {
            uint32_t m_name;
            uint64_t m_pathHash;
            int m_parent;
            int m_firstDir;
            int m_nextDir;
            int m_firstFile;
        };
#pragma pack(pop)

        /// get dir table
        INLINE const DirInfo* directories() const { return m_directories.typedData(); }

        /// get file table
        INLINE const FileInfo* files() const { return m_files.typedData(); }

        /// get package table
        INLINE const PackageInfo* packages() const { return m_packages.typedData(); }

        /// get number of packages
        INLINE uint32_t numPackages() const { return m_packages.size(); }

        /// get string table
        INLINE const char* stringTable() const { return m_stringTable.typedData(); }

        /// get directory entry for given directory path
        int findDirectoryEntry(base::StringView path) const;

        /// get file entry for given path
        int findFileEntry(base::StringView path) const;

        //--

        /// load the file system index from given directory, builds a new one if the current one is not up to date or does not exist
        static base::UniquePtr<FileSystemIndex> Load(const base::StringBuf& contentPath);

    private:
        base::Array<char> m_stringTable;
        base::Array<PackageInfo> m_packages;
        base::Array<FileInfo> m_files;
        base::Array<DirInfo> m_directories;

        //--

        base::HashMap<uint64_t, uint32_t> m_dirMap;
        base::HashMap<uint64_t, uint32_t> m_fileMap;

        //--

        bool load(base::io::IReadFileHandle& file);
        bool save(base::io::IWriteFileHandle& file) const;

        void dumpTables(base::StringBuilder& str) const;
        void dumpDir(base::StringBuilder& str, uint32_t depth, uint32_t index) const;

        void buildMaps();

        friend class FileSystemDataBuilder;
    };

} // hl2