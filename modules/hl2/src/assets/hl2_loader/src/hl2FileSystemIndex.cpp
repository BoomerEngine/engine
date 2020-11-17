/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "hl2FileSystemIndex.h"

#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/io/include/timestamp.h"
#include "base/containers/include/stringBuilder.h"

namespace hl2
{

    ///--

    namespace vpk
    {
#pragma pack(1)

        struct VPKHeader
        {
            uint32_t uiSignature;			// Always 0x55aa1234.
			uint32_t uiVersion;
			uint32_t uiDirectoryLength;
        };

        // Added in version 2.
        struct VPKExtendedHeader
        {
			uint32_t uiDummy0;
			uint32_t uiArchiveHashLength;
			uint32_t uiExtraLength;		// Looks like some more MD5 hashes.
			uint32_t uiDummy1;
        };

        struct VPKDirectoryEntry
        {
			uint32_t uiCRC;
            uint16_t uiPreloadBytes;
			uint16_t uiArchiveIndex;
			uint32_t uiEntryOffset;
			uint32_t uiEntryLength;
			uint16_t uiDummy0;			// Always 0xffff.
        };

        // Added in version 2.
        struct VPKArchiveHash
        {
			uint32_t uiArchiveIndex;
			uint32_t uiArchiveOffset;
			uint32_t uiLength;
            uint8_t lpHash[16];			// MD5
        };

        struct VBSPLump
        {
            uint32_t	uiOffset;
            uint32_t	uiLength;
            uint32_t	uiVersion;							// Default to zero.
            char	lpFourCC[4];						// Default to ( char )0, ( char )0, ( char )0, ( char )0.
        };

        struct VBSPHeader
        {
            unsigned int lpSignature;					// BSP file signature.
            int		iVersion;						// BSP file version.
            VBSPLump	lpLumps[64];	                // Lumps.
            int		iMapRevision;					// The map's revision (iteration, version) number.
        };

        struct ZIPEndOfCentralDirectoryRecord
        {
            uint32_t uiSignature; // 4 bytes (0x06054b50)
            uint16_t uiNumberOfThisDisk;  // 2 bytes
            uint16_t uiNumberOfTheDiskWithStartOfCentralDirectory; // 2 bytes
            uint16_t uiCentralDirectoryEntriesThisDisk;	// 2 bytes
            uint16_t uiCentralDirectoryEntriesTotal;	// 2 bytes
            uint32_t uiCentralDirectorySize; // 4 bytes
            uint32_t uiStartOfCentralDirOffset; // 4 bytes
            uint16_t uiCommentLength; // 2 bytes
            // zip file comment follows
        };

        struct ZIPFileHeader
        {
            uint32_t uiSignature; //  4 bytes (0x02014b50)
            uint16_t uiVersionMadeBy; // version made by 2 bytes
            uint16_t uiVersionNeededToExtract; // version needed to extract 2 bytes
            uint16_t uiFlags; // general purpose bit flag 2 bytes
            uint16_t uiCompressionMethod; // compression method 2 bytes
            uint16_t uiLastModifiedTime; // last mod file time 2 bytes
            uint16_t uiLastModifiedDate; // last mod file date 2 bytes
            uint32_t uiCRC32; // crc-32 4 bytes
            uint32_t uiCompressedSize; // compressed size 4 bytes
            uint32_t uiUncompressedSize; // uncompressed size 4 bytes
            uint16_t uiFileNameLength; // file name length 2 bytes
            uint16_t uiExtraFieldLength; // extra field length 2 bytes
            uint16_t uiFileCommentLength; // file comment length 2 bytes
            uint16_t uiDiskNumberStart; // disk number start 2 bytes
            uint16_t uiInternalFileAttribs; // internal file attributes 2 bytes
            uint32_t uiExternalFileAttribs; // external file attributes 4 bytes
            uint32_t uiRelativeOffsetOfLocalHeader; // relative offset of local header 4 bytes
            // file name (variable size)
            // extra field (variable size)
            // file comment (variable size)
        };

