/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#include "build.h"
#include "resourceBinaryLoader.h"
#include "resourceBinaryFileTables.h"
#include "resourceBinaryRuntimeTables.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/ioMemoryPool.h"
#include "base/object/include/memoryReader.h"

namespace base
{
    namespace res
    {
        namespace binary
        {

            BinaryLoader::BinaryLoader()
            {}

            BinaryLoader::~BinaryLoader()
            {}

            bool BinaryLoader::loadObjects(stream::IBinaryReader& file, const stream::LoadingContext& context, stream::LoadingResult& result)
            {
                PC_SCOPE_LVL1(LoadObjectsBinary);

                // load tables (names, exports, etc)
                FileTables fileTables;
                if (!fileTables.load(file))
                    return false;

                // resolve
                RuntimeTables runtimeTables;
                runtimeTables.resolve(context, fileTables);

                // build data
                runtimeTables.createExports(context, fileTables);

                // start loading all buffers
                if (context.m_selectiveLoadingClass == nullptr)
                    runtimeTables.loadBuffers(file, fileTables);

                // load data
                if (!runtimeTables.loadExports(0, file, context, fileTables, result))
                    return false;

                // post-load processing
                runtimeTables.postLoad();
                return true;
            }

            bool BinaryLoader::extractLoadingDependencies(stream::IBinaryReader& file, bool includeAsync, Array<stream::LoadingDependency>& outDependencies)
            {
                PC_SCOPE_LVL1(ExtractLoadingDependencies);

                // load tables (names, exports, etc)
                FileTables fileTables;
                if (!fileTables.load(file))
                    return false;

                // extract the dependencies from the file tables
                return fileTables.extractImports(includeAsync, outDependencies);
            }

            //---
            
            bool BinaryLoader::ValidateTables(uint64_t baseOffset, stream::IBinaryReader& file, const FileTables& fileTables)
            {
                uint8_t readBuffer[16 * 1024];

                // validate exports
                for (auto& ex : fileTables.m_exports)
                {
                    // no CRC specified
                    if (!ex.m_crc)
                        continue;

                    // move to object position
                    uint32_t objectSize = ex.m_dataSize;
                    file.seek(baseOffset + ex.m_dataOffset);

                    // read data and compute CRC
                    uint32_t crc = 0;
                    while (objectSize > 0)
                    {
                        // read chunk
                        auto maxRead = std::min<uint32_t>(ARRAY_COUNT(readBuffer), objectSize);
                        file.read(readBuffer, maxRead);
                        objectSize -= maxRead;

                        // compute CRC
                        crc = CRC32(crc).append(readBuffer, maxRead).crc();
                    }

                    // compare it
                    if (ex.m_crc != crc)
                    {
                        auto className  = &fileTables.m_strings[ex.m_className-1];
                        auto objectIndex = &ex - fileTables.m_exports.typedData();

                        TRACE_ERROR("FILE CORRUPTION: Object {} of class '{}' at offset {}, size {} is corrupted (CRC mismatch).",
                            objectIndex, className, ex.m_dataOffset, ex.m_dataSize);
                        return false;
                    }
                }

                // validate buffers
                for (auto& ex : fileTables.m_buffers)
                {
                    // no CRC specified
                    if (!ex.m_crc)
                        continue;

                    // move to object position
                    uint32_t objectSize = ex.m_dataSizeOnDisk;
                    file.seek(baseOffset + ex.m_dataOffset);

                    // read data and compute CRC
                    uint32_t crc = 0;
                    while (objectSize > 0)
                    {
                        // read chunk
                        auto maxRead = std::min<uint32_t>(ARRAY_COUNT(readBuffer), objectSize);
                        file.read(readBuffer, maxRead);
                        objectSize -= maxRead;

                        // compute CRC
                        crc = CRC32(crc).append(readBuffer, maxRead).crc();
                    }

                    // compare it
                    if (ex.m_crc != crc)
                    {
                        auto bufferIndex = &ex - fileTables.m_buffers.typedData();

                        TRACE_ERROR("FILE CORRUPTION !!!: Buffer {} at offset {}, size {} is corrupted (CRC mismatch).",
                            bufferIndex, ex.m_dataOffset, ex.m_dataSizeOnDisk);
                        return false;
                    }
                }

                // No errors found
                return true;
            }

            //--

        } // binary
    } // res
}// base
