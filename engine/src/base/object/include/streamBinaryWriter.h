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
    namespace stream
    {

        /// A file system writer
        class BASE_OBJECT_API IBinaryWriter : public IStream
        {
        public:
            IBinaryWriter(uint32_t flags);
            virtual ~IBinaryWriter();

            //! Serialize byte buffer
            virtual void write(const void* data, uint32_t size) = 0;

            // serialize simple types of known sizes
            // can be specialized to save a little bit performance by having compile time size
            virtual void write1(const void* data) { write(data, 1); }
            virtual void write2(const void* data) { write(data, 2); }
            virtual void write4(const void* data) { write(data, 4); }
            virtual void write8(const void* data) { write(data, 8); }
            virtual void write16(const void* data) { write(data, 16); }

            //! Byte serialization operator
            INLINE void writeValue(uint8_t value) { write1(&value); }
            INLINE void writeValue(uint16_t value) { write2(&value); }
            INLINE void writeValue(uint32_t value) { write4(&value); }
            INLINE void writeValue(uint64_t value) { write8(&value); }
            INLINE void writeValue(char value) { write1(&value); }
            INLINE void writeValue(short value) { write2(&value); }
            INLINE void writeValue(int value) { write4(&value); }
            INLINE void writeValue(int64_t value) { write8(&value); }
            INLINE void writeValue(float value) { write4(&value); }
            INLINE void writeValue(double value) { write8(&value); }
            INLINE void writeValue(wchar_t value) { write2(&value); }
            INLINE void writeValue(bool value) { write1(&value); }
            INLINE void writeValue(StringID value) { writeName(value); }
            INLINE void writeValue(const StringBuf& value) { writeText(value); }

            // write name, uses mapping if possible
            void writeName(StringID name);

            // write generic text
            void writeText(StringView<char> txt);

            // write type reference
            void writeType(Type type);

            // write a property reference
            void writeProperty(const rtti::Property* prop);

            //--

            IDataMapper* m_mapper;
        };

        //---

        // create a binary stream that writes to a native file
        extern BASE_OBJECT_API IBinaryWriter* CreateNativeStreamWriter(const io::FileHandlePtr& filePtr);

        //---

    } // stream
} // base