        struct ZIPLocalFileHeader
        {
            uint32_t uiSignature; //local file header signature 4 bytes (0x04034b50)
            uint16_t uiVersionNeededToExtract; // version needed to extract 2 bytes
            uint16_t uiFlags; // general purpose bit flag 2 bytes
            uint16_t uiCompressionMethod; // compression method 2 bytes
            uint16_t uiLastModifiedTime; // last mod file time 2 bytes
            uint16_t uiLastModifiedDate; // last mod file date 2 bytes
            uint32_t uiCRC32; // crc-32 4 bytes
            uint32_t uiCompressedSize; // compressed size 4 bytes
            uint32_t uiUncompressedSize; // uncompressed size 4 bytes
            uint16_t uiFileNameLength; // file name length 2 bytes
            uint16_t uiExtraFieldLength; // extra field length 2 bytes
            // file name (variable size)
            // extra field (variable size)
            // file data (variable size)
        };
#pragma pack()
    }

    ///--

    class FileSystemDataBuilder : public base::NoCopy
    {
    public:
        FileSystemDataBuilder(FileSystemIndex& data)
            : m_data(data)
        {
            // empty string
            data.m_stringTable.reset();
            data.m_stringTable.pushBack(0);

            // reserve
            m_lastDirEntries.reserve(10000);
            m_lastFileEntries.reserve(10000);
        }

        uint32_t mapString(const char* str)
        {
            // empty string always maps to zero
            if (!str || !*str)
                return 0;

            // add to table
            auto length = strlen(str) + 1;
            auto ptr  = m_data.m_stringTable.allocateUninitialized(length);
            memcpy(ptr, str, length);

            // map for future use
            auto offset = range_cast<uint32_t>(ptr - m_data.m_stringTable.typedData());
            return offset;
        }

        uint32_t mapPhysicalFile(const base::StringBuf& filePath, const base::StringBuf& virtualDirectoryPath)
        {
            // get timestamp
            base::io::TimeStamp timeStamp;
            if (!base::io::FileTimeStamp(filePath, timeStamp))
                return 0;

            // get size of the file
            uint64_t fileSize = 0;
            if (!base::io::FileSize(filePath, fileSize))
                return 0;

            // calculate file CRC
            uint64_t crc = 0;

            // create virtual path
            auto fileName = base::StringBuf(filePath.view().fileName());
            auto virtualFilePath = base::TempString("{}/{}", virtualDirectoryPath, fileName);

            // create package entry
            auto packageIndex = m_data.m_packages.size();
            auto& entry = m_data.m_packages.emplaceBack();
            entry.m_name = mapString(virtualFilePath.c_str());
            entry.m_numFiles = 0;
            entry.m_timeStamp = timeStamp.value();

            // create a file entry
            auto dirIndex = createPath(virtualDirectoryPath.c_str());
            mapFile(dirIndex, fileName.c_str(), packageIndex, 0, fileSize, crc);

            // return the package index for the physical file
            return packageIndex;
        }

        uint32_t mapArchive(const base::StringBuf& dirPath, int archiveIndex)
        {
            // get timestamp
            base::io::TimeStamp timeStamp;

            // format package name
            const auto packageName = dirPath.view().fileName();
            if (archiveIndex >= 0)
            {
                char archiveIndexStr[32];
                sprintf(archiveIndexStr, "%03d", archiveIndex);

                auto coreName = packageName.beforeLast("_dir");
                base::StringBuf packageFilePath = base::TempString("{}{}_{}.vpk", dirPath.view().baseDirectory(), coreName, (char*)archiveIndexStr);
                if (!base::io::FileTimeStamp(packageFilePath, timeStamp))
                {
                    TRACE_WARNING("Referenced package '{}' does not exist", packageFilePath);
                }
            }
            else
            {
                base::io::FileTimeStamp(dirPath, timeStamp);
            }

            auto& entry = m_data.m_packages.emplaceBack();
            entry.m_name = mapString(TempString("{}", packageName));
            entry.m_numFiles = 0;
            entry.m_timeStamp = timeStamp.value();

            return (uint32_t)m_data.m_packages.lastValidIndex();
        }

        void buildDirectoryPathString(int dirIndex, base::StringBuilder& str)
        {
            if (dirIndex > 0)
            {
                auto &dirEntry = m_data.m_directories[dirIndex];

                if (dirEntry.m_parent != 0)
                    buildDirectoryPathString(dirEntry.m_parent, str);

                auto name = m_data.m_stringTable.typedData() + dirEntry.m_name;
                str.append(name);
                str.append("/");
            }
        }

