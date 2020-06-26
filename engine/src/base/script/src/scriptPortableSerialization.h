/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: runtime #]
***/

#pragma once

#include "base/script/include/scriptOpcodes.h"
#include "base/containers/include/hashSet.h"
#include "base/containers/include/queue.h"
#include "base/containers/include/pagedBuffer.h"
#include "base/memory/include/linearAllocator.h"

namespace base
{
    namespace script
    {

        ///---

        // stub mapper - explores all the stubs
        class StubMapper : public IStubWriter
        {
        public:
            StubMapper();

            virtual void writeBool(bool b) override final {};
            virtual void writeInt8(char val) override final {};
            virtual void writeInt16(short val) override final {};
            virtual void writeInt32(int val) override final {};
            virtual void writeInt64(int64_t val) override final {};
            virtual void writeUint8(uint8_t val) override final {};
            virtual void writeUint16(uint16_t val) override final {};
            virtual void writeUint32(uint32_t val) override final {};
            virtual void writeUint64(uint64_t val) override final {};
            virtual void writeFloat(float val) override final {};
            virtual void writeDouble(double val) override final {};

            virtual void writeString(StringView<char> str) override final;
            virtual void writeName(StringID name) override final;
            virtual void writeRef(const Stub* otherStub) override final;

            //--

            HashMap<const Stub*, uint32_t> m_stubMap;
            HashMap<uint64_t, uint16_t> m_stringMap;
            HashMap<StringID, uint16_t> m_nameMap;

            Array<const Stub*> m_stubs;
            Array<StringBuf> m_strings;
            Array<StringID> m_names;

            uint32_t m_firstUnprocessedObjects;
            Array<const Stub*> m_unprocessedObjects;
            HashSet<const Stub*> m_processedObjects;

            void processObject(const Stub* stub);
            bool processRemainingObjects();
        };

        ///---

        // data writer
        class StubDataWriter : public IStubWriter
        {
        public:
            StubDataWriter(const StubMapper& mapper, PagedBuffer<uint8_t>& outMemory);

            virtual void writeBool(bool b) override final;
            virtual void writeInt8(char val) override final;
            virtual void writeInt16(short val) override final;
            virtual void writeInt32(int val) override final;
            virtual void writeInt64(int64_t val) override final;
            virtual void writeUint8(uint8_t val) override final;
            virtual void writeUint16(uint16_t val) override final;
            virtual void writeUint32(uint32_t val) override final;
            virtual void writeUint64(uint64_t val) override final;
            virtual void writeFloat(float val) override final;
            virtual void writeDouble(double val) override final;

            virtual void writeString(StringView<char> str) override final;
            virtual void writeName(StringID name) override final;
            virtual void writeRef(const Stub* otherStub) override final;

            void writeRawString(StringView<char> txt);

            void writeContainers();

        private:
            const StubMapper& m_mapper;
            PagedBuffer<uint8_t>& m_outMemory;
        };

        ///---

        // data reader
        class StubDataReader : public IStubReader
        {
        public:
            StubDataReader(const void* data, uint32_t dataSize, mem::LinearAllocator& unpackedMem);

            virtual bool readBool() override final;
            virtual char readInt8() override final;
            virtual short readInt16() override final;
            virtual int readInt32() override final;
            virtual int64_t readInt64() override final;
            virtual uint8_t readUint8() override final;
            virtual uint16_t readUint16() override final;
            virtual uint32_t readUint32() override final;
            virtual uint64_t readUint64() override final;
            virtual float readFloat() override final;
            virtual double readDouble() override final;
            virtual StringBuf readString() override final;
            virtual StringID readName() override final;
            virtual const Stub* readRef() override final;

            StringView<char> readRawString();

            bool readContainers();

            void allStubs(Array<const Stub*>& outAllStubs) const;

            const StubModule* exportedModule() const;

        private:
            mem::LinearAllocator& m_mem;
            bool m_hasErrors;

            const uint8_t* m_readPtr;
            const uint8_t* m_endPtr;

            base::Array<StringID> m_names;
            base::Array<StringBuf> m_strings;
            base::Array<Stub*> m_stubs;

            bool validateRead(uint32_t size);

            template< typename T >
            INLINE T read()
            {
                auto ret  = *(const T*)m_readPtr;
                m_readPtr += sizeof(T);
                return ret;
            }
        };


    } // script
} // base

