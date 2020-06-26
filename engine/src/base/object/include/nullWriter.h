/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary\null #]
***/

#pragma once

#include "streamBinaryWriter.h"

#include "base/io/include/ioFileHandle.h"

namespace base
{
    namespace stream
    {

        /// NULL writer that does nothing (it keeps track of the size and offset though)
        class BASE_OBJECT_API NullWriter : public IBinaryWriter
        {
        public:
            NullWriter();
            virtual ~NullWriter();

            virtual uint64_t pos() const override final;
            virtual uint64_t size() const override final;
            virtual void seek(uint64_t pos) override final;
            virtual void write(const void *data, uint32_t size) override final;

        private:
            uint64_t m_pos;
            uint64_t m_size;
        };

    } // stream
} // base


