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

namespace rendering
{

    //--

    void DescriptorEntry::view(const IDeviceObjectView* ptr, uint32_t offset/*= 0*/)
    {
        DEBUG_CHECK_RETURN(ptr != nullptr);
        DEBUG_CHECK_RETURN(id.empty());
        DEBUG_CHECK_RETURN(type == DeviceObjectViewType::Invalid);
        
#ifdef VALIDATE_DESCRIPTOR_BOUND_RESOURCES
		viewPtr = ptr;
#endif

        this->id = ptr->viewId();
        this->type = ptr->viewType();
        this->offset = offset;
        this->size = 0;
        this->offsetPtr = nullptr;
    }

    void DescriptorEntry::constants(const void* data, uint32_t size)
    {
        DEBUG_CHECK_RETURN(id.empty());
        DEBUG_CHECK_RETURN(type == DeviceObjectViewType::Invalid);
        DEBUG_CHECK_RETURN(data != nullptr);
        DEBUG_CHECK_RETURN(size != 0);

        this->id = ObjectID();
		this->type = DeviceObjectViewType::ConstantBuffer;
		this->offset = 0;
		this->size = size;
		this->offsetPtr = (const uint32_t*)data;
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

} // rendering