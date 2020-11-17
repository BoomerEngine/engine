/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#pragma once

#include "resourceFileTables.h"
#include "base/containers/include/hashMap.h"

namespace base
{
    namespace res
    {

        //--

        // builder of the file tables
        class BASE_RESOURCE_API FileTablesBuilder : public NoCopy
        {
        public:
            FileTablesBuilder();
            FileTablesBuilder(const FileTables& tables); // load existing data

            //--

            // write to physical file
            bool write(io::IWriteFileHandle* file, uint32_t headerFlags, uint64_t objectEndPos, uint64_t bufferEndPos) const;

            //--

            int version = VER_CURRENT;

            Array<char> stringTable;
            Array<FileTables::Name> nameTable;
            Array<FileTables::Type> typeTable;
            Array<FileTables::Path> pathTable;
            Array<FileTables::Property> propertyTable;
            Array<FileTables::Import> importTable;
            Array<FileTables::Export> exportTable;

            //--

            HashMap<FileTables::Type, uint32_t> typeMap;
            HashMap<FileTables::Path, uint32_t> pathMap;
            HashMap<FileTables::Property, uint32_t> propertyMap;
            HashMap<FileTables::ImportKey, uint32_t> importMap;

            HashMap<StringBuf, uint32_t> stringRawMap;
            HashMap<StringID, uint32_t> nameRawMap;
            HashMap<StringID, uint32_t> typeRawMap;
            HashMap<StringBuf, uint32_t> pathRawMap;

            //--

            uint32_t mapString(StringView txt);
            uint16_t mapName(StringID name);

            uint16_t mapType(StringID typeName);
            uint16_t mapType(Type type);

            uint16_t mapPath(StringView path);
            uint16_t mapPath(uint16_t parent, StringView elem);

            uint16_t mapProperty(StringID classType, StringID propName);
            uint16_t mapProperty(const FileTables::Property& prop);
            uint16_t mapProperty(const rtti::Property* prop);

            uint16_t mapImport(StringID classType, StringView importPath, bool async);
            uint16_t mapImport(const FileTables::Import& importInfo);
        };

        //--

    } // res
} // base