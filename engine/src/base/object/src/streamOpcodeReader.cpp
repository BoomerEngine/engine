/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiProperty.h"
#include "streamOpcodeReader.h"

namespace base
{
    namespace stream
    {
        //--

        OpcodeResolvedResourceReference::OpcodeResolvedResourceReference()
        {}

        //--

        OpcodeResolvedReferences::OpcodeResolvedReferences()
        {}

        //--

        OpcodeReader::OpcodeReader(const OpcodeResolvedReferences& refs, const void* data, uint64_t size, bool safeLayout)
            : m_refs(refs)
            , m_protectedStream(safeLayout)
        {
            m_base = (const uint8_t*)data;
            m_cur = m_base;
            m_end = m_base + size;
        }

        OpcodeReader::~OpcodeReader()
        {}

        Buffer OpcodeReader::readBuffer(bool makeCopy)
        {
            checkOp(StreamOpcode::DataInlineBuffer);
            const auto size = readCompressedNumber();

            ASSERT_EX(m_cur + size <= m_end, "Read past buffer end");
            auto* ptr = m_cur;
            m_cur += size;

            if (makeCopy)
                return Buffer::Create(POOL_SERIALIZATION, size, 16, ptr);
            else
                return Buffer::CreateExternal(POOL_SERIALIZATION, size, (void*)ptr);
        }

        //--

    } // stream
} // base
