/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver #]
***/

#include "build.h"

#include "glDriver.h"
#include "glBuffer.h"
#include "glImage.h"
#include "glSampler.h"
#include "glDriverThread.h"
#include "glShaderLibraryAdapter.h"
#include "glObjectCache.h"

namespace rendering
{
    namespace gl4
    {

        ///---

        BufferView Driver::createBuffer(const BufferCreationInfo& info, const SourceData* sourceData /*= nullptr*/)
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
                return BufferView();

            // constant buffer can't be read as a format buffer
            DEBUG_CHECK_EX(!info.allowCostantReads || !info.allowShaderReads, "Buffer can't be used as uniform and shader buffer at the same time (it might by UAV buffer though)");
            if (info.allowCostantReads && info.allowShaderReads)
                return BufferView();

            // create the buffer in the static pool
            auto buffer = Buffer::CreateBuffer(this, info, sourceData);
            if (!buffer)
            {
                TRACE_ERROR("Failed to create buffer {}", info);
                return BufferView();
            }

            // create the default view wrapper of the whole buffer
            auto flags = info.computeFlags();
            if (info.stride)
                flags |= BufferViewFlag::Structured;
            return BufferView(buffer->handle(), 0, info.size, info.stride, flags);
        }

        ///---

        ImageView Driver::createImage(const ImageCreationInfo& info, const SourceData* sourceData /*= nullptr*/)
        {
            // NOTE: no lock since the actual buffer shit is created on first actual binding

            // create the image in the static pool
            auto image = Image::CreateImage(this, info, sourceData);
            if (!image)
            {
                TRACE_ERROR("Failed to create image {}", info);
                return ImageView();
            }

            // fill image with source data
            if (sourceData != nullptr)
            {
                // TODO
            }

            // setup flags
            auto flags = info.computeViewFlags();
            if (sourceData)
                flags |= ImageViewFlag::Preinitialized;

            // if it's a writable render target make sure it's created right away
            /*if (info.allowRenderTarget || info.allowUAV)
            {
                m_thread->run([image]()
                    {
                        image->ensureInitialized();
                    });
            }*/

            // return read only view for the created object
            ImageViewKey key(info.format, info.view, 0, info.numSlices, 0, info.numMips);
            return ImageView(image->handle(), key, info.numSamples, info.width, info.height, info.depth, flags, info.sampler);
        }

        ObjectID Driver::createSampler(const SamplerState& info)
        {
            auto sampler = MemNew(Sampler, this, info);
            return sampler->handle();
        }

        void Driver::createPredefinedSampler(uint32_t id, const SamplerState& info, const char* debugName)
        {
            m_predefinedSamplers[id] = m_objectCache->resolveSampler(info);
        }

        void Driver::createPredefinedImageFromColor(uint32_t id, const base::Color fillColor, const ImageFormat format, const char *debugName)
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
            auto view = createImage(setup, &data);

            // get the created object and put it in the map
            auto image  = ResolveStaticObject<Image>(view.id());
            ASSERT(image != nullptr);            
            ASSERT(m_predefinedImages[id] == nullptr);
            m_predefinedImages[id] = image;

            // make sure default object is initialized
            image->ensureInitialized();
        }

        void Driver::createPredefinedRenderTarget(uint32_t id, const ImageFormat format, uint32_t numSlices, const char* debugName)
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
            auto view = createImage(setup);
            auto image = ResolveStaticObject<Image>(view.id());
            ASSERT(image != nullptr);
            ASSERT(m_predefinedImages[id] == nullptr);
            m_predefinedImages[id] = image;

            // make sure default object is initialized
            image->ensureInitialized();
        }

        void Driver::releaseObject(const ObjectID id)
        {
            if (id.isStatic())
            {
                auto object  = static_cast<Object*>(id.internalPointer());

                if (object->objectType() == ObjectType::Output)
                {
                    auto* output = static_cast<DriverOutput*>(object);
                    if (output->windowHandle)
                        m_windows->disconnectWindow(output->windowHandle);
                }

                m_thread->releaseObject(object);
            }
        }

        ObjectID Driver::createShaders(const ShaderLibraryData* shaders)
        {
            if (shaders)
            {
                auto object = MemNew(ShaderLibraryAdapter, this, shaders);
                return object->handle();
            }
            
            return ObjectID();
        }

        ResolvedImageView Driver::resolvePredefinedImage(const ImageView& view) const
        {
            auto id = view.id().internalIndex();

            if (id < ARRAY_COUNT(m_predefinedImages))
                if (auto image = m_predefinedImages[id])
                    return image->resolveView(view.key());

            return ResolvedImageView();
        }

        GLuint Driver::resolvePredefinedSampler(uint32_t id) const
        {
            if (id < ARRAY_COUNT(m_predefinedSamplers))
                return m_predefinedSamplers[id];

            return 0;
        }

    } // gl4
} // rendering