        void buildFilePathString(int fileIndex, base::StringBuilder& str)
        {
            auto& fileEntry = m_data.m_files[fileIndex];
            buildDirectoryPathString(fileEntry.m_parent, str);

            auto name  = m_data.m_stringTable.typedData() + fileEntry.m_name;
            str.append(name);
        }

        int mapDirectory(int parent, const char* name)
        {
            // create entry
            auto& entry = m_data.m_directories.emplaceBack();
            entry.m_name = mapString(name);
            entry.m_parent = parent;
            entry.m_nextDir = -1;
            entry.m_firstFile = -1;
            entry.m_firstDir = -1;
            entry.m_pathHash = 0;

            // link
            auto index = m_data.m_directories.lastValidIndex();
            auto& parentEntry = m_data.m_directories[parent];
            if (m_lastDirEntries[parent] == -1)
            {
                parentEntry.m_firstDir = index;
            }
            else
            {
                auto& lastDirEntry = m_data.m_directories[m_lastDirEntries[parent]];
                lastDirEntry.m_nextDir = index;
            }

            // set hash
            base::StringBuilder str;
            buildDirectoryPathString(index, str);
            entry.m_pathHash = base::CRC64().append(str.c_str(), strlen(str.c_str())).crc();

            // remember
            m_lastDirEntries[parent] = index;
            m_lastDirEntries.pushBack(-1);
            m_lastFileEntries.pushBack(-1);
            return index;
        }

        int mapFile(int parent, const char* name, uint32_t packageIndex, uint32_t dataOffset, uint32_t dataSize, uint64_t dataCRC)
        {
            // create entry
            auto& entry = m_data.m_files.emplaceBack();
            entry.m_name = mapString(name);
            entry.m_dataSize = dataSize;
            entry.m_dataCRC = dataCRC;
            entry.m_dataOffset = dataOffset;
            entry.m_nextFile = -1;
            entry.m_packageIndex = packageIndex;
            entry.m_parent = parent;

            // link
            auto index = m_data.m_files.lastValidIndex();
            auto& parentEntry = m_data.m_directories[parent];
            if (m_lastFileEntries[parent] == -1)
            {
                parentEntry.m_firstFile = index;
            }
            else
            {
                auto& lastFileEntry = m_data.m_files[m_lastFileEntries[parent]];
                lastFileEntry.m_nextFile = index;
            }

            // set hash
            base::StringBuilder str;
            buildFilePathString(index, str);
            entry.m_pathHash = base::CRC64().append(str.c_str(), strlen(str.c_str())).crc();
           // TRACE_INFO("File '{}' hash %08llX", str, entry.pathHash);

            // remember
            m_lastFileEntries[parent] = index;
            return index;
        }

        int createDirectory(int parent, const char* name)
        {
            // find in existing dirs
            auto& existingParentDir = m_data.m_directories[parent];
            auto dirIndex = existingParentDir.m_firstDir;
            while (dirIndex != -1)
            {
                auto& existingChildDir = m_data.m_directories[dirIndex];
                auto existingChildDirName  = m_data.m_stringTable.typedData() + existingChildDir.m_name;
                if (0 == _stricmp(name, existingChildDirName))
                    return dirIndex;

                dirIndex = existingChildDir.m_nextDir;
            }

            // create entry
            return mapDirectory(parent, name);
        }

        int createPath(const char* path)
        {
            int cur = 0; // root

            // parse the path
            base::PathEater<char> parser(path, true);
            while (!parser.endOfPath())
            {
                cur = createDirectory(cur, base::TempString("{}", parser.eatDirectoryName()).c_str());
            }

            return cur;
        }

        bool readSafe(const base::io::ReadFileHandlePtr& file, void* data, uint32_t size)
        {
            memset(data, 0, size);
            if (size != file->readSync(data, size))
            {
                TRACE_ERROR("IO error {} from VPK file", size);
                return false;
            }

            return true;
        }

        template< typename T >
        bool readSafe(const base::io::ReadFileHandlePtr& file, T& data)
        {
            return readSafe(file, &data, sizeof(T));
        }

