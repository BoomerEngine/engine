/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#include "build.h"
#include "fileTables.h"

BEGIN_BOOMER_NAMESPACE()

const uint32_t FileTables::FILE_MAGIC = 0x49524346; // 'IRCF';
const uint32_t FileTables::FILE_VERSION_MIN = 1;
const uint32_t FileTables::FILE_VERSION_MAX = VER_CURRENT;

//---

bool FileTables::ValidateHeader(const Header& header, uint32_t sizeOfData)
{
    if (sizeOfData != sizeof(header))
        return false;

    if (header.magic != FILE_MAGIC)
        return false;

    if (header.version < FILE_VERSION_MIN || header.version > FILE_VERSION_MAX)
        return false;

    const auto headerCRC = CalcHeaderCRC(header);
    if (headerCRC != header.crc)
        return false;
            
    return true;
}

bool FileTables::validate(uint64_t memorySize) const
{
    // no data or not enough data
    if (memorySize < sizeof(Header))
    {
        TRACE_WARNING("Not enough bytes for a file header ({} < {})", memorySize, sizeof(Header));
        return false;
    }

    // check header magic
    const auto* headerData = header();
    if (headerData->magic != FILE_MAGIC)
    {
        TRACE_WARNING("Invalid header magic {} != {}", Hex(headerData->magic), Hex(FILE_MAGIC));
        return false;
    }

    // check version range
    if (headerData->version < FILE_VERSION_MIN || headerData->version > FILE_VERSION_MAX)
    {
        TRACE_WARNING("Invalid header version {}}", headerData->version);
        return false;
    }

    // check header CRC value
    const auto headerCRC = CalcHeaderCRC(*headerData);
    if (headerCRC != headerData->crc)
    {
        TRACE_WARNING("Invalid header CRC {} != {}", Hex(headerCRC), Hex(headerData->crc));
        return false;
    }

    // check that data for all chunks if within the block we read
    const auto numChunks = (uint8_t)ChunkType::MAX;
    for (uint32_t i = 0; i < numChunks; ++i)
    {
        const auto& chunk = headerData->chunks[i];
        if (chunk.offset + chunk.size > memorySize)
        {
            TRACE_WARNING("Chunk {} spans past the end of the memory block: {} + {} > {}", i, chunk.offset, chunk.size, memorySize);
            return false;
        }

        if (headerData->flags & FileTables::FileFlag_ProtectedLayout)
        {
            const auto* chunkData = (const uint8_t*)this + chunk.offset;
            const auto chunkCRC = CRC32().append(chunkData, chunk.size).crc();
            if (chunkCRC != chunk.crc)
            {
                TRACE_WARNING("Chunk {} has invalid CRC: {} != {}", i, Hex(chunkCRC), Hex(chunk.crc));
                return false;
            }
        }
    }

    // clear imports
    if (headerData->version < VER_NEW_RESOURCE_ID)
    {
        auto& nonConstChunk = const_cast<Chunk&>(header()->chunks[(int)ChunkType::Imports]);
        nonConstChunk.count = 0;
    }

    // check all name entries
    {
        const auto count = chunkCount(ChunkType::Names);
        const auto stringCount = chunkCount(ChunkType::Strings);
        const auto* ptr = nameTable();
        for (uint32_t i = 0; i < count; ++i, ptr++)
        {
            if (ptr->stringIndex >= stringCount)
            {
                TRACE_WARNING("Name entry {} points to stirng table at {} that is outside it's size {}", i, ptr->stringIndex, stringCount);
                return false;
            }
        }
    }

    // check all type entries
    {
        const auto count = chunkCount(ChunkType::Types);
        const auto nameCount = chunkCount(ChunkType::Names);
        const auto* ptr = typeTable();
        for (uint32_t i = 0; i < count; ++i, ptr++)
        {
            if (ptr->nameIndex >= nameCount)
            {
                TRACE_WARNING("Type entry {} points to name table at {} that is outside it's size {}", i, ptr->nameIndex, nameCount);
                return false;
            }
        }
    }

    // check all properties
    {
        const auto count = chunkCount(ChunkType::Properties);
        const auto nameCount = chunkCount(ChunkType::Names);
        const auto typeCount = chunkCount(ChunkType::Types);
        const auto* ptr = propertyTable();
        for (uint32_t i = 0; i < count; ++i, ptr++)
        {
            if (ptr->classTypeIndex >= typeCount)
            {
                TRACE_WARNING("Property entry {} points to type table at {} that is outside it's size {}", i, ptr->classTypeIndex, typeCount);
                return false;
            }

            /*if (ptr->typeIndex >= typeCount)
            {
                TRACE_WARNING("Property entry {} points to type table at {} that is outside it's size {}", i, ptr->typeIndex, typeCount);
                return false;
            }*/

            if (ptr->nameIndex >= nameCount)
            {
                TRACE_WARNING("Property entry {} points to name table at {} that is outside it's size {}", i, ptr->nameIndex, nameCount);
                return false;
            }
        }
    }

    // check all exports
    {
        const auto count = chunkCount(ChunkType::Exports);
        const auto typeCount = chunkCount(ChunkType::Types);
        const auto* ptr = exportTable();
        for (uint32_t i = 0; i < count; ++i, ptr++)
        {
            if (ptr->classTypeIndex >= typeCount)
            {
                TRACE_WARNING("Export entry {} points to type table at {} that is outside it's size {}", i, ptr->classTypeIndex, typeCount);
                return false;
            }

            if (ptr->parentIndex > i)
            {
                TRACE_WARNING("Export entry {} points to export table at {} that is not preceeding it in the file", i, ptr->parentIndex);
                return false;
            }
        }
    }

    // check all imports
    {
        const auto count = chunkCount(ChunkType::Imports);
        const auto typeCount = chunkCount(ChunkType::Types);
        const auto* ptr = importTable();
        for (uint32_t i = 0; i < count; ++i, ptr++)
        {
            if (ptr->classTypeIndex >= typeCount)
            {
                TRACE_WARNING("Import entry {} points to type table at {} that is outside it's size {}", i, ptr->classTypeIndex, typeCount);
                return false;
            }
        }
    }

    // tables seem valid
    return true;
}

//---

FileTables::CRCValue FileTables::CalcHeaderCRC(const Header& header)
{
    // tricky bit - the CRC field cannot be included in the CRC calculation - calculate the CRC without it
    Header tempHeader;
    tempHeader = header;
    tempHeader.crc = 0xDEADBEEF; // special hacky stuff

    return CRC32().append(&tempHeader, sizeof(tempHeader)).crc();
}

//--

END_BOOMER_NAMESPACE()
