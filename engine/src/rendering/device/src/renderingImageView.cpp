/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\views #]
***/

#include "build.h"
#include "renderingImageView.h"
#include "renderingPredefinedObjects.h"

namespace rendering
{

    //---

    static ImageViewType CollapseArrayView(ImageViewType arrayViewType)
    {
        switch (arrayViewType)
        {
            case ImageViewType::View1DArray: return ImageViewType::View1D;
            case ImageViewType::View2DArray: return ImageViewType::View2D;
            case ImageViewType::ViewCubeArray: return ImageViewType::ViewCube;
        }

        ASSERT(!"Invalid array view type");
        return ImageViewType::View2D;
    }

    //---

    void ImageViewKey::print(base::IFormatStream& f) const
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

    ImageView::ImageView()
        : ObjectView(ObjectViewType::Image)
        , m_sampler(ObjectID::DefaultBilinearSampler())
    {}

    ImageView::ImageView(ObjectID id, ImageViewKey key, uint8_t numSamples, uint16_t width, uint16_t height, uint16_t depth, ImageViewFlags flags, ObjectID sampler)
        : ObjectView(ObjectViewType::Image, id)
        , m_key(key)
        , m_numSamples(numSamples)
        , m_width(width)
        , m_height(height)
        , m_depth(depth)
        , m_flags(flags)
        , m_sampler(sampler)
    {
        DEBUG_CHECK_EX(numSamples >= 1, "Invalid sample count");
        DEBUG_CHECK_EX(m_width >= 1, "Invalid width");
        DEBUG_CHECK_EX(m_depth >= 1, "Invalid depth");
        DEBUG_CHECK_EX(m_height >= 1, "Invalid height");

        // auto fix flags
        if (GetImageFormatInfo(key.format).formatClass == ImageFormatClass::SRGB)
        {
            m_flags |= ImageViewFlag::SRGB;
        }
        else
        {
            DEBUG_CHECK_EX(!m_flags.test(ImageViewFlag::SRGB), "SRGB flag can only be set for image views with SRGB format");
            m_flags -= ImageViewFlag::SRGB;
        }

        // auto fix flags
        if (GetImageFormatInfo(key.format).formatClass == ImageFormatClass::DEPTH)
        {
            m_flags |= ImageViewFlag::Depth;
        }
        else
        {
            DEBUG_CHECK_EX(!m_flags.test(ImageViewFlag::Depth), "Depth flag can only be set for image views with depth format");
            m_flags -= ImageViewFlag::Depth;
        }
    }

    uint32_t ImageView::calcMemorySize() const
    {
        uint32_t numPixels = (uint32_t)m_width * (uint32_t)m_height * (uint32_t)m_depth;
        numPixels *= std::max<uint32_t>(m_numSamples, 1);
        numPixels *= m_key.numSlices;
        return numPixels * GetImageFormatInfo(m_key.format).bitsPerPixel / 8;
    }

    void ImageView::print(base::IFormatStream& f) const
    {
        f << id();

        if (!empty())
        {
            f << " " << m_key;

            if (m_key.viewType == ImageViewType::View1D || m_key.viewType == ImageViewType::View1DArray)
                f.appendf(", [{}]", m_width);
            else if (m_key.viewType == ImageViewType::View2D || m_key.viewType == ImageViewType::View2DArray)
                f.appendf(", [{}x{}]", m_width, m_height);
            else if (m_key.viewType == ImageViewType::View3D)
                f.appendf(", [{}x{}x{}]", m_width, m_height, m_depth);
            else if (m_key.viewType == ImageViewType::ViewCube || m_key.viewType == ImageViewType::ViewCubeArray)
                f.appendf(", [6x{}x{}]", m_width, m_height);

            if (m_numSamples > 0)
                f.appendf(", {} sample{}", m_numSamples, (m_numSamples != 1) ? "s" : "");

            if (m_flags.test(ImageViewFlag::RenderTarget))
                f.appendf(", RT");
            if (m_flags.test(ImageViewFlag::Depth))
                f.appendf(", DEPTH");
            else if (m_flags.test(ImageViewFlag::UAVCapable))
                f.appendf(", UAV");
            else if (m_flags.test(ImageViewFlag::Dynamic))
                f.appendf(", DYNAMIC");
            else if (m_flags.test(ImageViewFlag::Multisampled))
                f.appendf(", MULTISAMPLED");
            else if (m_flags.test(ImageViewFlag::Preinitialized))
                f.appendf(", PREINITIALIZED");
            else if (m_flags.test(ImageViewFlag::SRGB))
                f.appendf(", SRGB");
        }
    }