        const char* consumeString(const uint8_t*& data)
        {
            // end fo string
            const char* txt = nullptr;
            if (*data)
            {
                // extract string
                txt = (const char *) data;
                while (*data)
                    ++data;
            }

            // skip the zero
            ++data;
            return txt;
        }

        template< typename T >
        const T* consumeData(const uint8_t*& data)
        {
            auto ptr  = (const T*)data;
            data += sizeof(T);
            return ptr;
        }

        uint32_t mapLocalPackage(const base::StringBuf& path, base::Array<int>& localMap, uint16_t packageIndex)
        {
            if (packageIndex == 0x7fff)
            {
                if (localMap[0] == -1)
                    localMap[0] = mapArchive(path, -1);

                return localMap[0];
            }
            else
            {
                if (localMap[packageIndex+1] == -1)
                    localMap[packageIndex+1] = mapArchive(path, packageIndex);

                return localMap[packageIndex+1];
            }
        }

        bool processDirectoryVPK(const base::StringBuf& packageFilePath)
        {
            // open the archive file
            auto file = base::io::OpenForReading(packageFilePath);
            if (!file)
                return false;

            // read the file header
            vpk::VPKHeader header;
            if (!readSafe(file, header))
            {
                TRACE_ERROR("Unable to read header");
                return false;
            }

            // check the magic
            if (header.uiSignature != 0x55aa1234)
            {
                TRACE_ERROR("Invalid VPK magic");
                return false;
            }

            // invalid version
            if (header.uiVersion > 2)
            {
                TRACE_ERROR("Unrecognized VPK version {}", header.uiVersion);
                return false;
            }

            // load the extended header if required
            TRACE_INFO("VPK version {}", header.uiVersion);
            if (header.uiVersion >= 2)
            {
                vpk::VPKExtendedHeader extendedHeader;
                if (!readSafe(file, extendedHeader))
                    return false;
            }

            // load the data
            base::Array<uint8_t> directoryData;
            auto dataDirectoryOffset = file->pos();
            directoryData.resize(header.uiDirectoryLength);
            if (!readSafe(file, directoryData.data(), directoryData.dataSize()))
                return false;

            // archives
            base::Array<int> archiveMap;
            archiveMap.resizeWith(100, -1);

            // create root directory
            auto& rootDir = m_data.m_directories.emplaceBack();
            rootDir.m_name = 0;
            rootDir.m_parent = -1;
            rootDir.m_firstDir = -1;
            rootDir.m_firstFile = -1;
            rootDir.m_nextDir = -1;
            m_lastDirEntries.pushBack(-1);
            m_lastFileEntries.pushBack(-1);

            // offset to data
            auto dataStorageOffset = file->pos();

            // consume data
            uint32_t numFiles = 0;
            const auto* base  = directoryData.typedData();
            const auto* pos  = directoryData.typedData();
            const auto* end  = directoryData.typedData() + directoryData.dataSize();
            while (pos < end)
            {
                auto ext  = consumeString(pos);
                if (!ext)
                    break;

                // path part
                while (pos < end)
                {
                    auto path  = consumeString(pos);
                    if (!path)
                        break;

                    // trip
                    while (*path == ' ')
                        path += 1;

                    // create directory entry
                    auto dirEntryIndex = createPath(path);
                    const auto& dirEntry = m_data.m_directories[dirEntryIndex];

                    // name part
                    while (pos < end)
                    {
                        auto name = consumeString(pos);
                        if (!name)
                            break;

                        // get the file entry
                        auto vpkEntry = consumeData<vpk::VPKDirectoryEntry>(pos);

                        // map package index
                        uint32_t dataOffset = 0;
                        if (vpkEntry->uiArchiveIndex == 0x7FFF)
                        {
                            dataOffset = dataStorageOffset + vpkEntry->uiEntryOffset;

                            if (dataOffset + vpkEntry->uiEntryLength > file->size())
                            {
                                TRACE_WARNING("File '{}' is outside the package boundary", base::TempString("{}.{}", name, ext));
                                continue;
                            }
                        }
                        else if (vpkEntry->uiPreloadBytes > 0)
                        {
                            dataOffset = dataDirectoryOffset + (pos - base);
                            pos += vpkEntry->uiEntryLength;
                        }
                        else
                        {
                            dataOffset = vpkEntry->uiEntryOffset;
                        }

                        // create the file entry
                        auto packageIndex = mapLocalPackage(packageFilePath, archiveMap, vpkEntry->uiArchiveIndex);
                        mapFile(dirEntryIndex, base::TempString("{}.{}", name, ext), packageIndex, dataOffset, vpkEntry->uiEntryLength, vpkEntry->uiCRC);

                        // count files in the package
                        auto& packageEntry = m_data.m_packages[packageIndex];
                        packageEntry.m_numFiles += 1;
                        numFiles += 1;
                    }
                }
            }

            // report number of extracted files
            TRACE_INFO("Found {} files in directory '{}'", numFiles, packageFilePath);
            return true;
        }

