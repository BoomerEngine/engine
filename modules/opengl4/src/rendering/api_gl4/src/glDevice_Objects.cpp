/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"

#include "glDevice.h"
#include "glBuffer.h"
#include "glImage.h"
#include "glSampler.h"
#include "glDeviceThread.h"
#include "glShaders.h"
#include "glObjectCache.h"
#include "glObjectRegistry.h"

namespace rendering
{
    namespace gl4
    {
        ///---

        void Device::createPredefinedImages()
        {
            //--

            createPredefinedImageFromColor(ID_BlackTexture, base::Color(0, 0, 0, 0), ImageFormat::RGBA8_UNORM, "DefaultBlack");
            createPredefinedImageFromColor(ID_WhiteTexture, base::Color(255, 255, 255, 255), ImageFormat::RGBA8_UNORM, "DefaultWhite");
            createPredefinedImageFromColor(ID_GrayLinearTexture, base::Color(127, 127, 127, 255), ImageFormat::RGBA8_UNORM, "DefaultLinearGray");
            createPredefinedImageFromColor(ID_GraySRGBTexture, base::Color(170, 170, 170, 255), ImageFormat::RGBA8_UNORM, "DefaultGammaGray");
            createPredefinedImageFromColor(ID_NormZTexture, base::Color(127, 127, 255, 255), ImageFormat::RGBA8_UNORM, "DefaultNormal");

            //--

            createPredefinedRenderTarget(ID_DefaultDepthRT, ImageFormat::D24S8, 1, "DefaultDepthRT");
            createPredefinedRenderTarget(ID_DefaultColorRT, ImageFormat::RGBA16F, 1, "DefaultColorRT");
            createPredefinedRenderTarget(ID_DefaultDepthArrayRT, ImageFormat::D24S8, 4, "DefaultDepthArrayRT");

            //--
        }

