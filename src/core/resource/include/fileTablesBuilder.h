/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#pragma once

#include "fileTables.h"

#include "core/containers/include/hashMap.h"
#include "core/system/include/guid.h"

BEGIN_BOOMER_NAMESPACE()

//--

// builder of the file tables
class CORE_RESOURCE_API FileTablesBuilder : public NoCopy
{
public:
    FileTablesBuilder();
    FileTablesBuilder(const FileTables& tables); // load existing data

    //--

    // write to physical file
    bool write(IWriteFileHandle* file, uint32_t headerFlags, uint64_t objectEndPos, uint64_t bufferEndPos, const void* prevHeader = nullptr) const;

    //--

    int version = VER_CURRENT;

    Array<char> stringTable;
    Array<FileTables::Name> nameTable;
    Array<FileTables::Type> typeTable;
    Array<FileTables::Property> propertyTable;
    Array<FileTables::Import> importTable;
    Array<FileTables::Export> exportTable;
    Array<FileTables::Buffer> bufferTable;

    //--

    HashMap<FileTables::Type, uint32_t> typeMap;
    HashMap<FileTables::Property, uint32_t> propertyMap;
    HashMap<FileTables::Import, uint32_t> importMap;
    
    HashMap<StringBuf, uint32_t> stringRawMap;
    HashMap<StringID, uint32_t> nameRawMap;
    HashMap<StringID, uint32_t> typeRawMap;
    HashMap<uint64_t, uint32_t> bufferRawMap;

    //--

    struct BufferData
    {
        uint64_t crc = 0;
        Buffer compressedData;
        CompressionType compressionType = CompressionType::Uncompressed;
        uint64_t uncompressedSize = 0;
    };

    Array<BufferData> bufferData;

    //--

    uint32_t mapString(StringView txt);
    uint16_t mapName(StringID name);

    uint16_t mapType(StringID typeName);
    uint16_t mapType(Type type);

    uint16_t mapProperty(StringID classType, StringID propName);
    uint16_t mapProperty(const FileTables::Property& prop);
    uint16_t mapProperty(const Property* prop);

    uint16_t mapImport(StringID classType, GUID id, bool async);
    uint16_t mapImport(const FileTables::Import& importInfo);

    void initFromTables(const FileTables& tables);
};

//--

END_BOOMER_NAMESPACE()
