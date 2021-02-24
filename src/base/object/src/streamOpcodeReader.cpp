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

BEGIN_BOOMER_NAMESPACE(base::stream)

//--

OpcodeResolvedResourceReference::OpcodeResolvedResourceReference()
{}

//--

OpcodeResolvedReferences::OpcodeResolvedReferences()
{}

//--

OpcodeReader::OpcodeReader(const OpcodeResolvedReferences& refs, const void* data, uint64_t size, bool safeLayout, uint32_t version)
    : m_refs(refs)
    , m_protectedStream(safeLayout)
    , m_version(version)
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

void OpcodeReader::discardSkipBlock()
{
    ASSERT_EX(m_protectedStream, "Trying to skip blocks in unprotected stream. This should have been found by validator.");
            
    checkOp(StreamOpcode::SkipHeader);

    uint32_t skipLevel = 1;
    while (skipLevel && m_cur < m_end)
    {
        const auto op = (StreamOpcode) *m_cur++;
        switch (op)
        {
            case StreamOpcode::SkipHeader:
            {
                skipLevel += 1;
                break;
            }

            case StreamOpcode::SkipLabel:
            {
                skipLevel -= 1;
                break;
            }

            case StreamOpcode::Array:
            case StreamOpcode::Compound:
            case StreamOpcode::Property:
            case StreamOpcode::DataTypeRef:
            case StreamOpcode::DataName:
            case StreamOpcode::DataObjectPointer:
            case StreamOpcode::DataResourceRef:
            {
                readCompressedNumber();
                break;
            }

            case StreamOpcode::DataRaw:
            case StreamOpcode::DataInlineBuffer:
            {
                auto size = readCompressedNumber();
                ASSERT_EX(m_cur + size <= m_end, "Data size larger read region. This should be caught by validator.");
                m_cur += size;
                break;
            }

            case StreamOpcode::DataAsyncBuffer:
            {
                break;
            }

            case StreamOpcode::CompoundEnd:
            case StreamOpcode::ArrayEnd:
                break;

            case StreamOpcode::Nop:
            default:
                ASSERT(!"Unknown opcode found when skipping. This should have been found by validator.");
        }

    }

    ASSERT_EX(skipLevel == 0, "End of stream reached before skip target was found. This should have been found by validator.");
}

//--

END_BOOMER_NAMESPACE(base::stream)
