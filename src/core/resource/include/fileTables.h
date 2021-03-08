/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

/// File tables for binary serialization format
class CORE_RESOURCE_API FileTables
{
public:
    static const uint32_t FILE_MAGIC;
    static const uint32_t FILE_VERSION_MIN;
    static const uint32_t FILE_VERSION_MAX;

    static const uint16_t EMPTY_OBJECT_PARENT = 0xFFFF;

    static const uint16_t ImportFlag_Load = 1;
    static const uint16_t FileFlag_ProtectedLayout = 1;
    static const uint16_t FileFlag_ExtractedBuffers = 2;

    enum class ChunkType : uint8_t
    {
        Strings = 0,
        Names = 1,
        Types = 2,
        Imports = 4,
        Properties = 5,
        Exports = 6,
        Buffers = 7,
        MAX,
    };

    typedef uint32_t CRCValue;

    struct Chunk
    {
        uint32_t offset = 0; // offset to table, always at the start of the file so 32 bits is enough
        uint32_t size = 0; // size of the chunk's data
        uint32_t count = 0; // number of entries
        CRCValue crc; // CRC of the data in the chunk table
    };

    struct Header
    {
        uint32_t magic; // file header
        uint32_t version; // file version number
        uint32_t flags; // file flags - mostly do we have the "protected stream" data

        uint64_t headersEnd; // where the header ends and object data begins (this is the minimal read size to get all the tables and metadata)
        uint64_t objectsEnd; // where the object data ends (this is the data we need to load to load the resource)
        uint64_t buffersEnd; // Where the buffer data ends (total file size - some of this data may be not needed right away)

        CRCValue crc; // CRC of the header

        Chunk chunks[(uint8_t)ChunkType::MAX]; // chunks
    };

#pragma pack(push, 1)
    struct Name
    {
        uint32_t stringIndex = 0;

        INLINE bool operator==(const Name& other) const
        {
            return (other.stringIndex == stringIndex);
        }

        static uint32_t CalcHash(const Name& entry)
        {
            return CRC32() << entry.stringIndex;
        }
    };

    struct Type
    {
        uint32_t nameIndex = 0;

        INLINE bool operator==(const Type& other) const
        {
            return (other.nameIndex == nameIndex);
        }

        static uint32_t CalcHash(const Type& entry)
        {
            return CRC32() << entry.nameIndex;
        }
    };

    struct Import
    {
        uint32_t guid[4];
        uint16_t classTypeIndex = 0;
        uint16_t flags = 0;

        INLINE bool operator==(const Import& other) const
        {
            return (other.classTypeIndex == classTypeIndex) && (guid[0] == other.guid[0]) && (guid[1] == other.guid[1]) && (guid[2] == other.guid[2]) && (guid[3] == other.guid[3]);
        }

        static uint32_t CalcHash(const Import& entry)
        {
            return CRC32() << entry.guid[0] << entry.guid[1] << entry.guid[2] << entry.guid[3] << entry.classTypeIndex;
        }
    };

    struct Export
    {
        uint32_t parentIndex = 0;
        uint16_t classTypeIndex = 0;
        uint32_t dataSize = 0;
        uint64_t dataOffset = 0;
        uint32_t crc = 0;
    };

    struct Property
    {
        uint16_t classTypeIndex = 0;
        uint16_t nameIndex = 0;

        INLINE bool operator==(const Property& other) const
        {
            return (other.classTypeIndex == classTypeIndex) && (nameIndex == other.nameIndex);
        }

        static uint32_t CalcHash(const Property& entry)
        {
            return CRC32() << entry.classTypeIndex << entry.nameIndex;
        }
    };

    struct Buffer
    {
        uint64_t crc = 0;
        uint64_t dataOffset = 0; // zero if extracted away
        uint64_t uncompressedSize = 0;
        uint64_t compressedSize : 56;
        uint64_t compressionType : 8;
    };

#pragma pack(pop)

    static_assert(sizeof(Name) == 4, "Structure is read directly into memory. Size cannot be changed without an adapter.");
    static_assert(sizeof(Type) == 4, "Structure is read directly into memory. Size cannot be changed without an adapter.");
    static_assert(sizeof(Import) == 20, "Structure is read directly into memory. Size cannot be changed without an adapter.");
    static_assert(sizeof(Export) == 22, "Structure is read directly into memory. Size cannot be changed without an adapter.");
    static_assert(sizeof(Property) == 4, "Structure is read directly into memory. Size cannot be changed without an adapter.");
    static_assert(sizeof(Buffer) == 32, "Structure is read directly into memory. Size cannot be changed without an adapter.");

    //--

    // validate that the tables are readable and valid
    // NOTE: as long as we have at least sizeof(Header) bytes read this function is 100% safe to call even with the most corrupted file
    bool validate(uint64_t avaiableSize) const;

    //--

    /// get header
    INLINE const Header* header() const { return (const Header*)this; }

    /// get chunk data  (NOTE: valid only after validate())
    INLINE const void* chunkData(ChunkType type) const { return (const uint8_t*)this + header()->chunks[(int)type].offset; }

    /// get number of entries in chunk
    INLINE const uint32_t chunkCount(ChunkType type) const { return header()->chunks[(int)type].count; }

    /// get the string table (NOTE: valid only after validate())
    INLINE const char* stringTable() const { return (const char*)chunkData(ChunkType::Strings); }

    /// get the string ID table (NOTE: valid only after validate())
    INLINE const Name* nameTable() const { return (const Name*)chunkData(ChunkType::Names); }

    /// get the type table (NOTE: valid only after validate())
    INLINE const Type* typeTable() const { return (const Type*)chunkData(ChunkType::Types); }

    /// get the property table (NOTE: valid only after validate())
    INLINE const Property* propertyTable() const { return (const Property*)chunkData(ChunkType::Properties); }

    /// get the import table (NOTE: valid only after validate())
    INLINE const Import* importTable() const { return (const Import*)chunkData(ChunkType::Imports); }

    /// get the export table (NOTE: valid only after validate())
    INLINE const Export* exportTable() const { return (const Export*)chunkData(ChunkType::Exports); }

    /// get the buffer table (NOTE: valid only after validate())
    INLINE const Buffer* bufferTable() const { return (const Buffer*)chunkData(ChunkType::Buffers); }

    //--
            
    // validate the header only
    static bool ValidateHeader(const Header& header, uint32_t sizeOfData);

    // calculate CRC for the header
    static CRCValue CalcHeaderCRC(const Header& header);
};

//--

END_BOOMER_NAMESPACE()
