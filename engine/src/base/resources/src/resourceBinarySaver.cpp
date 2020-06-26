/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#include "build.h"
#include "resourceBinarySaver.h"
#include "resourceBinaryStructureMapper.h"
#include "resourceBinaryFileTables.h"
#include "resourceBinaryFileTablesBuilder.h"

#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/object.h"

namespace base
{
    namespace res
    {
        namespace binary
        {

            BinarySaver::BinarySaver()
            {
            }

            bool BinarySaver::saveObjects(stream::IBinaryWriter& file, const stream::SavingContext& context)
            {
                // analyze structure of objects to save
                StructureMapper structureMapper;
                structureMapper.mapObjects(context);

                // build serialization tables using the captured data
                FileTables fileTables;
                FileTablesBuilder tableBuilder(fileTables);
                {
                    fileTables.m_names.reserve(structureMapper.m_names.size());
                    fileTables.m_imports.reserve(structureMapper.m_imports.size());
                    fileTables.m_exports.reserve(structureMapper.m_exports.size());
                    fileTables.m_properties.reserve(structureMapper.m_properties.size());
                    fileTables.m_buffers.reserve(structureMapper.m_buffers.size());

                    SetupNames(structureMapper, tableBuilder);
                    SetupProperties(structureMapper, tableBuilder);
                    SetupImports(structureMapper, tableBuilder);
                    SetupExports(structureMapper, tableBuilder);
                    SetupBuffers(structureMapper, tableBuilder);
                }

                // Write the initial header, all offsets are RELATIVE to the header, not to the file start
                uint64_t headerOffset = file.pos();
                fileTables.save(file);

                // copy flags
                //file.m_saveEditorOnlyProperties = context.m_saveEditorOnlyProperties;

                // Write exports
                if (!writeExports(file, context, structureMapper, tableBuilder, headerOffset))
                    return false;

                // Write buffers, NOTE: buffers can be extracted here
                if (!writeBuffers(file, context, structureMapper, tableBuilder, headerOffset))
                    return false;

                // Resave header, now with full set of data
                uint64_t endOffset = file.pos();
                file.seek(headerOffset);
                fileTables.save(file);
                file.seek(endOffset);

                // objects saved
                return true;
            }

            void BinarySaver::SetupNames(StructureMapper& mapper, class FileTablesBuilder& builder)
            {
                for (auto& name : mapper.m_names)
                    builder.addName(name);
            }

            void BinarySaver::SetupImports(StructureMapper& mapper, class FileTablesBuilder& builder)
            {
                // Emit imports into the file tables
                for (auto& import : mapper.m_imports)
                {
                    DEBUG_CHECK_EX(!import.path.empty(), "Import with no resource");

                    FileTablesBuilder::ImportInfo importBuilder;
                    importBuilder.m_mustBeLoaded = !import.async;
                    importBuilder.m_mustBeFullyLoaded = true;
                    importBuilder.m_path = import.path;
                    importBuilder.m_className = import.loadClass->name();

                    builder.addImport(importBuilder);
                }
            }

            void BinarySaver::SetupExports(StructureMapper& mapper, class FileTablesBuilder& builder)
            {
                // Emit export data
                for (uint32_t exportIndex=0; exportIndex<mapper.m_exports.size(); ++exportIndex)
                {
                    auto& obj = mapper.m_exports[exportIndex];

                    // setup export info
                    FileTablesBuilder::ExportInfo exportBuilder;
                    exportBuilder.m_className = obj->cls()->name();
                    
                    // Map parent
                    auto parentObject = obj->parent();
                    if (parentObject)
                    {
                        stream::MappedObjectIndex parentIndex = 0;
                        mapper.m_objectIndices.find(parentObject, parentIndex);

                        exportBuilder.m_parent = parentIndex;
                    }

                    // Register in output data, for now we do not support re indexing
                    builder.addExport(exportBuilder);
                }
            }

