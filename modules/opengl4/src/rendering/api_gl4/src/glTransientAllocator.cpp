/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\transient #]
***/

#include "build.h"
#include "glTransientAllocator.h"
#include "glTransientBufferAllocator.h"
#include "glDriver.h"
#include "glUtils.h"
#include "glImage.h"
#include "glBuffer.h"

#include "base/containers/include/stringBuilder.h"

namespace rendering
{
    namespace gl4
    {
        //---

        struct BufferConfig
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(BufferConfig);

        public:
            uint32_t m_size;
            uint32_t m_increment;

            BufferConfig(uint32_t size=0, uint32_t increment=0)
                : m_size(size)
                , m_increment(increment)
            {}
        };

        RTTI_BEGIN_TYPE_STRUCT(BufferConfig);
            RTTI_PROPERTY(m_size);
            RTTI_PROPERTY(m_increment);
        RTTI_END_TYPE();

        // get initial sizes
        base::ConfigProperty<BufferConfig> cvConstantsBuffer("Rendering.GL4", "ConstantsBuffer", BufferConfig(1024, 1024));
        base::ConfigProperty<BufferConfig> cvStatingBuffer("Rendering.GL4", "StagingBuffer", BufferConfig(1024, 1024));
        base::ConfigProperty<BufferConfig> cvGeometryBuffer("Rendering.GL4", "GeometryBuffer", BufferConfig(1024, 1024));
        base::ConfigProperty<BufferConfig> cvStorageBuffer("Rendering.GL4", "StorageBuffer", BufferConfig(1024, 1024));

        //---

        static uint64_t CalcTransientViewID(const ObjectID& id, uint32_t offset)
        {
            //return ((uint64_t)id.internalIndex() << 32) | offset;
            return id.internalIndex();
        }

        //---

        TransientAllocator::TransientAllocator(Driver* drv)
            : m_driver(drv)
        {
            // create the buffer pools
            m_constantsBufferPool = new TransientBufferAllocator(drv, cvConstantsBuffer.get().m_size << 10, cvConstantsBuffer.get().m_increment << 10, TransientBufferType::Constants);
            m_stagingBufferPool = new TransientBufferAllocator(drv, cvStatingBuffer.get().m_size << 10, cvStatingBuffer.get().m_increment << 10, TransientBufferType::Staging);
            m_geometryBufferPool = new TransientBufferAllocator(drv, cvGeometryBuffer.get().m_size << 10, cvGeometryBuffer.get().m_increment << 10, TransientBufferType::Geometry);
            m_storageBufferPool = new TransientBufferAllocator(drv, cvStorageBuffer.get().m_size << 10, cvStorageBuffer.get().m_increment << 10, TransientBufferType::Storage);
        }

        TransientAllocator::~TransientAllocator()
        {
            // free the pools
            delete m_geometryBufferPool;
            delete m_stagingBufferPool;
            delete m_storageBufferPool;
            delete m_constantsBufferPool;
        }

        TransientFrame* TransientAllocator::buildFrame(const TransientFrameBuilder &info)
        {
            PC_SCOPE_LVL2(BuildFrame);

            // create a frame object
            // TODO: pool if this hurts
            auto frame  = new TransientFrame;

            // create buffers, NOTE: allocation of size zero still works, just there's no actual buffer inside
            {
                PC_SCOPE_LVL2(Allocate);
                frame->m_constantsBuffer = m_constantsBufferPool->allocate(info.m_requiredConstantsBuffer);
                frame->m_stagingBuffer = m_stagingBufferPool->allocate(info.m_requiredStagingBuffer);
                frame->m_geometryBuffer = m_geometryBufferPool->allocate(info.m_requiredGeometryBuffer);
                frame->m_storageBuffer = m_storageBufferPool->allocate(info.m_requiredStorageBuffer);
            }

            // upload data to staging buffer
            {
                PC_SCOPE_LVL2(ProcessWrites);
                for (auto &write : info.m_writes)
                    frame->m_stagingBuffer->writeData(write.m_offset, write.m_size, write.m_data);
            }

            // finish writes
            {
                PC_SCOPE_LVL2(FlushWrites);
                frame->m_stagingBuffer->flushWrites();
            }

            // copy data to other buffers
            {
                PC_SCOPE_LVL2(CopyBuffers);
                for (auto &copy : info.m_copies)
                {
                    TransientBuffer *targetBuffer = nullptr;
                    switch (copy.m_targetType)
                    {
                        case TransientBufferType::Constants: targetBuffer = frame->m_constantsBuffer; break;
                        case TransientBufferType::Geometry: targetBuffer = frame->m_geometryBuffer; break;
                        case TransientBufferType::Storage: targetBuffer = frame->m_storageBuffer; break;
                    }

                    targetBuffer->copyDataFrom(frame->m_stagingBuffer, copy.m_sourceOffset, copy.m_targetOffset, copy.m_size);
                }

                // end copying
                frame->m_constantsBuffer->flushCopies();
            }

            // resolve views
            {
                PC_SCOPE_LVL2(ResolveViews);

                // copy mapping
                frame->m_resolvedTransientBuffers.reserve(info.m_mapping.size());
                //TRACE_INFO("Transients {}", info.m_mapping.size());
                for (auto &mapping : info.m_mapping)
                {
                    TransientFrame::BufferView view;
                    if (mapping.m_type == TransientBufferType::Geometry)
                        view.bufferPtr = frame->m_geometryBuffer;
                    else if (mapping.m_type == TransientBufferType::Storage)
                        view.bufferPtr = frame->m_storageBuffer;

                    // resolve the default untyped view
                    view.untypedView = view.bufferPtr->resolveUntypedView(mapping.m_offset, mapping.m_size);

                    // store in map
                    auto id = CalcTransientViewID(mapping.m_id, mapping.m_offset);
                    //TRACE_INFO("Transient {} at {} size {} type {}: {}", mapping.m_id, mapping.m_offset, mapping.m_size, (int)mapping.m_type, id);
                    frame->m_resolvedTransientBuffers[id] = view;
                }
            }

            return frame;
        }

