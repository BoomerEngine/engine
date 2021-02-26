/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\object #]
***/

#include "build.h"
#include "renderingImage.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//---

void ImageViewKey::print(IFormatStream& f) const
{
    f << format;

    switch (viewType)
    {
    case ImageViewType::View1D: f << "1D"; break;
    case ImageViewType::View1DArray: f << "1DArray"; break;
    case ImageViewType::View2D: f << "2D"; break;
    case ImageViewType::View2DArray: f << "2DArray"; break;
    case ImageViewType::ViewCube: f << "CUBE"; break;
    case ImageViewType::ViewCubeArray: f << "CUBEArray"; break;
    case ImageViewType::View3D: f << "3D"; break;
    }

    if (viewType == ImageViewType::View1DArray || viewType == ImageViewType::View2DArray || viewType == ImageViewType::ViewCubeArray)
    {
        if (firstSlice == 0)
            f.appendf(", {} slices", numSlices);
        else
            f.appendf(", {} slices @{}", numSlices, firstSlice);
    }

    if (numMips > 1 || firstMip != 0)
    {
        if (firstMip == 0)
            f.appendf(", {} mips", numMips);
        else
            f.appendf(", {} mips @{}", numMips, firstMip);
    }
}

void ImageViewKey::validate()
{
    DEBUG_CHECK_EX(format != ImageFormat::UNKNOWN, "Unknown format set");
    DEBUG_CHECK_EX(numMips >= 1, "Invalid mip count");
    DEBUG_CHECK_EX(numSlices >= 1, "Invalid slice count");

    if (viewType == ImageViewType::View1D || viewType == ImageViewType::View2D || viewType == ImageViewType::View3D)
    {
        DEBUG_CHECK_EX(numSlices == 1, "Non-array views require a single array slice");
    }

    if (viewType == ImageViewType::ViewCube || viewType == ImageViewType::ViewCubeArray)
    {
        DEBUG_CHECK_EX(numSlices % 6 == 0, "Cubemap require 6 array slices");
        DEBUG_CHECK_EX(firstSlice % 6 == 0, "Cubemap require 6 array slices");
    }
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(ImageObject);
RTTI_END_TYPE();

ImageObject::ImageObject(ObjectID id, IDeviceObjectHandler* impl, const Setup& setup)
    : IDeviceObject(id, impl)
    , m_key(setup.key)
    , m_flags(setup.flags)
    , m_width(setup.width)
    , m_height(setup.height)
    , m_depth(setup.depth)
    , m_numSamples(setup.numSamples)
	, m_initialLayout(setup.initialLayout)
{
    DEBUG_CHECK_EX(m_numSamples >= 1, "Invalid sample count");
    DEBUG_CHECK_EX(m_width >= 1, "Invalid width");
    DEBUG_CHECK_EX(m_depth >= 1, "Invalid depth");
    DEBUG_CHECK_EX(m_height >= 1, "Invalid height");

    // auto fix flags
    if (GetImageFormatInfo(m_key.format).formatClass == ImageFormatClass::SRGB)
    {
        m_flags |= ImageViewFlag::SRGB;
    }
    else
    {
        DEBUG_CHECK_EX(!m_flags.test(ImageViewFlag::SRGB), "SRGB flag can only be set for image views with SRGB format");
        m_flags -= ImageViewFlag::SRGB;
    }

    // auto fix flags
    if (GetImageFormatInfo(m_key.format).formatClass == ImageFormatClass::DEPTH)
    {
        m_flags |= ImageViewFlag::Depth;
    }
    else
    {
        DEBUG_CHECK_EX(!m_flags.test(ImageViewFlag::Depth), "Depth flag can only be set for image views with depth format");
        m_flags -= ImageViewFlag::Depth;
    }
}

ImageObject::~ImageObject()
{}

uint32_t ImageObject::calcMemorySize() const
{
    uint32_t numPixels = (uint32_t)m_width * (uint32_t)m_height * (uint32_t)m_depth;
    numPixels *= std::max<uint32_t>(m_numSamples, 1);
    numPixels *= m_key.numSlices;
    return numPixels * GetImageFormatInfo(m_key.format).bitsPerPixel / 8;
}

bool ImageObject::validateSampledView(uint32_t firstMip, uint32_t numMips, uint32_t firstSlice, uint32_t numSlices, ImageSampledView::Setup& outSetup) const
{
    if (numMips == INDEX_MAX)
        numMips = mips() - firstMip;

    if (numSlices == INDEX_MAX)
        numSlices = slices() - firstSlice;

    DEBUG_CHECK_RETURN_V(shaderReadable(), false);
    DEBUG_CHECK_RETURN_V(firstSlice < slices(), false);
	DEBUG_CHECK_RETURN_V(firstMip < mips(), false);
	DEBUG_CHECK_RETURN_V(firstMip + numMips <= mips(), false);
    DEBUG_CHECK_RETURN_V(firstSlice + numSlices <= slices(), false);

    /*if (numSlices > 1 || firstSlice)
        DEBUG_CHECK_RETURN_V(type() == ImageViewType::View1DArray || type() == ImageViewType::View2DArray || type() == ImageViewType::ViewCube, false);*/

    outSetup.firstMip = firstMip;
    outSetup.firstSlice = firstSlice;
    outSetup.numMips = numMips;
    outSetup.numSlices = numSlices;

    return true;
}

bool ImageObject::validateReadOnlyView(uint32_t mip, uint32_t slice, ImageReadOnlyView::Setup& outSetup) const
{
	DEBUG_CHECK_RETURN_V(shaderReadable(), false);
	DEBUG_CHECK_RETURN_V(slice < slices(), false);
	DEBUG_CHECK_RETURN_V(mip < mips(), false);

	outSetup.mip = mip;
	outSetup.slice = slice;
	return true;
}

bool ImageObject::validateWritableView(uint32_t mip, uint32_t slice, ImageWritableView::Setup& outSetup) const
{
    DEBUG_CHECK_RETURN_V(uavCapable(), false);
    DEBUG_CHECK_RETURN_V(slice < slices(), false);
    DEBUG_CHECK_RETURN_V(mip < mips(), false);

    outSetup.mip = mip;
    outSetup.slice = slice;
    return true;
}

bool ImageObject::validateRenderTargetView(uint32_t mip, uint32_t firstSlice, uint32_t numSlices, RenderTargetView::Setup& outSetup) const
{
    DEBUG_CHECK_RETURN_V(type() == ImageViewType::View2D || type() == ImageViewType::View2DArray || type() == ImageViewType::ViewCube || type() == ImageViewType::ViewCubeArray, false);

    if (numSlices == INDEX_MAX)
        numSlices = slices() - firstSlice;

    DEBUG_CHECK_RETURN_V(renderTarget(), false);
	DEBUG_CHECK_RETURN_V(numSlices > 0, false);
	DEBUG_CHECK_RETURN_V(firstSlice < slices(), false);
    DEBUG_CHECK_RETURN_V(numSlices <= slices(), false);
    DEBUG_CHECK_RETURN_V(firstSlice + numSlices <= slices(), false);
    DEBUG_CHECK_RETURN_V(mip < mips(), false);

	if (type() == ImageViewType::View2DArray)
	{
		if (numSlices == 1)
		{
			outSetup.arrayed = false;
			outSetup.type = ImageViewType::View2D;
		}
		else
		{
			outSetup.arrayed = true;
			outSetup.type = ImageViewType::View2DArray;
		}
	}
	else if (type() == ImageViewType::ViewCube || type() == ImageViewType::ViewCubeArray)
	{
		if (numSlices == 1)
		{
			outSetup.arrayed = false;
			outSetup.type = ImageViewType::View2D;
		}
		else
		{
			DEBUG_CHECK_RETURN_V((numSlices % 6) == 0, false);
			DEBUG_CHECK_RETURN_V((firstSlice == 0), false);
			outSetup.arrayed = true;
			outSetup.type = type();
		}
	}
	else
	{
		DEBUG_CHECK_RETURN_V(numSlices == 1, false);
		DEBUG_CHECK_RETURN_V(firstSlice == 0, false);
		outSetup.arrayed = false;
		outSetup.type = ImageViewType::View2D;
	}

    outSetup.format = format();
	outSetup.depth = renderTargetDepth();
    outSetup.flipped = flippedY();
    outSetup.firstSlice = firstSlice;
    outSetup.numSlices = numSlices;
    outSetup.msaa = multisampled();
    outSetup.samples = samples();
    outSetup.srgb = srgb();
    outSetup.swapchain = false;
    outSetup.width = std::max<uint32_t>(m_width >> mip, 1);
    outSetup.height = std::max<uint32_t>(m_height >> mip, 1);

    return true;
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ImageSampledView);
RTTI_END_TYPE();

ImageSampledView::ImageSampledView(ObjectID viewId, ImageObject* img, IDeviceObjectHandler* impl, const Setup& setup)
	: IDeviceObjectView(viewId, DeviceObjectViewType::SampledImage, img, impl)
	, m_firstMip(setup.firstMip)
	, m_firstSlice(setup.firstSlice)
	, m_numMips(setup.numMips)
	, m_numSlices(setup.numSlices)
{
	m_width = std::max<uint32_t>(1, img->width() >> setup.firstMip);
	m_height = std::max<uint32_t>(1, img->height() >> setup.firstMip);
	m_depth = std::max<uint32_t>(1, img->depth() >> setup.firstMip);
}

ImageSampledView::~ImageSampledView()
{}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ImageReadOnlyView);
RTTI_END_TYPE();

