/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#include "build.h"
#include "fileTables.h"
#include "fileTablesBuilder.h"
#include "core/io/include/fileHandle.h"
#include "core/object/include/rttiProperty.h"

BEGIN_BOOMER_NAMESPACE()

//--

FileTablesBuilder::FileTablesBuilder()
{
    stringTable.reserve(4096);
    stringTable.pushBack(0);

    nameTable.reserve(64);
    nameTable.emplaceBack();

    typeTable.reserve(64);
    typeTable.emplaceBack();

    importTable.reserve(64);
    exportTable.reserve(64);
    propertyTable.reserve(64);

    stringRawMap.reserve(64);
    nameRawMap.reserve(64);
    typeMap.reserve(64);
    importMap.reserve(64);
}

template< typename T >
static void ExtractChunk(Array<T>& localData, const FileTables& tables, FileTables::ChunkType chunkType)
{
    const auto count = tables.chunkCount(chunkType);
    localData.resize(count);
    memcpy(localData.data(), tables.chunkData(chunkType), sizeof(T) * count);
};

FileTablesBuilder::FileTablesBuilder(const FileTables& tables)
{
    ExtractChunk(stringTable, tables, FileTables::ChunkType::Strings);
    ExtractChunk(nameTable, tables, FileTables::ChunkType::Names);
    ExtractChunk(propertyTable, tables, FileTables::ChunkType::Properties);
    ExtractChunk(importTable, tables, FileTables::ChunkType::Imports);
    ExtractChunk(exportTable, tables, FileTables::ChunkType::Exports);

    //--

    if (stringTable.empty())
        stringTable.pushBack(0);

    const char* strPtr = stringTable.typedData() + 1;
    const char* strEnd = stringTable.typedData() + stringTable.size();
    while (strPtr < strEnd)
    {
        const auto length = strlen(strPtr);
        stringRawMap[StringBuf(strPtr)] = strPtr - stringTable.typedData();
        strPtr += (length + 1);
    }

    //--

    for (uint32_t i = 0; i < nameTable.size(); ++i)
    {
        const auto& nameEntry = nameTable[i];
        const auto* nameString = stringTable.typedData() + nameEntry.stringIndex;
        const auto name = StringID(nameString);
        nameRawMap[name] = i;
    }

    //--
}

//--

uint32_t FileTablesBuilder::mapString(StringView txt)
{
    if (txt.empty())
        return 0;

    uint32_t index = 0;
    if (stringRawMap.find(txt, index))
        return index;

    index = stringTable.size();

    {
        uint32_t size = txt.length() + 1;
        auto ptr = stringTable.allocateUninitialized(size);
        memcpy(ptr, txt.data(), txt.length());
        ptr[txt.length()] = 0;
    }

    stringRawMap.set(StringBuf(txt), index);
    return index;
}

//--

uint16_t FileTablesBuilder::mapName(StringID name)
{
    if (!name)
        return 0;

    uint32_t index = 0;
    if (nameRawMap.find(name, index))
        return index;

    index = nameTable.size();

    auto& nameEntry = nameTable.emplaceBack();
    nameEntry.stringIndex = mapString(name.view());
    nameRawMap[name] = index;

    return index;
}

//--

uint16_t FileTablesBuilder::mapType(Type type)
{
    if (!type)
        return 0;

    return mapType(type->name());
}

uint16_t FileTablesBuilder::mapType(StringID typeName)
{
    if (!typeName)
        return 0;

    uint32_t ret = 0;
    if (typeRawMap.find(typeName, ret))
        return ret;

    FileTables::Type info;
    info.nameIndex = mapName(typeName);

    if (typeMap.find(info, ret))
        return ret;

    ret = typeTable.size();
    typeTable.pushBack(info);
    typeRawMap[typeName] = ret;
    typeMap[info] = ret;

    // TRACE_INFO("Internal Type '{}' mapped to {}", typeName, ret);
    return ret;
}

uint16_t FileTablesBuilder::mapProperty(StringID classType, StringID propName)
{
    if (!classType || !propName)
        return 0;

    FileTables::Property prop;
    prop.classTypeIndex = mapType(classType);
    prop.nameIndex = mapName(propName);
    return mapProperty(prop);
}

uint16_t FileTablesBuilder::mapProperty(const Property* prop)
{
    if (!prop)
        return 0;

    return mapProperty(prop->parent()->name(), prop->name());
}

uint16_t FileTablesBuilder::mapProperty(const FileTables::Property& prop)
{
    if (!prop.classTypeIndex || !prop.nameIndex)
        return 0;

    uint32_t ret = 0;
    if (propertyMap.find(prop, ret))
        return ret;

    ret = propertyTable.size();
    propertyTable.pushBack(prop);
    propertyMap[prop] = ret;
    return ret;
}

uint16_t FileTablesBuilder::mapImport(StringID classType, GUID id, bool async)
{
    if (!classType || !id)
        return 0;

    FileTables::Import importInfo;
    importInfo.classTypeIndex = mapType(classType);
    importInfo.guid[0] = id.data()[0];
    importInfo.guid[1] = id.data()[1];
    importInfo.guid[2] = id.data()[2];
    importInfo.guid[3] = id.data()[3];
    importInfo.flags = async ? 0 : FileTables::ImportFlag_Load;
    return mapImport(importInfo);
}

uint16_t FileTablesBuilder::mapImport(const FileTables::Import& importInfo)
{
    if (!importInfo.classTypeIndex || (!importInfo.guid[0] && !importInfo.guid[1] && !!importInfo.guid[2] && !importInfo.guid[3]))
        return 0;

    uint32_t ret = 0;
    if (importMap.find(importInfo, ret))
    {
        ASSERT(ret != 0);
        auto& entry = importTable[ret - 1];
        entry.flags |= importInfo.flags;
        return ret;
    }

    importTable.pushBack(importInfo);
    ret = importTable.size();

    importMap[importInfo] = ret;
    return ret;
}

