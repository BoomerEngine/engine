/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "renderingResources.h"

#include "base/containers/include/stringBuilder.h"

#include <stdarg.h>

namespace rendering
{
    //--

    RTTI_BEGIN_TYPE_ENUM(ImageViewType);
        RTTI_ENUM_OPTION(View1D);
        RTTI_ENUM_OPTION(View2D);
        RTTI_ENUM_OPTION(View3D);
        RTTI_ENUM_OPTION(ViewCube);
        RTTI_ENUM_OPTION(View1DArray);
        RTTI_ENUM_OPTION(View2DArray);
        RTTI_ENUM_OPTION(ViewCubeArray);
    RTTI_END_TYPE();

    //--

    bool ImageCreationInfo::operator==(const ImageCreationInfo& other) const
    {
        return (view == other.view)
            && (format == other.format)
            && (width == other.width)
            && (height == other.height)
            && (depth == other.depth)
            && (numMips == other.numMips)
            && (numSlices == other.numSlices)
            && (multisampled() == other.multisampled())
            && (allowRenderTarget == other.allowRenderTarget)
            && (allowDynamicUpdate == other.allowDynamicUpdate)
            && (allowCopies == other.allowCopies)
            && (allowShaderReads == other.allowShaderReads)
            && (allowUAV == other.allowUAV);
    }

    bool ImageCreationInfo::operator!=(const ImageCreationInfo& other) const
    {
        return !operator==(other);
    }

    ImageViewFlags ImageCreationInfo::computeViewFlags() const
    {
        ImageViewFlags flags;

        if (allowShaderReads) flags |= ImageViewFlag::ShaderReadable;
        if (allowUAV) flags |= ImageViewFlag::UAVCapable;
        if (allowCopies) flags |= ImageViewFlag::CopyCapable;
        if (allowRenderTarget) flags |= ImageViewFlag::RenderTarget;
        if (allowDynamicUpdate) flags |= ImageViewFlag::Dynamic;

        if (multisampled()) flags |= ImageViewFlag::Multisampled;

        if (GetImageFormatInfo(format).compressed) flags |= ImageViewFlag::Compressed;
        if (GetImageFormatInfo(format).formatClass == ImageFormatClass::SRGB) flags |= ImageViewFlag::SRGB;
        if (GetImageFormatInfo(format).formatClass == ImageFormatClass::DEPTH) flags |= ImageViewFlag::Depth;

        return flags;
    }

    void ImageCreationInfo::print(base::IFormatStream& ret) const
    {
        switch (view)
        {
        case ImageViewType::View1D: ret.append("1D "); break;
        case ImageViewType::View1DArray: ret.append("1DArray "); break;
        case ImageViewType::View2D: ret.append("2D "); break;
        case ImageViewType::View2DArray: ret.append("2DArray "); break;
        case ImageViewType::View3D: ret.append("3D "); break;
        case ImageViewType::ViewCube: ret.append("Cube "); break;
        case ImageViewType::ViewCubeArray: ret.append("CubeArray "); break;
        }

        ret.append(GetImageFormatInfo(format).name);

        if (view == ImageViewType::View1D || view == ImageViewType::View1DArray)
            ret.appendf("{} ", width);
        else if (view == ImageViewType::View2D || view == ImageViewType::View2DArray)
            ret.appendf("{}x{} ", width, height);
        else if (view == ImageViewType::View3D)
            ret.appendf("{}x{}x{} ", width, height, depth);
        else if (view == ImageViewType::ViewCube || view == ImageViewType::ViewCubeArray)
            ret.appendf("{}x{}x6 ", width, width);

        if (numMips > 1)
            ret.appendf("{} mips ", numMips);

        if (numSlices > 1)
            ret.appendf("{} slices ", numSlices);

        if (allowShaderReads) ret << ", SHADER";
        if (allowDynamicUpdate) ret << ", DYNAMIC";
        if (allowCopies) ret << ", COPY";
        if (allowRenderTarget) ret << ", RT";
        if (allowUAV) ret << ", UAV";
    }

    uint32_t ImageCreationInfo::calcMemoryUsage() const
    {
        uint32_t totalMemory = 0;

        uint32_t sizePerPixel = 4; // TODO!

        uint32_t sizePerLayer = 0;
        auto localWidth = width;
        auto localHeight = height;
        auto localDepth = depth;
        for (uint32_t i = 0; i < numMips; ++i)
        {
            sizePerLayer += (localWidth * localHeight * localDepth) * sizePerPixel;

            localWidth = std::max<uint16_t>(1, localWidth / 2);
            localHeight = std::max<uint16_t>(1, localHeight / 2);
            localDepth = std::max<uint16_t>(1, localDepth / 2);
        }

        totalMemory += sizePerLayer * numSlices;
        return totalMemory;
    }

    uint32_t ImageCreationInfo::hash() const
    {
        base::CRC32 crc;
        crc << allowShaderReads;
        crc << allowDynamicUpdate;
        crc << allowCopies;
        crc << allowRenderTarget;
        crc << allowUAV;
        crc << (uint8_t)view;
        crc << (uint8_t)format;
        crc << width;
        crc << height;
        crc << depth;
        crc << numMips;
        crc << numSlices;
        crc << numSamples;
        return crc.crc();
    }

    //--

    BufferViewFlags BufferCreationInfo::computeFlags() const
    {
        BufferViewFlags ret;

        if (allowCostantReads) ret |= BufferViewFlag::Constants;
        if (allowShaderReads) ret |= BufferViewFlag::ShaderReadable;
        if (allowDynamicUpdate) ret |= BufferViewFlag::Dynamic;
        if (allowCopies) ret |= BufferViewFlag::CopyCapable;
        if (allowUAV) ret |= BufferViewFlag::UAVCapable;
        if (allowVertex) ret |= BufferViewFlag::Vertex;
        if (allowIndex) ret |= BufferViewFlag::Index;
        if (allowIndirect) ret |= BufferViewFlag::IndirectArgs;

        return ret;
    }

    void BufferCreationInfo::print(base::IFormatStream& f) const
    {
        f.appendf("{} bytes", size);

        if (stride != 0)
            f.appendf(", {} stride", size);

        if (allowCostantReads) f << ", ConstantReads";
        if (allowShaderReads) f << ", ShaderReader";
        if (allowDynamicUpdate) f << ", DynamicUpdate";
        if (allowCopies) f << ", CopyCapable";
        if (allowUAV) f << ", UAVCapable";
        if (allowVertex) f << ", Vertex";
        if (allowIndex) f << ", Index";
        if (allowIndirect) f << ", Indirect";
    }

    //--

} // rendering