ImageReadOnlyView::ImageReadOnlyView(ObjectID viewId, ImageObject* img, IDeviceObjectHandler* impl, const Setup& setup)
    : IDeviceObjectView(viewId, DeviceObjectViewType::Image, img, impl)
	, m_mip(setup.mip)
	, m_slice(setup.slice)
{}

ImageReadOnlyView::~ImageReadOnlyView()
{}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ImageWritableView);
RTTI_END_TYPE();

ImageWritableView::ImageWritableView(ObjectID viewId, ImageObject* img, IDeviceObjectHandler* impl, const Setup& setup)
    : IDeviceObjectView(viewId, DeviceObjectViewType::ImageWritable, img, impl)
    , m_mip(setup.mip)
    , m_slice(setup.slice)
{}

ImageWritableView::~ImageWritableView()
{}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(RenderTargetView);
RTTI_END_TYPE();

RenderTargetView::RenderTargetView(ObjectID viewId, IDeviceObject* owner, IDeviceObjectHandler* impl, const Setup& setup)
    : IDeviceObjectView(viewId, DeviceObjectViewType::RenderTarget, owner, impl)
    , m_width(setup.width)
    , m_height(setup.height)
    , m_format(setup.format)
    , m_samples(setup.samples)
    , m_mip(setup.mip)
    , m_firstSlice(setup.firstSlice)
    , m_numSlices(setup.numSlices)
    , m_flipped(setup.flipped)
    , m_swapchain(setup.swapchain)
    , m_depth(setup.depth)
    , m_srgb(setup.srgb)
    , m_array(setup.arrayed)
{}

RenderTargetView::~RenderTargetView()
{}

//--

END_BOOMER_NAMESPACE_EX(gpu)