//--

void FileTablesBuilder::initFromTables(const FileTables& tables)
{
    stringTable.reset();
    nameTable.reset();
    typeTable.reset();
    importTable.reset();
    exportTable.reset();
    propertyTable.reset();

    stringTable.pushBack(0);
    nameTable.emplaceBack();
    typeTable.emplaceBack();

    {
        const auto numNames = tables.chunkCount(FileTables::ChunkType::Names);
        for (uint32_t i = 1; i < numNames; ++i)
        {
            const auto* str = tables.stringTable() + tables.nameTable()[i].stringIndex;

            auto& entry = nameTable.emplaceBack();
            entry.stringIndex = mapString(str);

            nameRawMap[StringID(str)] = i;
        }
    }

    {
        const auto numTypes = tables.chunkCount(FileTables::ChunkType::Types);
        for (uint32_t i = 1; i < numTypes; ++i)
        {
            auto& entry = typeTable.emplaceBack();
            entry.nameIndex = tables.typeTable()[i].nameIndex;

            const auto* str = tables.stringTable() + tables.nameTable()[entry.nameIndex].stringIndex;
            typeRawMap[StringID(str)] = i;
        }
    }

    {
        const auto numProperties = tables.chunkCount(FileTables::ChunkType::Properties);
        for (uint32_t i = 0; i < numProperties; ++i)
        {
            auto& entry = propertyTable.emplaceBack();
            entry.classTypeIndex = tables.propertyTable()[i].classTypeIndex;
            entry.nameIndex = tables.propertyTable()[i].nameIndex;
            propertyMap[entry] = i;
        }
    }

    {
        const auto numExports = tables.chunkCount(FileTables::ChunkType::Exports);
        for (uint32_t i = 0; i < numExports; ++i)
        {
            const auto& src = tables.exportTable()[i];
            auto& entry = exportTable.emplaceBack();
            entry.classTypeIndex = src.classTypeIndex;
            entry.crc = src.crc;
            entry.parentIndex = src.parentIndex;
            entry.dataOffset = src.dataOffset;
            entry.dataSize = src.dataSize;
        }
    }

    {
        const auto numImports = tables.chunkCount(FileTables::ChunkType::Imports);
        for (uint32_t i = 0; i < numImports; ++i)
        {
            const auto& src = tables.importTable()[i];

            auto& entry = importTable.emplaceBack();
            entry.classTypeIndex = src.classTypeIndex;
            entry.flags = src.flags;
            entry.guid[0] = src.guid[0];
            entry.guid[1] = src.guid[1];
            entry.guid[2] = src.guid[2];
            entry.guid[3] = src.guid[3];
        }
    }
}

//--

template< typename T >
bool WriteChunk(IWriteFileHandle* file, uint64_t baseOffset, const Array<T>& entries, FileTables::Chunk& outChunk)
{
    outChunk.count = entries.size();
    outChunk.size = entries.dataSize();
    outChunk.offset = file->pos() - baseOffset;
    outChunk.crc = CRC32().append(entries.data(), entries.dataSize()).crc();

    return file->writeSync(entries.data(), entries.dataSize()) == entries.dataSize();
}

bool FileTablesBuilder::write(IWriteFileHandle* file, uint32_t headerFlags, uint64_t objectEndPos, uint64_t bufferEndPos, const void* prevHeader) const
{
    const uint64_t baseOffset = file->pos();

    // write the header (placeholder)
    FileTables::Header writeHeader;
    memset(&writeHeader, 0, sizeof(writeHeader));
    if (file->writeSync(&writeHeader, sizeof(writeHeader)) != sizeof(writeHeader))
        return false;

    // write chunks
    if (!WriteChunk(file, baseOffset, stringTable, writeHeader.chunks[(int)FileTables::ChunkType::Strings]))
        return false;
    if (!WriteChunk(file, baseOffset, nameTable, writeHeader.chunks[(int)FileTables::ChunkType::Names]))
        return false;
    if (!WriteChunk(file, baseOffset, typeTable, writeHeader.chunks[(int)FileTables::ChunkType::Types]))
        return false;
    if (!WriteChunk(file, baseOffset, propertyTable, writeHeader.chunks[(int)FileTables::ChunkType::Properties]))
        return false;
    if (!WriteChunk(file, baseOffset, importTable, writeHeader.chunks[(int)FileTables::ChunkType::Imports]))
        return false;
    if (!WriteChunk(file, baseOffset, exportTable, writeHeader.chunks[(int)FileTables::ChunkType::Exports]))
        return false;

    // update header
    writeHeader.magic = FileTables::FILE_MAGIC;
    writeHeader.headersEnd = file->pos() - baseOffset;
    writeHeader.buffersEnd = bufferEndPos;
    writeHeader.objectsEnd = objectEndPos;

    // setup version and flags - if we had previous header use values from it
    if (prevHeader)
    {
        const auto& prevHeaderData = *(const FileTables::Header*)prevHeader;
        writeHeader.version = prevHeaderData.version;
        writeHeader.flags = prevHeaderData.flags;
    }
    else
    {
        writeHeader.version = FileTables::FILE_VERSION_MAX;
        writeHeader.flags = headerFlags;
    }

    writeHeader.crc = FileTables::CalcHeaderCRC(writeHeader);

    // write patched up header again
    {
        auto pos = file->pos();
        file->pos(baseOffset);
        if (file->writeSync(&writeHeader, sizeof(writeHeader)) != sizeof(writeHeader))
            return false;
        file->pos(pos);
    }

    // file tables saved
    return true;
}

//--

END_BOOMER_NAMESPACE()
