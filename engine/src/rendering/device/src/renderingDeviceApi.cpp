/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "renderingDeviceApi.h"
#include "renderingResources.h"

namespace rendering
{
    
    //--

    RTTI_BEGIN_TYPE_CLASS(DeviceNameMetadata);
    RTTI_END_TYPE();

    DeviceNameMetadata::DeviceNameMetadata()
        : m_name(nullptr)
    {}

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDevice);
    RTTI_END_TYPE();

    IDevice::IDevice()
    {}

    IDevice::~IDevice()
    {}

    //--

	bool IDevice::validateBufferCreationSetup(const BufferCreationInfo& info, BufferObject::Setup& outSetup) const
	{
		uint8_t numExclusiveModes = 0;
		if (info.allowCostantReads) numExclusiveModes += 1;
		if (info.allowVertex) numExclusiveModes += 1;
		if (info.allowIndirect) numExclusiveModes += 1;
		if (info.allowIndex) numExclusiveModes += 1;
		//if (info.allowShaderReads) numExclusiveModes += 1;
		DEBUG_CHECK_RETURN_EX_V(numExclusiveModes <= 1, "Only one of the exclusive primary usage modes must be set: vertex, index, uniform, shader or indirect", nullptr);

		// constant buffer can't be read as a format buffer
		DEBUG_CHECK_RETURN_EX_V(!info.allowCostantReads || !info.allowShaderReads, "Buffer can't be used as uniform and shader buffer at the same time (it might by UAV buffer though)", nullptr);

		// align to min size
		DEBUG_CHECK_RETURN_EX_V(!info.allowCostantReads || ((info.size & 15) == 0), "Constant buffer size must be aligned to 16 bytes", nullptr);

		// describe buffer
		outSetup.flags = info.computeFlags();
		outSetup.size = info.size;
		outSetup.stride = info.stride;

		if (info.initialLayout == ResourceLayout::INVALID)
			outSetup.layout = info.computeDefaultLayout();
		else
			outSetup.layout = info.initialLayout;

		if (info.stride)
			outSetup.flags |= BufferViewFlag::Structured;

		// seems valid enough
		return true;
	}

	static uint32_t CalcMaxMipCount(const ImageCreationInfo& setup)
	{
		auto w = setup.width;
		auto h = setup.height;
		auto d = setup.depth;

		uint32_t mipCount = 1;
		while (w > 1 || h > 1 || d > 1)
		{
			w = std::max<uint32_t>(1, w / 2);
			h = std::max<uint32_t>(1, h / 2);
			d = std::max<uint32_t>(1, d / 2);
			mipCount += 1;
		}

		return mipCount;
	}

	bool IDevice::validateImageCreationSetup(const ImageCreationInfo& info, ImageObject::Setup& outSetup) const
	{
		DEBUG_CHECK_RETURN_EX_V(info.width >= 1 && info.height >= 1 && info.depth >= 1, "Invalid image size", false);
		DEBUG_CHECK_RETURN_EX_V(info.format != ImageFormat::UNKNOWN, "Unknown image format", false);

		uint32_t maxMipCount = CalcMaxMipCount(info);
		DEBUG_CHECK_RETURN_EX_V(info.numMips != 0, "Invalid mip count", false);
		DEBUG_CHECK_RETURN_EX_V(info.numMips <= maxMipCount, "To many mipmaps for image of this size", false);

		auto array = (info.view == ImageViewType::View1DArray) || (info.view == ImageViewType::View2DArray) || (info.view == ImageViewType::ViewCubeArray) || (info.view == ImageViewType::ViewCube);
		DEBUG_CHECK_RETURN_EX_V(info.numSlices >= 1, "Invalid slice count", false);
		DEBUG_CHECK_RETURN_EX_V(array || info.numSlices == 1, "Only array images may have slices", false);
		DEBUG_CHECK_RETURN_EX_V(!array || info.numSlices >= 1, "Array images should have slices", false);

		if (info.view == ImageViewType::View1D || info.view == ImageViewType::View1DArray)
		{
			DEBUG_CHECK_RETURN_EX_V(info.height == 1, "1D texture should have no height", false);
			DEBUG_CHECK_RETURN_EX_V(info.depth == 1, "1D texture should have no depth", false);
		}
		else if (info.view == ImageViewType::View2D || info.view == ImageViewType::View2DArray)
		{
			DEBUG_CHECK_RETURN_EX_V(info.depth == 1, "2D texture should have no depth", false);
		}
		if (info.view == ImageViewType::ViewCube)
		{
			DEBUG_CHECK_RETURN_EX_V(info.numSlices == 6, "Cubemap must have 6 slices", false);
			DEBUG_CHECK_RETURN_EX_V(info.width == info.height, "Cubemap must be square", false);
			DEBUG_CHECK_RETURN_EX_V(info.depth == 1, "Cubemap must can't have depth", false);
		}
		else if (info.view == ImageViewType::ViewCubeArray)
		{
			DEBUG_CHECK_RETURN_EX_V(info.numSlices >= 6, "Cubemap must have 6 slices", false);
			DEBUG_CHECK_RETURN_EX_V((info.numSlices % 6) == 0, "Cubemap array must have multiple of 6 slices", false);
			DEBUG_CHECK_RETURN_EX_V(info.width == info.height, "Cubemap must be square", false);
			DEBUG_CHECK_RETURN_EX_V(info.depth == 1, "Cubemap must can't have depth", false);
		}

		if (info.multisampled())
		{
			DEBUG_CHECK_RETURN_EX_V(info.allowRenderTarget, "Multisampling is only allowed for render targets", false);
		}

		// setup flags
		auto flags = info.computeViewFlags();

		// configure
		outSetup.key = ImageViewKey(info.format, info.view, 0, info.numSlices, 0, info.numMips);
		outSetup.width = info.width;
		outSetup.height = info.height;
		outSetup.depth = info.depth;
		outSetup.flags = info.computeViewFlags();
		outSetup.numSamples = info.numSamples;
		return true;
	}

	//--

} // rendering
