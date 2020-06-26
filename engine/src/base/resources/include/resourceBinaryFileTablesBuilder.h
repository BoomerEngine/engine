/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#pragma once

#include "resourceBinaryFileTables.h"
#include "base/containers/include/hashMap.h"

namespace base
{
    namespace res
    {
        namespace binary
        {

            /// Helper class that can be used to build the content of the FileTables
            class BASE_RESOURCES_API FileTablesBuilder
            {
            public:
                FileTablesBuilder(class FileTables& outData);
                ~FileTablesBuilder();

                struct ImportInfo
                {
                    StringBuf m_path;
                    StringID m_className;
                    bool m_mustBeLoaded = false;
                    bool m_mustBeFullyLoaded = false;
                };

                struct ExportInfo
                {
                    StringID m_className = 0;
                    uint32_t m_parent = 0;
                };

                struct BufferInfo
                {
                    uint32_t m_dataSizeInMemory = 0;
                };

                // add name (no mapping)
                uint16_t addName(StringID name);

                // add property (no mapping)
                uint16_t addProperty(const rtti::Property* prop);

                // add import
                uint32_t addImport(const ImportInfo& importInfo);

                // add export
                uint32_t addExport(const ExportInfo& exportInfo);

                // add buffer data
                uint32_t addBuffer(const BufferInfo& bufferInfo);

                // add ANSI string, returns string index
                uint32_t mapString(StringView<char> string);

                // add mapped name, returns name index
                uint16_t mapName(StringID name) const;

                // add mapped path
                uint32_t mapPath(const StringBuf& path);

                // add mapped property, returns property index
                uint16_t mapProperty(const rtti::Property* prop) const;

                // patch export with data offset and size
                void patchExport(uint32_t exportIndex, uint32_t dataOffset, uint32_t dataSize, uint32_t dataCRC);

                // patch buffer with data offset and size
                void patchBuffer(uint32_t bufferIndex, uint64_t dataOffset, uint64_t dataSizeOnDisk, uint64_t dataSizeInMemory, uint64_t bufferCRC);

            private:
                class FileTables*       m_data;

                typedef HashMap< StringBuf, uint32_t > TStringMap;
                typedef HashMap< StringID, uint16_t > TNameMap;
                typedef HashMap< const rtti::Property*, uint16_t > TPropertiesMap;

                TStringMap      m_stringMap;
                TNameMap        m_nameMap;
                TPropertiesMap  m_propertyMap;
            };


        } // binary
    } // res
} // base