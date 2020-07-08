/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#include "build.h"
#include "resourceBinaryFileTables.h"

#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/streamBinaryVersion.h"
#include "base/object/include/serializationLoader.h"

namespace base
{
    namespace res
    {
        namespace binary
        {

            const uint32_t FileTables::FILE_MAGIC = 0x49524346; // 'IRCF';
            const uint32_t FileTables::FILE_VERSION = VER_CURRENT;

            namespace helper
            {
                // helper for loading data table with validation
                template< typename T >
                static bool LoadChunkData(stream::IBinaryReader& file, const FileTables::ChunkType chunkName, uint32_t chunksMask, Array<T>& outData, const FileTables::Chunk* chunks, uint64_t baseOffset)
                {
                    // do not load if not requested
                    if (0 == (chunksMask & (1 << (uint32_t)chunkName)))
                        return true;

                    // load data
                    auto& chunk = chunks[(uint8_t)chunkName];
                    if (chunk.m_count)
                    {
                        outData.resize(chunk.m_count);

                        file.seek(baseOffset + chunk.m_offset);
                        file.read(outData.data(), outData.dataSize());

#ifndef BUILD_RELEASE
                        // validate data by computing CRC of the loaded buffer
                        auto crcValue = CRC32().append(outData.data(), outData.dataSize()).crc();
                        if (chunk.m_crc != crcValue)
                        {
                            TRACE_ERROR("FILE CORRUPTION DETECTED");
                            return false;
                        }
#endif
                    }

                    // valid
                    return true;
                }

                // helper for saving data table
                template< typename T >
                static void SaveChunkData(stream::IBinaryWriter& file, const Array< T >& data, FileTables::Chunk& outChunk, uint64_t baseOffset)
                {
                    if (data.empty())
                    {
                        outChunk.m_count = 0;
                        outChunk.m_crc = 0;
                        outChunk.m_offset = 0;
                    }
                    else
                    {
                        // setup chunk
                        outChunk.m_offset = (const uint32_t)(file.pos() - baseOffset);
                        outChunk.m_count = data.size();
                        outChunk.m_crc = CRC32().append(data.data(), data.dataSize()).crc();

                        file.write(data.data(), data.dataSize());
                    }
                }
            }

            FileTables::FileTables()
                : m_version(0)
            {
            }

            bool FileTables::save(stream::IBinaryWriter& file) const
            {
                uint64_t headerOffset = file.pos();

                // write placeholder header
                Header header;
                memzero(&header, sizeof(header));

                // save the initial header
                file.seek(headerOffset);
                file.write(&header, sizeof(header));
                
                // setup header identification data
                header.m_version = file.version();
                header.m_magic = FILE_MAGIC;

                // save data tables
                header.m_numChunks = ARRAY_COUNT(header.m_chunks); // keep up to date
                helper::SaveChunkData(file, m_strings, header.m_chunks[(uint8_t)ChunkType::Strings], headerOffset);
                helper::SaveChunkData(file, m_names, header.m_chunks[(uint8_t)ChunkType::Names], headerOffset);
                helper::SaveChunkData(file, m_imports, header.m_chunks[(uint8_t)ChunkType::Imports], headerOffset);
                helper::SaveChunkData(file, m_properties, header.m_chunks[(uint8_t)ChunkType::Properties], headerOffset);
                helper::SaveChunkData(file, m_exports, header.m_chunks[(uint8_t)ChunkType::Exports], headerOffset);
                helper::SaveChunkData(file, m_buffers, header.m_chunks[(uint8_t)ChunkType::Buffers], headerOffset);
                helper::SaveChunkData(file, m_source, header.m_chunks[(uint8_t)ChunkType::Sources], headerOffset);

                // compute the size of the object data (preloaded)
                header.m_objectsEnd = (uint32_t)(file.pos() - headerOffset);
                for (const Export& e : m_exports)
                {
                    auto endOffset = e.m_dataOffset + e.m_dataSize;
                    header.m_objectsEnd = std::max<uint32_t>(header.m_objectsEnd, endOffset);
                }

                // compute the end position of the buffered data (preloaded)
                header.m_buffersEnd = header.m_objectsEnd;
                for (const Buffer& b : m_buffers)
                {
                    // only locally stored data
                    if (b.m_dataOffset)
                    {
                        auto endOffset = range_cast<uint32_t>(b.m_dataOffset + b.m_dataSizeOnDisk);
                        header.m_buffersEnd = std::max<uint32_t>(header.m_buffersEnd, endOffset); // TODO: fix
                    }
                }

                // compute header CRC
                header.m_crc = CalcHeaderCRC(header);

                // save it again, preserve file offset at the end of the data
                uint64_t currentOffset = file.pos();
                file.seek(headerOffset);
                file.write(&header, sizeof(header));
                file.seek(currentOffset);
                return true;
            }