            void BinarySaver::SetupBuffers(StructureMapper& mapper, class FileTablesBuilder& builder)
            {
                // Emit buffer data
                for (auto& buf : mapper.m_buffers)
                {
                    // extra paranoia
                    DEBUG_CHECK_EX(buf.crc != 0, "Found mapped buffer with invalid ID");
                    DEBUG_CHECK_EX(buf.size != 0, "Found mapped buffer with zero size");
                    DEBUG_CHECK_EX(buf.data.data() != nullptr, "Defered data buffer got lost before it got saved. File cannot be saved like that. Check your save related logic.");
                    DEBUG_CHECK_EX(buf.data.size() == buf.size, "Defered data buffer's size has changed before it got saved. File cannot be saved like that. Check your save related logic.");

                    // setup buffer info and add it to the builder
                    FileTablesBuilder::BufferInfo bufferBuilder;
                    bufferBuilder.m_dataSizeInMemory = buf.size; // the only thing we know right now is the size in memory :)

                    // add to buffer table
                    builder.addBuffer(bufferBuilder);
                }
            }

            void BinarySaver::SetupProperties(StructureMapper& mapper, class FileTablesBuilder& builder)
            {
                for (auto prop  : mapper.m_properties)
                    builder.addProperty(prop);
            }

            namespace helper
            {
                // mapping used to translate data to already preallocated indices
                class SavingMapper : public stream::IDataMapper, public base::NoCopy
                {
                public:
                    SavingMapper(const StructureMapper& structureMapper, const FileTablesBuilder& fileTables)
                        : m_structureMapper(structureMapper)
                        , m_fileTables(fileTables)
                    {}

                    virtual void mapName(StringID name, stream::MappedNameIndex& outIndex) override final
                    {
                        if (!name.empty())
                            outIndex = m_fileTables.mapName(name);
                        else
                            outIndex = 0;
                    }

                    virtual void mapType(Type rttiType, stream::MappedTypeIndex& outIndex) override final
                    {
                        if (rttiType)
                            outIndex = m_fileTables.mapName(rttiType->name());
                        else
                            outIndex = 0;
                    }

                    virtual void mapProperty(const rtti::Property* rttiProperty, stream::MappedPropertyIndex& outIndex) override final
                    {
                        if (rttiProperty)
                            outIndex = m_fileTables.mapProperty(rttiProperty);
                        else
                            outIndex = 0;
                    }

                    virtual void mapPointer(const IObject* object, stream::MappedObjectIndex& outIndex) override final
                    {
                        outIndex = 0;
                        m_structureMapper.m_objectIndices.find(object, outIndex);
                    }

                    virtual void mapResourceReference(StringView<char> path, ClassType resourceClass, bool async, stream::MappedPathIndex& outIndex) override final
                    {
                        outIndex = 0;
                        m_structureMapper.m_importIndices.find(path, outIndex);
                    }

                    virtual void mapBuffer(const Buffer& data, stream::MappedBufferIndex& outIndex) override final
                    {
                        outIndex = 0;

                        if (data && data.size())
                        {
                            // calculate data CRC - it's the KEY 
                            auto crc = CRC64().append(data.data(), (uint32_t)data.size()).crc();
                            DEBUG_CHECK_EX(crc != 0, "Invalid CRC computed from non-zero buffer content");

                            // find buffer ID
                            if (!m_structureMapper.m_bufferIndices.find(crc, outIndex))
                            {
                                FATAL_ERROR("Trying to save unmapped deferred data buffer. All new content should be generated in OnPreSave().");
                            }
                        }
                    }

                private:
                    const StructureMapper&      m_structureMapper;
                    const FileTablesBuilder&    m_fileTables;
                };
            }

            bool BinarySaver::writeExports(stream::IBinaryWriter& file, const stream::SavingContext& context, const StructureMapper& mapper, FileTablesBuilder& tables, uint64_t headerOffset)
            {
                // bind mapper to file because we want to use indexed tables
                helper::SavingMapper savingMapper(mapper, tables);
                file.m_mapper = &savingMapper;

                for (uint32_t index = 0; index < mapper.m_exports.size(); ++index)
                {
                    auto& exp = mapper.m_exports[index];

                    // Get export offset
                    auto exportDataOffset = (uint32_t)(file.pos() - headerOffset);

                    // Save using the serialization interface
                    exp->onWriteBinary(file);

                    // Calculate data size
                    auto exportDataSize = (uint32_t)(file.pos() - headerOffset) - exportDataOffset;

                    // Patch the export data in the file tables
                    tables.patchExport(index, exportDataOffset, exportDataSize, 0);
                }

                // unbind mapper
                file.m_mapper = nullptr;

                // exports saved
                return true;
            }

