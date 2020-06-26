/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\mapping #]
***/

#include "build.h"
#include "serializationUnampper.h"

namespace base
{
    namespace stream
    {

        //--

        PreloadedBufferLatentLoader::PreloadedBufferLatentLoader(const Buffer& data, uint64_t crc)
            : m_data(data)
            , m_crc(crc)
        {}

        uint32_t PreloadedBufferLatentLoader::size() const
        {
            return m_data.size();
        }

        uint64_t PreloadedBufferLatentLoader::crc() const
        {
            return m_crc;
        }

        bool PreloadedBufferLatentLoader::resident() const
        {
            return true;
        }

        Buffer PreloadedBufferLatentLoader::loadAsync() const
        {
            return m_data;
        }

        //--

        IDataUnmapper::IDataUnmapper()
        {}

        IDataUnmapper::~IDataUnmapper()
        {}

        //--

    } // stream
} // base
