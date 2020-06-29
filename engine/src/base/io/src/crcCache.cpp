/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "crcCache.h"

#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/system/include/scopeLock.h"
#include "timestamp.h"

namespace base
{
    namespace io
    {
        //--

        CRCCache::CRCCache()
        {
            m_entiresMap.reserve(4096);
        }

        bool CRCCache::fileCRC(StringView<wchar_t> absolutePath, uint64_t& outCRC, uint64_t* outTimestamp, uint64_t* outFileSize) CAN_YIELD
        {
            // get current file timestamp
            io::TimeStamp fileTimeStamp;
            uint64_t fileSize = 0;
            if (!IO::GetInstance().fileTimeStamp(absolutePath, fileTimeStamp, &fileSize))
                return false; // file does not exist

            // we will have to compute it
            auto lock = CreateLock(m_lock);

            // check in cache if it's already computed
            {
                auto entry = m_entiresMap.find(absolutePath);
                if (entry && entry->timestamp == fileTimeStamp.value() && entry->size == fileSize)
                {
                    outCRC = entry->crc;
                    if (outTimestamp)
                        *outTimestamp = entry->timestamp;
                    if (outFileSize)
                        *outFileSize = entry->size;
                    return true;
                }
            }

            // do we have a job entry already ?
            RefWeakPtr<CacheJob> existingCacheJob;
            if (m_activeJobMap.find(absolutePath, existingCacheJob))
            {
                // get the valid entry (note: we may have been released)
                if (auto validCacheJob = existingCacheJob.lock())
                {
                    // release the lock to allow other threads to join waiting for this resource
                    lock.release();

                    // wait for the signal from the thread that created the job
                    TRACE_INFO("CRC cache contention on '{}', waiting for previous job", absolutePath);
                    Fibers::GetInstance().waitForCounterAndRelease(validCacheJob->signal);

                    // we have failed
                    if (!validCacheJob->valid)
                        return false;

                    // return the data
                    // NOTE: access to validCacheJob->m_calculatedCRC is safe since we waited for the fence first
                    // NOTE: the job may have failed as well, we don't care here
                    outCRC = validCacheJob->crc;
                    if (outTimestamp)
                        *outTimestamp = validCacheJob->timestamp;
                    if (outFileSize)
                        *outFileSize = validCacheJob->size;
                    return true;
                }
            }

            // create a cache job entry
            auto validCacheJob = base::CreateSharedPtr<CacheJob>();
            validCacheJob->path = AbsolutePath::Build(absolutePath);
            validCacheJob->crc = 0;
            validCacheJob->timestamp = fileTimeStamp.value();
            validCacheJob->size = fileSize;
            validCacheJob->signal = Fibers::GetInstance().createCounter("CRCCacheBuildFence");
            m_activeJobMap[validCacheJob->path] = validCacheJob;

            // release lock so other threads may join waiting
            lock.release();

            // calculate the CRC of the file
            for (;;)
            {
                // process the entry, this will try to load it and if that fails it will build a new one using the build func
                bool valid = CalculateFileCRC(absolutePath, validCacheJob->crc);

                // if we did compute the CRC make sure still has the same attributes
                if (valid)
                {
                    io::TimeStamp currentFileTimeStamp;
                    uint64_t currentFileSize = 0;
                    if (!IO::GetInstance().fileTimeStamp(absolutePath, currentFileTimeStamp, &currentFileSize))
                    {
                        TRACE_ERROR("File '{}' got deleted while we were calculating CRC", absolutePath);
                    }
                    else if (currentFileTimeStamp.value() != validCacheJob->timestamp || currentFileSize != validCacheJob->size)
                    {
                        TRACE_WARNING("File '{}' got modified while having it's CRC calculated, restarting ({}->{}) ({}->{})",
                            absolutePath, validCacheJob->timestamp, currentFileTimeStamp.value(), validCacheJob->size, currentFileSize);
                        validCacheJob->timestamp = currentFileTimeStamp.value();
                        validCacheJob->size = currentFileSize;
                        continue;
                    }
                }

                // done, put results in
                {
                    validCacheJob->valid = valid;

                    auto lock = CreateLock(m_lock);
                    if (valid)
                    {
                        Entry entry;
                        entry.crc = validCacheJob->crc;
                        entry.timestamp = validCacheJob->timestamp;
                        entry.size = validCacheJob->size;
                        m_entiresMap[validCacheJob->path] = entry;
                    }
                    else
                    {
                        m_entiresMap.remove(validCacheJob->path);
                    }
                }

                // signal dependencies
                Fibers::GetInstance().signalCounter(validCacheJob->signal);
                break;
            }

            // return the cache blob data
            outCRC = validCacheJob->crc;
            if (outTimestamp)
                *outTimestamp = validCacheJob->timestamp;
            if (outFileSize)
                *outFileSize = validCacheJob->size;
            return true;

        }

