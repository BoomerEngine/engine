/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#include "build.h"
#include "resourceBinaryFileTables.h"
#include "resourceBinaryFileTablesBuilder.h"

#include "base/object/include/rttiProperty.h"
#include "base/object/include/rttiType.h"

namespace base
{
    namespace res
    {
        namespace binary
        {

            FileTablesBuilder::FileTablesBuilder(class FileTables& outData)
                : m_data(&outData)
            {
                // cleanup
                m_data->m_buffers.clear();
                m_data->m_buffers.reserve(100);
                m_data->m_exports.clear();
                m_data->m_exports.reserve(1000);
                m_data->m_imports.clear();
                m_data->m_imports.reserve(100);
                m_data->m_names.clear();
                m_data->m_names.reserve(200);
                m_data->m_properties.clear();
                m_data->m_properties.reserve(400);
                m_data->m_strings.clear();
                m_data->m_strings.reserve(4096);

                // allocate empty string entry
                m_data->m_strings.pushBack(0);
            }

            FileTablesBuilder::~FileTablesBuilder()
            {
            }

            uint32_t FileTablesBuilder::mapString(StringView<char> string)
            {
                // empty string always maps to zero
                if (string.empty())
                    return 0;

                // find in map
                uint32_t index = 0;
                if (m_stringMap.find(string, index))
                    return index;

                // append
                uint32_t size = string.length() + 1;
                uint32_t offset = m_data->m_strings.size();
                auto ptr  = m_data->m_strings.allocateUninitialized(size);
                memcpy(ptr, string.data(), string.length());
                ptr[string.length()] = 0;

                // add to map
                m_stringMap.set(StringBuf(string), offset);
                return offset;
            }

            uint32_t FileTablesBuilder::mapPath(const StringBuf& path)
            {
                return mapString(path.view());
            }

            uint16_t FileTablesBuilder::addName(StringID name)
            {
                DEBUG_CHECK_EX(!name.empty(), "Adding empty name");

                // add new name
                FileTables::Name nameData;
                nameData.m_hash = StringID::CalcHash(name);
                nameData.m_string = mapString(name.view());
                m_data->m_names.pushBack(nameData);

                // add to map
                auto index = (uint16_t)m_data->m_names.size();
                m_nameMap.set(name, index);
                return index;
            }

            uint16_t FileTablesBuilder::mapName(StringID name) const
            {
                // null name is always mapped to 0
                if (name.empty())
                    return 0;

                // find in map
                uint16_t index = 0;
                bool found = m_nameMap.find(name, index);
                DEBUG_CHECK_EX(found, "Trying to save unmapped name");

                return index;
            }

            uint16_t FileTablesBuilder::addProperty(const rtti::Property* prop)
            {
                DEBUG_CHECK_EX(prop, "Adding invalid property");

                // add new property
                FileTables::Property propertyData;
                propertyData.m_className = mapName(prop->parent()->name());
                propertyData.m_typeName = mapName(prop->type()->name());
                propertyData.m_propertyName = mapName(prop->name());
                propertyData.m_flags = 0;
                propertyData.m_hash = prop->hash();
                m_data->m_properties.pushBack(propertyData);    

                // add to map
                auto index = (uint16_t)m_data->m_properties.size();
                m_propertyMap.set(prop, index);
                return index;
            }

            uint16_t FileTablesBuilder::mapProperty(const rtti::Property* prop) const
            {
                // null property is always mapped to 0
                if (!prop)
                    return 0;

                // find in map
                uint16_t index = 0;
                bool found = m_propertyMap.find(prop, index);
                DEBUG_CHECK_EX(found, "Trying to save reference to unmapped property");
                return index;
            }

            uint32_t FileTablesBuilder::addImport(const ImportInfo& importInfo)
            {
                // invalid resource path
                if (importInfo.m_path.empty())
                    return 0;

                // create import info
                FileTables::Import importData;
                importData.m_flags = 0;
                importData.m_className = mapName(importInfo.m_className);
                importData.m_path = mapPath(importInfo.m_path);

                // setup flags
                if (importInfo.m_mustBeLoaded)
                    importData.m_flags |= (uint16_t)FileTables::ImportFlags::Load;
                if (importInfo.m_mustBeFullyLoaded)
                    importData.m_flags |= (uint16_t)FileTables::ImportFlags::WaitForFinish;

                // add to list
                m_data->m_imports.pushBack(importData);

                // return assigned index
                return m_data->m_imports.size();
            }

            uint32_t FileTablesBuilder::addExport(const ExportInfo& exportInfo)
            {
                // invalid class name
                if (exportInfo.m_className.empty())
                    return (uint32_t)-1;

                // create export info
                FileTables::Export exportData;
                memzero(&exportData, sizeof(exportData));
                exportData.m_className = mapName(exportInfo.m_className);
                exportData.m_parent = exportInfo.m_parent;

                // add to list
                m_data->m_exports.pushBack(exportData);

                // return assigned index
                return m_data->m_exports.size();
            }

            uint32_t FileTablesBuilder::addBuffer(const BufferInfo& bufferInfo)
            {
                // create export info
                FileTables::Buffer bufferData;
                memzero(&bufferData, sizeof(bufferData));
                bufferData.m_dataSizeInMemory = bufferInfo.m_dataSizeInMemory;
                bufferData.m_dataSizeOnDisk = 0; // not known yet

                // add to list
                m_data->m_buffers.pushBack(bufferData);

                // return assigned index
                return m_data->m_buffers.size();
            }

            void FileTablesBuilder::patchExport(uint32_t exportIndex, uint32_t dataOffset, uint32_t dataSize, uint32_t dataCRC)
            {
                DEBUG_CHECK_EX(exportIndex < m_data->m_exports.size(), "Export index out of range");
                m_data->m_exports[exportIndex].m_dataOffset = dataOffset;
                m_data->m_exports[exportIndex].m_dataSize = dataSize;
                m_data->m_exports[exportIndex].m_crc = dataCRC;
            }

            void FileTablesBuilder::patchBuffer(uint32_t bufferIndex, uint64_t dataOffset, uint64_t dataSizeOnDisk, uint64_t dataSizeInMemory, uint64_t bufferCRC)
            {
                DEBUG_CHECK_EX(bufferIndex >= 0 && bufferIndex < m_data->m_buffers.size(), "Buffer index out of range");

                DEBUG_CHECK_EX(dataSizeOnDisk <= m_data->m_buffers[bufferIndex].m_dataSizeInMemory, "Buffer is smaller in memory than on disk - compression fuckup");
                m_data->m_buffers[bufferIndex].m_dataOffset = dataOffset;
                m_data->m_buffers[bufferIndex].m_dataSizeOnDisk = dataSizeOnDisk;
                m_data->m_buffers[bufferIndex].m_dataSizeInMemory = dataSizeInMemory;
                m_data->m_buffers[bufferIndex].m_crc = bufferCRC;
            }

        } // binary
    } // res
} // base
