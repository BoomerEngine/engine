/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary #]
***/

#include "build.h"
#include "streamBinaryReader.h"
#include "serializationUnampper.h"
#include "rttiType.h"
#include "rttiProperty.h"

namespace base
{
    namespace stream
    {

        //---

        IBinaryReader::IBinaryReader(uint32_t flags)
            : IStream(flags | (uint32_t)BinaryStreamFlags::Reader)
            , m_unmapper(nullptr)
            , m_resourceLoader(nullptr)
        {}

        IBinaryReader::~IBinaryReader()
        {}

        StringBuf IBinaryReader::readText()
        {
            uint32_t len = 0;
            readValue(len);

            auto ret = StringBuf(len);
            read((void*)ret.c_str(), len);
            return ret;
        }

        StringID IBinaryReader::readName()
        {
            StringID ret;

            if (m_unmapper != nullptr)
            {
                stream::MappedNameIndex index = 0;
                readValue(index);

                m_unmapper->unmapName(index, ret);
            }
            else
            {
                static const uint32_t INTERNAL_BUFFER_SIZE = 64;

                uint32_t len = 0;
                readValue(len);

                if (len < INTERNAL_BUFFER_SIZE)
                {
                    char buffer[INTERNAL_BUFFER_SIZE + 1];
                    read(buffer, len);
                    buffer[len] = 0;

                    ret = StringID(buffer);
                }
                else
                {
                    StringBuf txt(len);
                    read((void*)txt.c_str(), len);
                    ret = StringID(txt.view());
                }
            }

            return ret;
        }

        Type IBinaryReader::readType()
        {
            Type ret;

            if (m_unmapper != nullptr)
            {
                stream::MappedTypeIndex index = 0;
                readValue(index);

                m_unmapper->unmapType(index, ret);
            }
            else
            {
                auto typeName = readName();
                ret = RTTI::GetInstance().findType(typeName);
            }

            return ret;
        }

        //---

    } // stream

} // base
