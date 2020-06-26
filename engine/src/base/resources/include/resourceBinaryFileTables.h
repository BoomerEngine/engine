/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#pragma once

namespace base
{
    namespace res
    {
        namespace binary
        {

            /// File tables for binary serialization format
            class BASE_RESOURCES_API FileTables
            {
            public:
                static const uint32_t FILE_MAGIC;
                static const uint32_t FILE_VERSION;

                // flags for the imports
                enum class ImportFlags : uint16_t
                {
                    Load = FLAG(0),
                    WaitForFinish = FLAG(1),
                };

                // file chunk types
                enum class ChunkType : uint8_t
                {
                    Strings = 0,
                    Names = 1,
                    Paths = 2,
                    Imports = 3,
                    Properties = 4,
                    Exports = 5,
                    Buffers = 6,
                    Sources = 7,

                    MAX = 10,
                };

                // global typedefs
                typedef uint32_t CRCValue;

                // file chunk
                struct Chunk
                {
                    uint32_t m_offset;                // Offset to table
                    uint32_t m_count;             // Number of entries in table
                    CRCValue m_crc;                 // CRC of the data table (after loading)
                };

                // file header
                struct Header
                {
                    uint32_t m_magic;                 // File header
                    uint32_t m_version;                   // Version number
                    uint32_t m_flags;                 // Generic flags

                    uint32_t m_objectsEnd;                // Where the object data ends (deserializable data)
                    uint32_t m_buffersEnd;                // Where the buffer data ends (total file size)

                    CRCValue m_crc;                     // CRC of the header

                    uint32_t m_numChunks;             // Number of valid chunks in file
                    Chunk m_chunks[(uint8_t)ChunkType::MAX]; // Chunks
                };

#pragma pack(push, 2)
                // name data
                struct Name
                {
                    uint32_t      m_string;       // Index in string table (dynamic)
                    uint64_t      m_hash;         // Name hash (static)
                };

                // path
                struct Path
                {
                    uint32_t      m_string;       // Index in string table (dynamic)
                    uint16_t      m_parent;       // Parent path part (0 when empty)
                };

                // import data
                struct Import
                {
                    uint32_t      m_path;         // Import path (index in string table or HASH if the bit it set), TODO: RESIZE TO uint64_t
                    uint16_t      m_className;    // Name of the import class
                    uint16_t      m_flags;        // Import flags
                };

                // export data
                struct Export
                {
                    uint16_t      m_className;            // Export class (index to name table)
                    uint32_t      m_parent;               // Index of the parent object
                    uint32_t      m_dataSize;             // Object data size
                    uint32_t      m_dataOffset;           // Object data offset
                    uint32_t      m_crc;                  // CRC of the object data
                };

                // property info
                struct Property
                {
                    uint16_t      m_className;            // Parent class (index to name table)
                    uint16_t      m_typeName;             // Property type (index to name table)
                    uint16_t      m_propertyName;         // Property name (index to name table)
                    uint16_t      m_flags;                // Custom flags (0)
                    uint64_t      m_hash;                 // Property hash to speedup lookup on loading
                };

                // buffer info
                struct Buffer
                {
                    uint16_t      m_name;                 // Name of the buffer (index to name table) - placeholder for the future
                    uint16_t      m_flags;                // Buffer flags  - placeholder for the future
                    uint64_t      m_dataOffset;           // Offset to the buffer data (if resident)
                    uint64_t      m_dataSizeOnDisk;       // Size of the buffer data on disk
                    uint64_t      m_dataSizeInMemory;     // Size of the buffer data in memory
                    uint64_t      m_crc;                  // Calculated CRC of the buffer's data
                };

                // source asset information
                struct Source
                {
                    uint16_t      m_path;                 // Path to the source file (in the source depot)
                    uint16_t      m_extra1;               // Extra data
                    uint32_t      m_extra2;               // Extra data
                    uint64_t      m_timeStamp;            // Time stamp of the file
                    uint64_t      m_crc;                  // CRC of the file
                };
#pragma pack(pop)

                static_assert(sizeof(Name) == 12, "Structure is read directly into memory. Size cannot be changed without an adapter.");
                static_assert(sizeof(Path) == 6, "Structure is read directly into memory. Size cannot be changed without an adapter.");
                static_assert(sizeof(Import) == 8, "Structure is read directly into memory. Size cannot be changed without an adapter.");
                static_assert(sizeof(Export) == 18, "Structure is read directly into memory. Size cannot be changed without an adapter.");
                static_assert(sizeof(Property) == 16, "Structure is read directly into memory. Size cannot be changed without an adapter.");
                static_assert(sizeof(Source) == 24, "Structure is read directly into memory. Size cannot be changed without an adapter.");

                typedef Array< char >       TStringTable;
                typedef Array< Name >       TNameTable;
                typedef Array< Import >     TImportTable;
                typedef Array< Export >     TExportTable;
                typedef Array< Property >   TPropertyTable;
                typedef Array< Buffer >     TBufferTable;
                typedef Array< Source >     TSourceTable;

                TStringTable            m_strings;
                TNameTable              m_names;
                TImportTable            m_imports;
                TExportTable            m_exports;
                TPropertyTable          m_properties;
                TBufferTable            m_buffers;
                TSourceTable            m_source;

                uint32_t                  m_version;

            public:
                FileTables();

                // Store data to file
                bool save(stream::IBinaryWriter& writer) const;

                // Load data from file, will return false if validation fails
                bool load(stream::IBinaryReader& reader, uint32_t chunkMask = ~0U);

                //--

                // Extract loading dependencies directly, without costly resolve
                bool extractImports(bool includeAsync, Array<stream::LoadingDependency>& outDependencies) const;

            private:
                static CRCValue CalcHeaderCRC(const Header& header);
            };

        } // binary
    } // res
} // base