            bool BinarySaver::writeBuffers(stream::IBinaryWriter& file, const stream::SavingContext& context, const StructureMapper& mapper, FileTablesBuilder& tables, uint64_t headerOffset)
            {
                for (uint32_t i = 0; i < mapper.m_buffers.size(); ++i)
                {
                    auto& buf = mapper.m_buffers[i];

                    // Sanity checks
                    DEBUG_CHECK_EX(buf.crc != 0, "Mapped buffer with invalid ID");
                    DEBUG_CHECK_EX(buf.size > 0, "Mapped buffers should never be empty");
                    DEBUG_CHECK_EX(buf.data.size() > 0, "Mapped buffers should never be empty");
                    DEBUG_CHECK_EX(buf.data.data() != nullptr, "Mapped buffers should never be empty");

                    // Buffer is not extracted, save it at the end of the file
                    // NOTE: although buffers are not allowed to be bigger than 4GB each the whole file may be bigger than 4GB
                    uint64_t bufferDataOffset = file.pos() - headerOffset;

                    // compress buffer
                    // TODO: select best compression
                    ASSERT(buf.size != 0);
                    uint64_t compressedSize = 0;
                    auto compressedData = mem::Compress(mem::CompressionType::LZ4, buf.data.data(), buf.size, compressedSize, POOL_TEMP);

                    // we expect that the compressed buffer will be at least 90% of the original buffer, otherwise the compression is not worth it
                    auto maximumCompressedSize = (uint64_t)buf.size * 9 / 10;
                    if (!compressedData || (compressedSize > maximumCompressedSize))
                    {
                        TRACE_INFO("Saved buffer {}: raw {} at offset {}", Hex(buf.crc), MemSize(buf.size), bufferDataOffset);

                        // the compression did not work or is not effective
                        ASSERT(buf.data.size() == buf.size);
                        file.write(buf.data.data(), buf.size);
                        if (file.isError())
                            return false;

                        // make sure we advanced properly
                        uint64_t bufferDataEndOffset = file.pos() - headerOffset;
                        if (bufferDataEndOffset != (bufferDataOffset + buf.size))
                        {
                            TRACE_ERROR("Failed to save buffer data of size {} - OUT OF DISK SPACE ?", buf.size);
                            return false;
                        }

                        // Patch up the buffer data in the file tables
                        uint64_t bufferSizeOnDisk = buf.size; // no compression
                        tables.patchBuffer(i, bufferDataOffset, bufferSizeOnDisk, bufferSizeOnDisk, buf.crc);
                    }
                    else
                    {
                        TRACE_INFO("Saved buffer {}: compressed {} -> {} at offset {}", Hex(buf.crc), MemSize(buf.size), MemSize(compressedSize), bufferDataOffset);

                        // write the compressed data
                        file.write(compressedData, compressedSize);
                        if (file.isError())
                            return false;

                        // make sure we advanced properly
                        uint64_t bufferDataEndOffset = file.pos() - headerOffset;
                        if (bufferDataEndOffset != (bufferDataOffset + compressedSize))
                        {
                            TRACE_ERROR("Failed to save buffer data of size {} - OUT OF DISK SPACE ?", compressedSize);
                            return false;
                        }

                        // Patch up the buffer data in the file tables
                        uint64_t bufferSizeInMemory = buf.size; // no compression
                        uint64_t bufferSizeOnDisk = compressedSize;
                        tables.patchBuffer(i, bufferDataOffset, bufferSizeOnDisk, bufferSizeInMemory , buf.crc);
                    }

                    MemFree(compressedData);
                }

                return true;
            }


        }
    }
}
