/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/containers/include/hashMap.h"

namespace base
{
    namespace io
    {
        // a cache of file CRCs based on timestamp
        class BASE_IO_API CRCCache : public base::NoCopy
        {
        public:
            CRCCache();

            /// clear cache of all entries
            void clear();

            /// load cache content from a file, loaded entries will be merged with existing ones
            bool load(const AbsolutePath& absolutePath);

            /// save cache content to a file
            bool save(const AbsolutePath& absolutePath);

            //--

            /// get the cached CRC of the file, returns false if file does not exist
            /// NOTE: if another fiber requests the CRC for the same file it will join the waiting queue of the first request
            /// NOTE: files larger than some cutoff my return CRC 0 - then we assume they are always different if the timestamp/size changes
            bool fileCRC(StringView<wchar_t> absolutePath, uint64_t& outCRC, uint64_t* outTimestamp, uint64_t* outFileSize) CAN_YIELD;

        private:
            static const uint32_t HEADER_MAGIC = 0x43524343;//'CRCC';

            AbsolutePath m_filePath;
            FileHandlePtr m_fileHandle;

            //--

            struct Header
            {
                uint32_t magic = 0;
                uint32_t numEntries = 0;
                uint64_t crc = 0;
            };

            struct Entry
            {
                uint64_t timestamp = 0;
                uint64_t size = 0;
                uint64_t crc = 0;
            };

            HashMap<io::AbsolutePath, Entry> m_entiresMap;
            //SpinLock m_entiresMapLock;

            //--

            // a job, either loading or baking
            struct CacheJob : public IReferencable
            {
                io::AbsolutePath path;
                uint64_t crc = 0;
                uint64_t timestamp = 0;
                uint64_t size = 0;
                bool valid = false;
                fibers::WaitCounter signal;
            };

            // synchronization for blobs being loaded/saved
            HashMap<io::AbsolutePath, RefWeakPtr<CacheJob>> m_activeJobMap;
            SpinLock m_lock;

            //--

            static CAN_YIELD bool CalculateFileCRC(StringView<wchar_t> absoluteFilePath, uint64_t& outCRC);
        };

    } // io
} // base