        //--

        TransientFrameBuilder::TransientFrameBuilder()
            : m_requiredConstantsBuffer(0)
            , m_requiredStagingBuffer(0)
            , m_requiredGeometryBuffer(0)
            , m_requiredStorageBuffer(0)
            , m_constantsDataOffsetInStaging(0)
        {}

        uint32_t TransientFrameBuilder::allocStagingData(uint32_t size)
        {
            auto offset = base::Align<uint32_t>(m_requiredStagingBuffer, 256);
            m_requiredStagingBuffer = offset + size;
            return offset;
        }

        void TransientFrameBuilder::reportConstantsBlockSize(uint32_t size)
        {
            auto constsOffset = base::Align<uint32_t>(m_requiredConstantsBuffer, 256);
			m_requiredConstantsBuffer = constsOffset + size;
            m_constantsDataOffsetInStaging = allocStagingData(size);

            auto& copy = m_copies.emplaceBack();
            copy.m_size = size;
            copy.m_sourceOffset = m_constantsDataOffsetInStaging;
            copy.m_targetOffset = constsOffset;
            copy.m_targetType = TransientBufferType::Constants;
        }

        void TransientFrameBuilder::reportConstData(uint32_t offset, uint32_t size, const void* dataPtr, uint32_t& outOffsetInBigBuffer)
        {
            auto& write = m_writes.emplaceBack();
            write.m_size = size;
            write.m_offset = offset + m_constantsDataOffsetInStaging;
            write.m_data = dataPtr;

            outOffsetInBigBuffer = m_constantsDataOffsetInStaging + offset;
        }

        static TransientBufferType GetMainBufferType(const rendering::BufferView& view)
        {
            if (view.vertex() || view.index())
                return TransientBufferType::Geometry;
            return TransientBufferType::Storage;
        }

        void TransientFrameBuilder::reportBuffer(const rendering::TransientBufferView& buffer, const void* initalData, uint32_t initialUploadSize)
        {
            // get the main type of the temp buffer
            auto mainBufferType = GetMainBufferType(buffer);

            // allocate space in the storage
            uint32_t uploadOffset = 0;
            if (mainBufferType == TransientBufferType::Geometry)
            {
                uploadOffset = base::Align<uint32_t>(m_requiredGeometryBuffer, 256);
                m_requiredGeometryBuffer = uploadOffset + buffer.size();
            }
            else if (mainBufferType == TransientBufferType::Constants)
            {
                uploadOffset = base::Align<uint32_t>(m_requiredConstantsBuffer, 256);
                m_requiredConstantsBuffer = uploadOffset + buffer.size();
            }
            else if (mainBufferType == TransientBufferType::Storage)
            {
                uploadOffset = base::Align<uint32_t>(m_requiredStorageBuffer, 256);
                m_requiredStorageBuffer = uploadOffset + buffer.size();
            }

            // map the transient buffer to a place
            auto& mapping = m_mapping.emplaceBack();
            mapping.m_id = buffer.id();
            mapping.m_size = buffer.size();
            mapping.m_offset = uploadOffset;
            mapping.m_type = mainBufferType;
            ASSERT_EX(buffer.id().isTransient(), "Only transient IDs are allowed here");

            // if we need to initialize it with data prepare a proper upload place and schedule a copy operation
            if (initalData && 0 != initialUploadSize)
            {
                ASSERT(initialUploadSize <= buffer.size());

                // initialize place in the upload buffer
                auto stagingOffset = base::Align<uint32_t>(m_requiredStagingBuffer, 256);
                m_requiredStagingBuffer = stagingOffset + initialUploadSize;

                // write initial data to staging buffer
                auto &write = m_writes.emplaceBack();
                write.m_offset = stagingOffset;
                write.m_size = initialUploadSize;
                write.m_data = initalData;
                ASSERT((uint64_t)initalData >= 0x1000000);

                // copy data from staging buffer to final place
                auto& copy = m_copies.emplaceBack();
                copy.m_targetType = mainBufferType;
                copy.m_size = initialUploadSize;
                copy.m_sourceOffset = stagingOffset;
                copy.m_targetOffset = uploadOffset;
            }
        }

