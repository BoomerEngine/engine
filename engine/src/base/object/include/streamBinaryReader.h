/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary #]
***/

#pragma once

#include "streamBinaryBase.h"

namespace base
{
    namespace res
    {
        class IResourceLoader;
    }

    namespace stream
    {

        /// A file system reader
        class BASE_OBJECT_API IBinaryReader : public IStream
        {
        public:
            IBinaryReader(uint32_t flags);
            virtual ~IBinaryReader();

            //! Serialize byte buffer
            virtual void read(void* data, uint32_t size) = 0;

            // serialize simple types of known sizes
            // can be specialized to save a little bit performance by having compile time size
            virtual void read1(void* data) { read(data, 1); }
            virtual void read2(void* data) { read(data, 2); }
            virtual void read4(void* data) { read(data, 4); }
            virtual void read8(void* data) { read(data, 8); }
            virtual void read16(void* data) { read(data, 16); }

            //! Byte serialization operator
            INLINE void readValue(uint8_t& value) { read1(&value); }
            INLINE void readValue(uint16_t& value) { read2(&value); }
            INLINE void readValue(uint32_t& value) { read4(&value); }
            INLINE void readValue(uint64_t& value) { read8(&value); }
            INLINE void readValue(char& value) { read1(&value); }
            INLINE void readValue(short& value) { read2(&value); }
            INLINE void readValue(int& value) { read4(&value); }
            INLINE void readValue(int64_t& value) { read8(&value); }
            INLINE void readValue(float& value) { read4(&value); }
            INLINE void readValue(double& value) { read8(&value); }
            INLINE void readValue(wchar_t& value) { read2(&value); }
            INLINE void readValue(bool& value) { read1(&value); }
            INLINE void readValue(StringID& value) { value = readName(); }
            INLINE void readValue(StringBuf& value) { value = readText(); }

            //-

            // write name, uses mapping if possible
            StringID readName();

            // write generic text
            StringBuf readText();

            // read type reference
            Type readType();

            //--

            IDataUnmapper* m_unmapper;
            res::IResourceLoader* m_resourceLoader;

        private:
            virtual IBinaryReader* toReader() { return this; }
        };

    } // stream
} // base