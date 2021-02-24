/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\descriptor #]
***/

#include "build.h"
#include "renderingDescriptor.h"
#include "renderingObject.h"

BEGIN_BOOMER_NAMESPACE(rendering)

//--

void DescriptorEntry::view(const IDeviceObjectView* ptr, uint32_t offset/*= 0*/)
{
    DEBUG_CHECK_RETURN_EX(ptr != nullptr, "Trying to bind NULL view to descriptor");
    DEBUG_CHECK_RETURN_EX(id.empty(), "Entry is already bound");
    DEBUG_CHECK_RETURN_EX(type == DeviceObjectViewType::Invalid, "Entry is already bound");
        
#ifdef VALIDATE_DESCRIPTOR_BOUND_RESOURCES
	viewPtr = ptr;
#endif

    this->id = ptr->viewId();
    this->type = ptr->viewType();
    this->offset = offset;
}

void DescriptorEntry::sampler(const SamplerObject* ptr)
{
	DEBUG_CHECK_RETURN_EX(id.empty(), "Entry is already bound");
	DEBUG_CHECK_RETURN_EX(type == DeviceObjectViewType::Invalid, "Entry is already bound");
	DEBUG_CHECK_RETURN_EX(ptr != nullptr, "Trying to bind NULL sampler to descriptor");

	this->id = ptr->id();
#ifdef VALIDATE_DESCRIPTOR_BOUND_RESOURCES
    this->objectPtr = ptr;
#endif
	this->type = DeviceObjectViewType::Sampler;
}

void DescriptorEntry::constants(const void* data, uint32_t size)
{
	DEBUG_CHECK_RETURN_EX(id.empty(), "Entry is already bound");
	DEBUG_CHECK_RETURN_EX(type == DeviceObjectViewType::Invalid, "Entry is already bound");
    DEBUG_CHECK_RETURN_EX(data != nullptr, "No data in inlined constants buffer");
    DEBUG_CHECK_RETURN_EX(size != 0, "No data in inlined constants buffer");

    this->id = ObjectID();
	this->type = DeviceObjectViewType::ConstantBuffer;
	this->size = size;
	this->inlinedConstants.sourceDataPtr = data;
}

void DescriptorEntry::print(base::IFormatStream& f) const
{
    switch (type)
    {
        case DeviceObjectViewType::ConstantBuffer:
            f << "CBV";
            if (id.empty())
                f.append(" ({}B)", size);
            else
                f.appendf(" {}", id);
            break;

        case DeviceObjectViewType::Buffer:
            f.appendf("BSRV {}", id);
            break;

        case DeviceObjectViewType::BufferWritable:
            f.appendf("BUAV {}", id);
            break;

        case DeviceObjectViewType::BufferStructured:
            f.appendf("SBSRV {}", id);
            break;

        case DeviceObjectViewType::BufferStructuredWritable:
            f.appendf("SBUAV {}", id);
            break;

		case DeviceObjectViewType::SampledImage:
			f.appendf("SSRV {}", id);
			break;

        case DeviceObjectViewType::Image:
            f.appendf("ISRV {}", id);
            break;

        case DeviceObjectViewType::ImageWritable:
            f.appendf("IUAV {}", id);
            break;

        case DeviceObjectViewType::RenderTarget:
            f.appendf("RTV {}", id);
            break;

        case DeviceObjectViewType::Sampler:
            f.appendf("SMPL {}", id);
            break;
    }

    if (offset)
        f.appendf("+{}", offset);
}

//----

END_BOOMER_NAMESPACE(rendering)