    //---

    ImageView ImageView::createArrayView(uint16_t firstRelativeSlice, uint16_t numSlices) const
    {
        DEBUG_CHECK_EX(firstRelativeSlice < m_key.numSlices, "Invalid first array slice index");
        uint16_t maxSlicesLeft = m_key.numSlices - firstRelativeSlice;
        DEBUG_CHECK_EX(!numSlices || numSlices <= maxSlicesLeft, "Invalid array slice count");
        uint16_t numSlicesLeft = numSlices ? std::min<uint16_t>(maxSlicesLeft, numSlices) : maxSlicesLeft;
        ImageViewKey key(m_key.format, m_key.viewType, m_key.firstSlice + firstRelativeSlice, numSlices, m_key.firstMip, m_key.numMips);
        return ImageView(id(), key, m_numSamples, m_width, m_height, m_depth, m_flags, m_sampler);
    }

    ImageView ImageView::createSingleSliceView(uint16_t relativeSliceIndex) const
    {
        DEBUG_CHECK_EX(m_key.array(), "Single slice view can be created only for arrays");
        DEBUG_CHECK_EX(m_key.firstSlice + relativeSliceIndex < m_key.numSlices, "Invalid array slice index");
        ImageViewKey key(m_key.format, CollapseArrayView(m_key.viewType), m_key.firstSlice + relativeSliceIndex, 1, m_key.firstMip, m_key.numMips);
        return ImageView(id(), key, m_numSamples, m_width, m_height, m_depth, m_flags, m_sampler);
    }

    ImageView ImageView::createSingleMipView(uint8_t relativeMipIndex) const
    {
        DEBUG_CHECK_EX(m_key.firstMip + relativeMipIndex < m_key.numMips, "Invalid first mip index");

        auto newWidth = std::max<uint16_t>(1, m_width >> relativeMipIndex);
        auto newHeight = std::max<uint16_t>(1, m_height >> relativeMipIndex);
        auto newDepth = std::max<uint16_t>(1, m_depth >> relativeMipIndex);

        ImageViewKey key(m_key.format, m_key.viewType, m_key.firstSlice, m_key.numSlices, m_key.firstMip + relativeMipIndex, 1);
        return ImageView(id(), key, m_numSamples, newWidth, newHeight, newDepth, m_flags, m_sampler);
    }

    ImageView ImageView::createSampledView(ObjectID sampler) const
    {
        DEBUG_CHECK_EX(id(), "Invalid view");
        DEBUG_CHECK_EX(sampler, "Invalid sampler");
        DEBUG_CHECK_EX(shaderReadable(), "Samplers have no sense on image views that are not readable by shaders");
        return ImageView(id(), m_key, m_numSamples, m_width, m_height, m_depth, m_flags, sampler);
    }

    //---


    static ImageView MakeDefaultTexture(uint8_t id)
    {
        ImageViewKey theDefaultImageKey(ImageFormat::RGBA8_UNORM, ImageViewType::View2D, 0, 1, 0, 1);
        auto flags = ImageViewFlags() | ImageViewFlag::Preinitialized | ImageViewFlag::ShaderReadable;
        return ImageView(ObjectID::CreatePredefinedID(id), theDefaultImageKey, 1, 16, 16, 1, flags, ObjectID::DefaultPointSampler());
    }

    ImageView ImageView::DefaultBlack()
    {
        static auto theView = MakeDefaultTexture(ID_BlackTexture);
        return theView;
    }

    ImageView ImageView::DefaultGrayLinear()
    {
        static auto theView = MakeDefaultTexture(ID_GrayLinearTexture);
        return theView;
    }

    ImageView ImageView::DefaultGraySRGB()
    {
        static auto theView = MakeDefaultTexture(ID_GraySRGBTexture);
        return theView;
    }

    ImageView ImageView::DefaultWhite()
    {
        static auto theView = MakeDefaultTexture(ID_WhiteTexture);
        return theView;
    }

    ImageView ImageView::DefaultFlatNormals()
    {
        static auto theView = MakeDefaultTexture(ID_NormZTexture);
        return theView;
    }

    ImageView ImageView::DefaultColorRT()
    {
        static auto theView = MakeDefaultTexture(ID_DefaultColorRT);
        return theView;
    }

    ImageView ImageView::DefaultDepthRT()
    {
        static auto theView = MakeDefaultTexture(ID_DefaultDepthRT);
        return theView;
    }

    ImageView ImageView::DefaultDepthArrayRT()
    {
        static auto theView = MakeDefaultTexture(ID_DefaultDepthArrayRT);
        return theView;
    }

    //--

} // rendering