        bool processBSPMapFile(uint32_t packageIndex, const base::StringBuf& packageFilePath)
        {
            // open the archive file
            auto file = base::io::OpenForReading(packageFilePath);
            if (!file)
                return false;

            // read the file header
            vpk::VBSPHeader header;
            if (!readSafe(file, header))
                return false;

            // not a BSP file ?
            if (header.lpSignature != 0x50534256)
            {
                TRACE_ERROR("File '{}' is not a valid BSP file", packageFilePath);
                return false;
            }

            // do we have zip file ?
            auto& zipLump = header.lpLumps[40];
            if (zipLump.uiLength == 0)
            {
                TRACE_INFO("File '{}' has no inner ZIP file", packageFilePath);
                return true;
            }

            // go to the lump location and load the zip header
            file->pos(zipLump.uiOffset + zipLump.uiLength - sizeof(vpk::ZIPEndOfCentralDirectoryRecord));
            vpk::ZIPEndOfCentralDirectoryRecord zipCD;
            if (!readSafe(file, zipCD))
                return false;

            // extract zip
            /*{
              base::Array<uint8_t> data;
              data.resize(zipLump.uiLength);
              file->pos(zipLump.uiOffset);
              file->readSync(data.data(), zipLump.uiLength);

              {
                  auto absPath = base::io::AbsolutePath::Build(L"/home/rexdex/hl2.zip");
                  base::io::SaveFileFromBuffer(absPath, data.data(), data.dataSize());
              }
            }*/

            // process files
            file->pos(zipLump.uiOffset);
            uint32_t numFiles = 0;
            auto endOffset = zipLump.uiOffset + zipLump.uiLength - sizeof(vpk::ZIPEndOfCentralDirectoryRecord);
            while (file->pos() + sizeof(vpk::ZIPLocalFileHeader) < endOffset)
            {
                // load local file header
                vpk::ZIPLocalFileHeader zipFileHeader;
                if (!readSafe(file, zipFileHeader))
                    return false;

                // not a valid file info ?
                if (zipFileHeader.uiSignature != 0x04034b50)
                {
                    TRACE_ERROR("Found invalid file entry after {} file(s) in inner ZIP for '{}'", numFiles, packageFilePath);
                    break;
                }

                // load file name
                base::Array<char> fileNameString;
                fileNameString.resize(zipFileHeader.uiFileNameLength);
                file->readSync(fileNameString.data(), zipFileHeader.uiFileNameLength);
                fileNameString.pushBack(0);
                //TRACE_INFO("Found file '{}' size {} in inner ZIP file of '{}'", fileNameString.typedData(), zipFileHeader.uiCompressedSize, packageFilePath);

                // skip the extra data
                file->pos(file->pos() + zipFileHeader.uiExtraFieldLength);

                // we can only handle uncompressed data
                if (zipFileHeader.uiCompressedSize == zipFileHeader.uiUncompressedSize)
                {
                    // map path
                    base::StringBuf fullFileName(fileNameString.typedData());
                    auto directoryName = fullFileName.stringBeforeLast("/");
                    auto dirIndex = createPath(directoryName.c_str());

                    // create file entry
                    auto fileName = fullFileName.stringAfterLast("/", true);
                    mapFile(dirIndex, fileName.c_str(), packageIndex, file->pos(), zipFileHeader.uiCompressedSize, zipFileHeader.uiCRC32);
                }

                // skip file data
                file->pos(file->pos() + zipFileHeader.uiCompressedSize);
                numFiles += 1;
            }

            // file processed
            TRACE_INFO("Found {} file(s) in inner ZIP of '{}'", numFiles, packageFilePath);
            return true;
        }

