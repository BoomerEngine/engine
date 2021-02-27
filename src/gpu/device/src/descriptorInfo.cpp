/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\descriptor #]
***/

#include "build.h"
#include "descriptorInfo.h"

#include "core/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

DescriptorInfo::DescriptorInfo()
    : m_hash(0)
{}

DescriptorInfo::DescriptorInfo(const DescriptorInfo& other, DescriptorID id)
    : m_hash(other.m_hash)
    , m_entries(other.m_entries)
    , m_id(id)
{}

void DescriptorInfo::print(IFormatStream& str) const
{
    bool first = true;

    for (auto type : m_entries)
    {
        if (first)
            first = false;
        else
            str << "-";

        switch (type)
        {
        case DeviceObjectViewType::ConstantBuffer: str.append("CBV"); break;
        case DeviceObjectViewType::Buffer: str.append("BSRV"); break;
        case DeviceObjectViewType::BufferWritable: str.append("BUAV"); break;
        case DeviceObjectViewType::BufferStructured: str.append("SBSRV"); break;
        case DeviceObjectViewType::BufferStructuredWritable: str.append("SBUAV"); break;
        case DeviceObjectViewType::Image: str.append("ISRV"); break;
		case DeviceObjectViewType::SampledImage: str.append("SSRV"); break;
        case DeviceObjectViewType::ImageWritable: str.append("IUAV"); break;
        case DeviceObjectViewType::RenderTarget: str.append("RTV"); break;
        case DeviceObjectViewType::Sampler: str.append("SMPL"); break;

            default:
                FATAL_ERROR(TempString("Invalid type of the object view {} in the parameter layout", (uint8_t) type));
                break;
        }
    }
}

bool DescriptorInfo::operator==(const DescriptorInfo& other) const
{
    if (m_hash != other.m_hash)
        return false;

    //ASSERT(m_entries == other.m_entries);
    return true;
}

//--

END_BOOMER_NAMESPACE_EX(gpu)
