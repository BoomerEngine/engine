/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#pragma once

#include "streamOpcodes.h"

#define SUPPORT_PROTECTED_STREAM

namespace base
{
    namespace stream
    {
        ///---

        struct BASE_OBJECT_API OpcodeResolvedResourceReference
        {
            StringBuf path;
            ClassType type;
            ObjectPtr loaded; // only if loaded

            OpcodeResolvedResourceReference();
        };

        struct BASE_OBJECT_API OpcodeResolvedReferences : public NoCopy
        {
            Array<StringID> stringIds;

            Array<Type> types;
            Array<StringID> typeNames;

            Array<const rtti::Property*> properties;
            Array<StringID> propertyNames;

            Array<ObjectPtr> objects;
            Array<OpcodeResolvedResourceReference> resources;

            OpcodeResolvedReferences();
        };

        ///---

        class BASE_OBJECT_API OpcodeReader : public NoCopy
        {
        public:
            OpcodeReader(const OpcodeResolvedReferences& refs, const void* data, uint64_t size, bool safeLayout);
            ~OpcodeReader();

            /// get data pointer (reads the OpValue)
            INLINE const void* pointer(uint64_t size);

            /// copy data to another place
            INLINE void readData(void* data, uint64_t size);

            /// load typed data
            template< typename T >
            INLINE void readTypedData(T& data);

            ///--

            /// enter compound block, returns number of properties to read
            INLINE void enterCompound(uint32_t& outNumProperties);

            /// leave compound block
            INLINE void leaveCompound();

            ///--

            /// enter array block, returns number of elements in the array
            INLINE void enterArray(uint32_t& outNumElements);

            /// exit array block
            INLINE void leaveArray();

            //--

            /// enter the skip block
            INLINE void enterSkipBlock();

            /// leave the skip block
            INLINE void leaveSkipBlock();

            /// jump over the skip block
            void discardSkipBlock();

            ///--

            /// read StringID
            INLINE StringID readStringID();

            /// read type reference
            INLINE Type readType(StringID& outTypeName);

            /// read property reference
            INLINE const rtti::Property* readProperty(base::StringID& outPropertyName);

            /// read pointer to other object
            INLINE IObject* readPointer();

            /// read resource reference
            INLINE const OpcodeResolvedResourceReference* readResource();

            /// read buffer, NOTE: optionally the existing IO memory may be used
            Buffer readBuffer(bool makeCopy=true);

            ///---

        private:
            const uint8_t* m_cur = nullptr;
            const uint8_t* m_end = nullptr;
            const uint8_t* m_base = nullptr;

            bool m_protectedStream = false;

            const OpcodeResolvedReferences& m_refs;

            INLINE uint64_t readCompressedNumber();

            INLINE void checkOp(StreamOpcode op);
            INLINE void checkDataOp(uint64_t size);
        };

        ///---

        INLINE const void* OpcodeReader::pointer(uint64_t size)
        {
#ifdef SUPPORT_PROTECTED_STREAM
            checkDataOp(size);
#endif
            ASSERT_EX(m_cur + size <= m_end, "Read past buffer end");
            auto* ptr = m_cur;
            m_cur += size;
            return ptr;
        }

        INLINE void OpcodeReader::readData(void* data, uint64_t size)
        {
#ifdef SUPPORT_PROTECTED_STREAM
            checkDataOp(size);
#endif
            ASSERT_EX(m_cur + size <= m_end, "Read past buffer end");
            memcpy(data, m_cur, size);
            m_cur += size;
        }

        template< typename T >
        INLINE void OpcodeReader::readTypedData(T& data)
        {
#ifdef SUPPORT_PROTECTED_STREAM
            checkDataOp(sizeof(T));
#endif
            ASSERT_EX(m_cur + sizeof(T) <= m_end, "Read past buffer end");
            if (alignof(T) < 16)
                data = *(const T*)m_cur;
            else
                memcpy(&data, m_cur, sizeof(T));
            m_cur += sizeof(T);
        }

        INLINE void OpcodeReader::checkOp(StreamOpcode op)
        {
#ifdef SUPPORT_PROTECTED_STREAM
            if (m_protectedStream)
            {
                ASSERT_EX(m_cur < m_end, "Read past buffer end");
                ASSERT_EX((StreamOpcode)*m_cur == op, "Invalid opcode");
                m_cur += 1;
            }
#endif
        }

        INLINE void OpcodeReader::checkDataOp(uint64_t size)
        {
#ifdef SUPPORT_PROTECTED_STREAM
            if (m_protectedStream)
            {
                checkOp(StreamOpcode::DataRaw);
                const auto savedSize = readCompressedNumber();
                ASSERT_EX(savedSize == size, TempString("Invalid size of saved data {} compared to what is requested ({})", savedSize, size));
            }
#endif
        }

        INLINE void OpcodeReader::enterCompound(uint32_t& outNumProperties)
        {
            checkOp(StreamOpcode::Compound);
            outNumProperties = readCompressedNumber();
        }

        INLINE void OpcodeReader::leaveCompound()
        {
            checkOp(StreamOpcode::CompoundEnd);
        }

        INLINE void OpcodeReader::enterArray(uint32_t& outNumElements)
        {
            checkOp(StreamOpcode::Array);
            outNumElements = readCompressedNumber();
        }

        INLINE void OpcodeReader::leaveArray()
        {
            checkOp(StreamOpcode::ArrayEnd);
        }

        INLINE void OpcodeReader::enterSkipBlock()
        {
            checkOp(StreamOpcode::SkipHeader);
            //readCompressedNumber(); // skip size
        }

        INLINE void OpcodeReader::leaveSkipBlock()
        {
            checkOp(StreamOpcode::SkipLabel);
        }
        
        INLINE StringID OpcodeReader::readStringID()
        {
            checkOp(StreamOpcode::DataName);
            const auto index = readCompressedNumber();
            return m_refs.stringIds[index];
        }

        INLINE Type OpcodeReader::readType(StringID& outTypeName)
        {
            checkOp(StreamOpcode::DataTypeRef);
            const auto index = readCompressedNumber();
            outTypeName = m_refs.typeNames[index];
            return m_refs.types[index];
        }

        INLINE const rtti::Property* OpcodeReader::readProperty(base::StringID& outPropertyName)
        {
            checkOp(StreamOpcode::Property);
            const auto index = readCompressedNumber();
            outPropertyName = m_refs.propertyNames[index];
            return m_refs.properties[index];
        }

        INLINE IObject* OpcodeReader::readPointer()
        {
            checkOp(StreamOpcode::DataObjectPointer);
            const auto index = readCompressedNumber();
            if (index == 0 || index > (int)m_refs.objects.size())
                return nullptr;

            return m_refs.objects[index - 1];
        }

        INLINE const OpcodeResolvedResourceReference* OpcodeReader::readResource()
        {
            checkOp(StreamOpcode::DataResourceRef);
            const auto index = readCompressedNumber();
            if (index == 0 || index > (int)m_refs.resources.size())
                return nullptr;

            return &m_refs.resources[index - 1];
        }

        INLINE uint64_t OpcodeReader::readCompressedNumber()
        {
            ASSERT_EX(m_cur < m_end, "Reading past the end of the stream");

            auto singleByte = *m_cur++;
            uint64_t ret = singleByte & 0x7F;
            uint32_t offset = 7;

            while (singleByte & 0x80)
            {
                ASSERT_EX(m_cur < m_end, "Reading past the end of the stream");
                singleByte = *m_cur++;
                ret |= (singleByte & 0x7F) << offset;
                offset += 7;
            }

            return ret;
        }

        //---

    } // stream
} // base