    private:
        FileSystemIndex& m_data;

        base::Array<int> m_lastDirEntries;
        base::Array<int> m_lastFileEntries;
    };

    ///--

    FileSystemIndex::FileSystemIndex()
    {
        m_stringTable.reserve(1 << 20);
        m_packages.reserve(64);
        m_files.reserve(100000);
        m_directories.reserve(10000);
    }

    FileSystemIndex::~FileSystemIndex()
    {
    }

    struct Header
    {
        static const uint32_t MAGIC = 0x45464786;
        uint32_t m_magic;
    };

    template< typename T >
    static bool writeTable(base::io::IWriteFileHandle& file, const base::Array<T>& data)
    {
        uint32_t dataSize = data.dataSize();
        if (sizeof(uint32_t) != file.writeSync(&dataSize, sizeof(dataSize)))
            return false;
        if (dataSize != file.writeSync(data.data(), dataSize))
            return false;
        return true;
    }

    template< typename T >
    static bool readTable(base::io::IReadFileHandle& file, base::Array<T>& data)
    {
        uint32_t dataSize = 0;
        if (sizeof(uint32_t) != file.readSync(&dataSize, sizeof(dataSize)))
            return false;
        data.resize(dataSize / sizeof(T));
        if (dataSize != file.readSync(data.data(), dataSize))
            return false;
        return true;
    }

    bool FileSystemIndex::load(base::io::IReadFileHandle& file)
    {
        // write header
        Header header;
        memset(&header, 0, sizeof(header));
        if (sizeof(header) != file.readSync(&header, sizeof(header)))
            return false;
        if (header.MAGIC != header.m_magic)
            return false;

        // load tables
        if (!readTable(file, m_stringTable)) return false;
        if (!readTable(file, m_packages)) return false;
        if (!readTable(file, m_files)) return false;
        if (!readTable(file, m_directories)) return false;
        return true;
    }

    bool FileSystemIndex::save(base::io::IWriteFileHandle& file) const
    {
        // write header
        Header header;
        header.m_magic = Header::MAGIC;
        file.writeSync(&header, sizeof(header));

        // write tables
        if (!writeTable(file, m_stringTable)) return false;
        if (!writeTable(file, m_packages)) return false;
        if (!writeTable(file, m_files)) return false;
        if (!writeTable(file, m_directories)) return false;
        return true;
    }

    void FileSystemIndex::dumpTables(base::StringBuilder& str) const
    {
        str.appendf("HL2FileSystem {} files, {} directories, {} packages\n", m_files.size(), m_directories.size(), m_packages.size());
        if (!m_directories.empty())
            dumpDir(str, 2, 0);
    }

    void FileSystemIndex::dumpDir(base::StringBuilder& str, uint32_t depth, uint32_t index) const
    {
        auto& dirEntry = m_directories[index];
        str.appendPadding(' ', depth);

        auto name  = m_stringTable.typedData() + dirEntry.m_name;
        str.appendf("Dir[{}]: {}, hash {}\n", index, name, Hex(dirEntry.m_pathHash));

        for (auto childDirIndex = dirEntry.m_firstDir; childDirIndex != -1; )
        {
            dumpDir(str, depth+2, childDirIndex);
            childDirIndex = m_directories[childDirIndex].m_nextDir;
        }

        for (auto fileIndex = dirEntry.m_firstFile; fileIndex != -1; )
        {
            auto& fileEntry = m_files[fileIndex];

            auto fileName  = m_stringTable.typedData() + fileEntry.m_name;
            str.appendPadding(' ', depth+2);
            str.appendf("File[{}]: {}, hash {}, offset {}, size {}, archive {}, crc {}\n",
                fileIndex, fileName, Hex(fileEntry.m_pathHash), fileEntry.m_dataOffset, fileEntry.m_dataSize, fileEntry.m_packageIndex, Hex(fileEntry.m_dataCRC));
            fileIndex = fileEntry.m_nextFile;
        }
    }

