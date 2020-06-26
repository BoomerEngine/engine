/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary #]
***/

#include "build.h"
#include "streamBinaryWriter.h"
#include "serializationMapper.h"
#include "rttiType.h"
#include "rttiProperty.h"

namespace base
{
    namespace stream
    {

        //--

        IBinaryWriter::IBinaryWriter(uint32_t flags)
            : IStream(flags | (uint32_t)BinaryStreamFlags::Writer)
            , m_mapper(nullptr)
        {}

        IBinaryWriter::~IBinaryWriter()
        {}

        void IBinaryWriter::writeName(StringID name)
        {
            if (m_mapper)
            {
                stream::MappedNameIndex index = 0;
                m_mapper->mapName(name, index);
                writeValue(index);
            }
            else
            {
                writeText(name.view());
            }
        }

        void IBinaryWriter::writeText(StringView<char> txt)
        {
            uint32_t len = txt.length();
            writeValue(len);
            write(txt.data(), len);
        }

        void IBinaryWriter::writeType(Type type)
        {
            if (m_mapper)
            {
                stream::MappedTypeIndex index = 0;
                m_mapper->mapType(type, index);
                writeValue(index);
            }
            else
            {
                writeName(type.name());
            }
        }

        void IBinaryWriter::writeProperty(const rtti::Property* prop)
        {
            if (m_mapper)
            {
                stream::MappedPropertyIndex index = 0;
                m_mapper->mapProperty(prop, index);
                writeValue(index);
            }
            else if (prop == nullptr)
            {
                writeName(StringID());
            }
            else
            {
                writeName(prop->name());
                writeName(prop->type()->name());
            }
        }

        //--

    } // stream
} // base