        void Device::createPredefinedSamplers()
        {
            {
                SamplerState info;
                info.magFilter = FilterMode::Nearest;
                info.minFilter = FilterMode::Nearest;
                info.mipmapMode = MipmapFilterMode::Nearest;
                createPredefinedSampler(ID_SamplerClampPoint, info, "ClampPoint");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapPoint, info, "WrapPoint");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Nearest;
                createPredefinedSampler(ID_SamplerClampBiLinear, info, "ClampBilinear");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapBiLinear, info, "WrapBilinear");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Linear;
                createPredefinedSampler(ID_SamplerClampTriLinear, info, "ClampTrilinear");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapTriLinear, info, "WrapTrilinear");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Linear;
                info.maxAnisotropy = 16; // TODO: limit by driver settings
                createPredefinedSampler(ID_SamplerClampAniso, info, "ClampAniso");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapAniso, info, "WrapAniso");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Nearest;
                info.minFilter = FilterMode::Nearest;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::LessEqual;
                createPredefinedSampler(ID_SamplerPointDepthLE, info, "PointDepthLT");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Nearest;
                info.minFilter = FilterMode::Nearest;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::GreaterEqual;
                createPredefinedSampler(ID_SamplerPointDepthGE, info, "PointDepthGT");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::LessEqual;
                createPredefinedSampler(ID_SamplerBiLinearDepthLE, info, "BilinearDepthLT");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::GreaterEqual;
                createPredefinedSampler(ID_SamplerBiLinearDepthGE, info, "BilinearDepthGT");
            }
        }

        ///---

        BufferObjectPtr Device::createBuffer(const BufferCreationInfo& info, const SourceData* sourceData /*= nullptr*/)
        {
            // NOTE: no lock since the actual buffer shit is created on first actual binding
            
            uint8_t numExclusiveModes = 0;
            if (info.allowCostantReads) numExclusiveModes += 1;
            if (info.allowVertex) numExclusiveModes += 1;
            if (info.allowIndirect) numExclusiveModes += 1;
            if (info.allowIndex) numExclusiveModes += 1;
            //if (info.allowShaderReads) numExclusiveModes += 1;
            DEBUG_CHECK_EX(numExclusiveModes <= 1, "Only one of the exclusive primary usage modes must be set: vertex, index, uniform, shader or indirect");
            if (numExclusiveModes > 1)
                return nullptr;

            // constant buffer can't be read as a format buffer
            DEBUG_CHECK_EX(!info.allowCostantReads || !info.allowShaderReads, "Buffer can't be used as uniform and shader buffer at the same time (it might by UAV buffer though)");
            if (info.allowCostantReads && info.allowShaderReads)
                return nullptr;

            // create the buffer in the static pool
            auto buffer = Buffer::CreateBuffer(this, info, sourceData);
            if (!buffer)
                return nullptr;

            // create the default view wrapper of the whole buffer
            auto flags = info.computeFlags();
            if (info.stride)
                flags |= BufferViewFlag::Structured;

            // create the wrapper
            const auto view = BufferView(buffer->handle(), 0, info.size, info.stride, flags);
            return base::RefNew<BufferObject>(view, m_objectRegistry->proxy());
        }

        ///---

        ImageObjectPtr Device::createImage(const ImageCreationInfo& info, const SourceData* sourceData /*= nullptr*/)
        {
            // create the image in the static pool
            auto image = Image::CreateImage(this, info, sourceData);
            if (!image)
                return nullptr;

            // setup flags
            auto flags = info.computeViewFlags();
            if (sourceData)
                flags |= ImageViewFlag::Preinitialized;

            // return read only view for the created object
            const auto key = ImageViewKey(info.format, info.view, 0, info.numSlices, 0, info.numMips);
            const auto view = ImageView(image->handle(), key, info.numSamples, info.width, info.height, info.depth, flags, info.sampler);
            return base::RefNew<ImageObject>(view, m_objectRegistry->proxy());
        }

        SamplerObjectPtr Device::createSampler(const SamplerState& info)
        {
            auto sampler = new Sampler(this, info);
            return base::RefNew<SamplerObject>(sampler->handle(), m_objectRegistry->proxy());
        }

        void Device::createPredefinedSampler(uint32_t id, const SamplerState& info, const char* debugName)
        {
            m_predefinedSamplers[id] = m_objectCache->resolveSampler(info);
        }

        void Device::createPredefinedImageFromColor(uint32_t id, const base::Color fillColor, const ImageFormat format, const char *debugName)
        {
            ImageCreationInfo setup;
            setup.width = 16;
            setup.height = 16;
            setup.depth = 1;
            setup.view = ImageViewType::View2D;
            setup.format = format;
            setup.numSlices = 1;
            setup.numMips = 1;
            setup.label = debugName;
            setup.allowUAV = true;
            setup.allowShaderReads = true;
            setup.allowCopies = true;

            base::Array<base::Color> colors;
            colors.resize(256);

            for (uint32_t i = 0; i < 256; ++i)
                colors[i] = fillColor;

            SourceData data;
            data.data = base::Buffer::Create(POOL_TEMP, colors.dataSize(), 16, colors.typedData());
            data.size = colors.dataSize();
            auto image = Image::CreateImage(this, setup, &data);

            // get the created object and put it in the map
            ASSERT(m_predefinedImages[id] == nullptr);
            m_predefinedImages[id] = image;

            // make sure default object is initialized
            image->ensureInitialized();
        }

        void Device::createPredefinedRenderTarget(uint32_t id, const ImageFormat format, uint32_t numSlices, const char* debugName)
        {
            ImageCreationInfo setup;
            setup.width = 16;
            setup.height = 16;
            setup.depth = 1;
            setup.view = (numSlices == 0) ? ImageViewType::View2D : ImageViewType::View2DArray;
            setup.format = format;
            setup.numSlices = std::max<uint32_t>(1, numSlices);
            setup.numMips = 1;
            setup.label = debugName;
            setup.allowUAV = true;
            setup.allowShaderReads = true;
            setup.allowCopies = true;
            setup.allowRenderTarget = true;

            // get the created object and put it in the map
            auto image = Image::CreateImage(this, setup, nullptr);
            ASSERT(m_predefinedImages[id] == nullptr);
            m_predefinedImages[id] = image;

            // make sure default object is initialized
            image->ensureInitialized();
        }

        ShaderObjectPtr Device::createShaders(const ShaderLibraryData* shaders)
        {
            DEBUG_CHECK_RETURN_V(shaders, nullptr);

            auto object = new Shaders(this, shaders);
            return base::RefNew<ShaderObject>(object->handle(), AddRef(shaders), m_objectRegistry->proxy());
        }

        ResolvedImageView Device::resolvePredefinedImage(const ImageView& view) const
        {
            DEBUG_CHECK(view.id().isPredefined());

            auto id = view.id().index();

            if (id < ARRAY_COUNT(m_predefinedImages))
                if (auto image = m_predefinedImages[id])
                    return image->resolveView(view.key());

            return ResolvedImageView();
        }

        GLuint Device::resolvePredefinedSampler(uint32_t id) const
        {
            if (id < ARRAY_COUNT(m_predefinedSamplers))
                return m_predefinedSamplers[id];

            return 0;
        }

    } // gl4
} // rendering