        void TransientFrameBuilder::reportBufferUpdate(const void* updateData, uint32_t updateSize, uint32_t& outStagingOffset)
        {
            // allocate space in the storage
            outStagingOffset = base::Align<uint32_t>(m_requiredStagingBuffer, 16);
            m_requiredStagingBuffer = outStagingOffset + updateSize;

            // write update data to storage
            auto &write = m_writes.emplaceBack();
            write.m_offset = outStagingOffset;
            write.m_size = updateSize;
            write.m_data = updateData;

            ASSERT((uint64_t)updateData >= 0x1000000);
        }

        //--

        TransientFrame::TransientFrame()
            : m_constantsBuffer(nullptr)
            , m_stagingBuffer(nullptr)
            , m_geometryBuffer(nullptr)
            , m_storageBuffer(nullptr)
        {}

        TransientFrame::~TransientFrame()
        {
            if (m_constantsBuffer)
            {
                m_constantsBuffer->returnToPool();
                m_constantsBuffer = nullptr;
            }

            if (m_stagingBuffer)
            {
                m_stagingBuffer->returnToPool();
                m_stagingBuffer = nullptr;
            }

            if (m_storageBuffer)
            {
                m_storageBuffer->returnToPool();
                m_storageBuffer= nullptr;
            }

            if (m_geometryBuffer)
            {
                m_geometryBuffer->returnToPool();
                m_geometryBuffer = nullptr;
            }
        }

        ResolvedBufferView TransientFrame::resolveConstants(const rendering::ConstantsView& constantsView) const
        {
            if (m_constantsBuffer)
            {
                if (!constantsView.empty())
                {
                    auto assignedRealOffset = *constantsView.offsetPtr() + constantsView.offset();
                    return m_constantsBuffer->resolveUntypedView(assignedRealOffset, constantsView.size());
                }
            }

            return ResolvedBufferView();
        }

        ResolvedBufferView TransientFrame::resolveStagingArea(uint32_t stagingOffset, uint32_t size) const
        {
            if (m_stagingBuffer)
                return m_stagingBuffer->resolveUntypedView(stagingOffset, size);

            return ResolvedBufferView();
        }

        ResolvedBufferView TransientFrame::resolveUntypedBufferView(const rendering::BufferView& view) const
        {
            if (view.id().isTransient())
            {
                auto index = (uint64_t) view.id().internalIndex();
                auto resolvedView  = m_resolvedTransientBuffers.find(index);
                if (resolvedView)
                    return resolvedView->untypedView;
            }

            return ResolvedBufferView();
        }

        ResolvedFormatedView TransientFrame::resolveTypedBufferView(const rendering::BufferView& view, const ImageFormat format) const
        {
            if (view.id().isTransient())
            {
                auto resolvedID = CalcTransientViewID(view.id(), view.offset());
                auto resolvedView  = m_resolvedTransientBuffers.find(resolvedID);
                if (resolvedView)
                    return resolvedView->bufferPtr->resolveTypedView(view.offset() + resolvedView->untypedView.offset, view.size(), format);
            }

            return ResolvedFormatedView();
        }

#if 0
        ResolvedBufferView TransientAllocator::resolveUntypedBufferView(const BufferView& view) const
        {
            auto bufferSpace  = m_aliveBuffers.find(view.id().value());
            if (bufferSpace)
                return ResolvedBufferView(bufferSpace->m_glBuffer, bufferSpace->offset + view.offset(), view.size());

            // unable to resolve view
            return ResolvedBufferView();
        }

        ResolvedBufferView TransientAllocator::resolveTypedBufferView(const BufferView& view) const
        {
            auto bufferSpace  = m_aliveBuffers.find(view.id().value());
            if (bufferSpace)
            {
                // translate format
                auto glSourceBuffer = bufferSpace->m_glBuffer;
                auto glFormat = TranslateImageFormat(view.format());

                // look for the matching view
                // TODO: optimize
                for (auto& typedView : m_typedBufferViews)
                {
                    if (typedView.m_glSourceBuffer == glSourceBuffer && typedView.m_glFormat == glFormat && typedView.offset == view.offset())
                    {
                        return ResolvedBufferView(typedView.m_glTypedBuffer, bufferSpace->offset + view.offset(), view.size());
                    }
                }

                // create typed view
                GLuint glTextureView = 0;
                GL_PROTECT(glCreateTextures(GL_TEXTURE_BUFFER, 1, &glTextureView));
                GL_PROTECT(glTextureBufferRange(glTextureView, glFormat, glSourceBuffer, bufferSpace->offset + view.offset(), view.size()));

                // store
                BufferTypedView viewInfo;
                viewInfo.m_glSourceBuffer = glSourceBuffer;
                viewInfo.m_glFormat = glFormat;
                viewInfo.m_glTypedBuffer = glTextureView;
                viewInfo.offset = view.offset();
                m_typedBufferViews.pushBack(viewInfo);

                // build typed view
                return ResolvedBufferView(glTextureView, 0, view.size());
            }

            // unable to resolve view
            return ResolvedBufferView();
        }
#endif

    } // vulkan
} // driver