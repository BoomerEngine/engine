/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary\null #]
***/

#include "build.h"
#include "nullWriter.h"

namespace base
{
    namespace stream
    {

        NullWriter::NullWriter()
            : IBinaryWriter((uint32_t)BinaryStreamFlags::NullFile)
            , m_pos(0)
            , m_size(0)
        {
        }

        NullWriter::~NullWriter()
        {
        }

        uint64_t NullWriter::pos() const
        {
            return m_pos;
        }

        uint64_t NullWriter::size() const
        {
            return m_size;
        }

        void NullWriter::seek(uint64_t pos)
        {
            m_pos = pos;
        }

        void NullWriter::write(const void *data, uint32_t size)
        {
            m_pos += size;
            m_size = std::max(m_size, m_pos);
        }

    } // stream
} // base