            bool FileTables::load(stream::IBinaryReader& file, uint32_t chunkMask /*= ~0U*/)
            {
                uint64_t baseOffset = file.pos();

                // file is to small to load data
                if (file.size() < sizeof(Header))
                {
                    TRACE_ERROR("File is to small to contain any serialized data");
                    return false;
                }

                // size of the header preamble
                uint32_t preamble = sizeof(uint32_t) * 3;

                // load header preamble
                Header header;
                memzero(&header, sizeof(header));
                file.read(&header, preamble);

                // validate header and magic
                if (header.m_magic != FILE_MAGIC)
                {
                    TRACE_ERROR("File magic is invalid (%08X != %08X), file is not a binary resource.", header.m_magic, FILE_MAGIC);
                    return false;
                }

                // validate header version - should not be older than the CRC version
                if (header.m_version > VER_CURRENT)
                {
                    TRACE_ERROR("File version is invalid ({} > {}), file is from newer version of the engine.", header.m_version, VER_CURRENT);
                    return false;
                }

                // read rest of the header
                file.read((uint8_t*)&header + preamble, sizeof(header) - preamble);

#ifndef BUILD_RELEASE
                // calculate current header CRC
                CRCValue crc = CalcHeaderCRC(header);
                if (header.m_crc && crc != header.m_crc)
                {
                    TRACE_ERROR("FILE CORRUPTION DETECTED");
                    return false;
                }
#endif

                // create the tables using the chunk information and load chunk data
                if (!helper::LoadChunkData(file, ChunkType::Strings, chunkMask, m_strings, header.m_chunks, baseOffset)) return false;
                if (!helper::LoadChunkData(file, ChunkType::Names, chunkMask, m_names, header.m_chunks, baseOffset)) return false;
                if (!helper::LoadChunkData(file, ChunkType::Imports, chunkMask, m_imports, header.m_chunks, baseOffset)) return false;
                if (!helper::LoadChunkData(file, ChunkType::Properties, chunkMask, m_properties, header.m_chunks, baseOffset)) return false;
                if (!helper::LoadChunkData(file, ChunkType::Exports, chunkMask, m_exports, header.m_chunks, baseOffset)) return false;
                if (!helper::LoadChunkData(file, ChunkType::Buffers, chunkMask, m_buffers, header.m_chunks, baseOffset)) return false;
                if (!helper::LoadChunkData(file, ChunkType::Sources, chunkMask, m_source, header.m_chunks, baseOffset)) return false;

                // check for data overrides
#ifndef BUILD_RELEASE
                if (chunkMask == ~0U)
                {
                    uint64_t minOffset = (file.pos() - baseOffset);
                    uint64_t curOffset = minOffset;

                    // check exports
                    for (uint32_t i = 0; i < m_exports.size(); ++i)
                    {
                        if (m_exports[i].m_dataOffset != curOffset)
                        {
                            TRACE_ERROR("FILE CORRUPTION DETECTED: Export {} at offset {} aliased with previous data (overlap: {})",
                                i, m_exports[i].m_dataOffset, curOffset - m_exports[i].m_dataOffset);

                            return false;
                        }

                        curOffset += m_exports[i].m_dataSize;
                    }

                    // validate header inter-offset
                    if (curOffset != header.m_objectsEnd)
                    {
                        TRACE_ERROR("FILE CORRUPTION DETECTED: Size of the object data: {}, expected {}",
                            curOffset, header.m_objectsEnd);

                        return false;
                    }

                    // check buffers
                    for (uint32_t i = 0; i < m_buffers.size(); ++i)
                    {
                        if (!m_buffers[i].m_dataOffset)
                            continue;

                        if (m_buffers[i].m_dataOffset != curOffset)
                        {
                            TRACE_ERROR("FILE CORRUPTION DETECTED: Buffer {} at offset {} aliased with previous data (overlap: {})",
                                i, m_buffers[i].m_dataOffset, curOffset - m_buffers[i].m_dataOffset);

                            return false;
                        }

                        curOffset += m_buffers[i].m_dataSizeOnDisk;
                    }
                }
#endif

                // done
                m_version = header.m_version;
                file.m_version = header.m_version;
                return true;
            }

            bool FileTables::extractImports(bool includeAsync, Array<stream::LoadingDependency>& outDependencies) const
            {
                for (const auto& data : m_imports)
                {
                    const auto alwaysLoaded = 0 != (data.m_flags & (uint16_t)FileTables::ImportFlags::Load);
                    if (includeAsync || alwaysLoaded)
                    {
                        // find the resource class, it should exist if the file is to be loaded
                        auto className = &m_strings[m_names[data.m_className - 1].m_string];
                        if (auto resourceClass = RTTI::GetInstance().findClass(StringID::Find(className)))
                        {
                            auto& depInfo = outDependencies.emplaceBack();
                            depInfo.async = !alwaysLoaded;
                            depInfo.resourceDepotPath = StringBuf(&m_strings[data.m_path]);
                            depInfo.resourceClass = resourceClass;
                        }
                    }
                }

                return true;
            }

            FileTables::CRCValue FileTables::CalcHeaderCRC(const Header& header)
            {
                // tricky bit - the CRC field cannot be included in the CRC calculation - calculate the CRC without it
                Header tempHeader;
                tempHeader = header;
                tempHeader.m_crc = 0xDEADBEEF; // special hacky stuff

                return CRC32().append(&tempHeader, sizeof(tempHeader)).crc();
            }

        } // binary
    } // res
} // base