    base::UniquePtr<FileSystemIndex> FileSystemIndex::Load(const base::StringBuf& contentPath)
    {
        auto ret = base::CreateUniquePtr<FileSystemIndex>();

        // try to load directly
        base::StringBuf indexFilePath = base::TempString("{}boomer.fscache", contentPath);
        {
            auto indexFileHandle = base::io::OpenForReading(indexFilePath);
            if (indexFileHandle)
            {
                if (ret->load(*indexFileHandle))
                {
                    ret->buildMaps();
                    return ret;
                }
            }
        }

        {
            FileSystemDataBuilder dataBuilder(*ret);

            // look for the pack files
            {
                base::Array<base::StringBuf> packFileNames;
                base::io::FindLocalFiles(contentPath, "*_dir.vpk", packFileNames);
                TRACE_INFO("Found {} dir VPKs", packFileNames.size());

                // process each VPK
                for (auto &name : packFileNames)
                {
                    base::StringBuf fullPath = base::TempString("{}{}", contentPath, name);
                    if (!dataBuilder.processDirectoryVPK(fullPath))
                    {
                        TRACE_WARNING("Failed to process package '{}', some content will not be avaiable", fullPath);
                        continue;
                    }
                }
            }

            // look for BSP files
            {
                base::StringBuf mapContentPath = base::TempString("{}maps/", contentPath);

                base::Array<base::StringBuf> bspFileNames;
                base::io::FindLocalFiles(mapContentPath, "*.bsp", bspFileNames);
                TRACE_INFO("Found {} BSP files in '{}'", bspFileNames.size(), mapContentPath);

                for (auto &name : bspFileNames)
                {
                    base::StringBuf fullPath = base::TempString("{}{}", mapContentPath, name);

                    // allow to load the BSP file directly for map loading
                    auto packageIndex = dataBuilder.mapPhysicalFile(fullPath, "maps");

                    // process the BSP file for it's hidden treasures (inner ZIP)
                    dataBuilder.processBSPMapFile(packageIndex, fullPath);
                }
            }
        }

        // print stats
        uint32_t totalFilesFound = 0;
        for (auto& packageEntry : ret->m_packages)
        {
            auto name  = ret->m_stringTable.typedData() + packageEntry.m_name;
            TRACE_INFO("Found {} files from '{}'", packageEntry.m_numFiles, name);
            totalFilesFound += packageEntry.m_numFiles;
        }
        TRACE_INFO("Found {} files in total", totalFilesFound);

        // save the dump
        {
            base::StringBuilder builder;
            ret->dumpTables(builder);

            base::StringBuf indexDumpFilePath = base::TempString("{}boomer.fscache.dump", contentPath);
            base::io::SaveFileFromString(indexDumpFilePath, builder.toString());
        }

        // save to file
        {
            auto indexFileHandle = base::io::OpenForWriting(indexFilePath);
            if (indexFileHandle)
            {
                if (!ret->save(*indexFileHandle))
                {
                    TRACE_WARNING("Saving depot index cache failed, mounting next time will be slow");
                }
            }
        }

        // use what we've gathered
        ret->buildMaps();
        return ret;
    }

    void FileSystemIndex::buildMaps()
    {
        base::ScopeTimer timer;

        // map directories
        {
            m_dirMap.reserve(m_directories.size());
            uint32_t index = 0;
            for (auto& dir : m_directories)
            {
                m_dirMap[dir.m_pathHash] = index;
                index += 1;
            }
        }

        // map files
        {
            m_fileMap.reserve(m_files.size());
            uint32_t index = 0;
            for (auto& file : m_files)
            {
                m_fileMap[file.m_pathHash] = index;
                index += 1;
            }
        }

        TRACE_INFO("Map building took {}", TimeInterval(timer.timeElapsed()));
    }

    int FileSystemIndex::findDirectoryEntry(StringView path) const
    {
        if (m_directories.empty())
            return -1;

        if (path.empty())
            return 0;

        uint32_t index = 0;
        if (m_dirMap.find(path.calcCRC64(), index))
            return index;

        return -1;
    }

    int FileSystemIndex::findFileEntry(StringView path) const
    {
        if (m_files.empty())
            return -1;

        uint32_t index = 0;
        auto pathHash = path.calcCRC64();
        if (m_fileMap.find(pathHash, index))
            return index;

        index = 0;
        for (auto& file : m_files)
        {
            if (file.m_pathHash == pathHash)
                return index;
            index += 1;
        }

        return -1;
    }

} // hl2