        bool CRCCache::load(const io::AbsolutePath& absolutePath)
        {
#if 0
            // open file
            auto ret  = CreateUniquePtr<CRCCacheImpl>();
            ret->m_filePath = absolutePath;
            ret->m_fileHandle = IO::GetInstance().openForReadingAndWriting(absolutePath, false);
            if (!ret->m_fileHandle)
            {
                TRACE_WARNING("CRC cache file '{}' does not exit", absolutePath);
                return nullptr;
            }

            // empty
            if (ret->m_fileHandle->size() < sizeof(CRCCacheHeader))
            {
                TRACE_SPAM("CRC cache file '{}' is empty file, a new one will be written", absolutePath);
                return nullptr;
            }

            // load the header
            CRCCacheHeader header;
            if ((sizeof(header) != ret->m_fileHandle->readSync(&header, sizeof(header))) || (header.m_magic != HEADER_MAGIC))
            {
                TRACE_WARNING("CRC cache file '{}' is invalid and/or corrupted", absolutePath);
                return nullptr;
            }

            // load the entries
            // TODO: optimize ?
            auto lastValidOffset  = ret->m_fileHandle->pos();
            for (;;)
            {
                // load the entry
                CRCCacheFileEntry entry;
                if (ret->m_fileHandle->readSync(&entry, sizeof(entry)) != sizeof(entry))
                    break;

                // invalid entry ?
                if (entry.m_magic != ENTRY_MAGIC)
                {
                    TRACE_WARNING("CRC cache '{}' corrupted at offset {}", absolutePath, ret->m_fileHandle->pos());
                    break;
                }

                // add to cache
                ret->m_entiresMap[entry.m_pathHash] = entry;

                // remember last valid file position
                lastValidOffset = ret->m_fileHandle->pos();
            }

            // move file pointer to position after last valid entry
            ret->m_fileHandle->pos(lastValidOffset);
            TRACE_SPAM("Loaded {} entries from CRC cache '{}', last valid offset {}", ret->m_entiresMap.size(), absolutePath, lastValidOffset);

            // use the cache
            return ret;
#endif
            return false;
        }

        bool CRCCache::CalculateFileCRC(StringView<wchar_t> absoluteFilePath, uint64_t& outCRC)
        {
            ScopeTimer timer;

            // open file
            auto filePtr  = IO::GetInstance().openForReading(absoluteFilePath);
            if (!filePtr)
                return false;

            // query file size
            auto fileSize  = filePtr->size();

            // compute CRC in blocks
            CRC64 crc;
            {
                // create the static read buffer
                static const uint64_t BUFFER_SIZE = 64 * 1024;
                static TYPE_TLS void* readBufferData = nullptr;

                // initialize the buffer
                if (nullptr == readBufferData)
                    readBufferData = MemAlloc(POOL_PERSISTENT, BUFFER_SIZE, 16);

                // read and process the file in chunks
                uint64_t offset = 0;
                uint64_t left = fileSize;
                while (left > 0)
                {
                    // TODO: parallelize the read and CRC computation

                    // load content
                    auto readSize = std::min<uint64_t>(BUFFER_SIZE, left);
                    if (readSize != filePtr->readSync(readBufferData, readSize))
                    {
                        TRACE_ERROR("Invalid read from '{}' while calculating CRC, at offset {}", absoluteFilePath, filePtr->pos());
                        return false;
                    }

                    // process loaded content via the CRC calculator
                    crc.append(readBufferData, range_cast<uint32_t>(readSize));
                    left -= readSize;
                    offset += readSize;
                }
            }

            // dump the time if it's longer than few ms
            auto elapsedTime  = timer.milisecondsElapsed();
            if (elapsedTime > 2.0f)
            {
                auto mbPerSecond  = fileSize / (1024.0*1024.0 * timer.timeElapsed());
                TRACE_INFO("Computed CRC of '{}' in {} to be 0x{} ({}/s)", absoluteFilePath, timer, Hex(crc.crc()), MemSize(mbPerSecond));
            }

            // return calculated crc
            outCRC = crc.crc();
            return true;
        }

        //--

    } // io
} // base