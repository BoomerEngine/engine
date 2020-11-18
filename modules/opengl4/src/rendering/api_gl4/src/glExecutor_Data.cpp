/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\frame\execution #]
***/

#include "build.h"
#include "glDevice.h"
#include "glTempBuffer.h"
#include "glExecutor.h"
#include "glFrame.h"

namespace rendering
{
    namespace gl4
    {
        namespace exec
        {

            void FrameExecutor::prepareFrameData(const RuntimeDataAllocations& info)
            {
                PC_SCOPE_LVL2(PrepareFrameData);

                // create buffers, NOTE: allocation of size zero still works, just there's no actual buffer inside
                {
                    PC_SCOPE_LVL2(Allocate);

                    if (info.m_requiredConstantsBuffer)
                    {
                        auto buffer = m_device.costantsBufferPool().allocate(info.m_requiredConstantsBuffer);

                        m_frame.registerCompletionCallback([buffer]() { buffer->returnToPool(); });
                        m_tempConstantBuffer = buffer;

                    }

                    if (info.m_requiredStagingBuffer)
                    {
                        auto buffer = m_device.stagingBufferPool().allocate(info.m_requiredStagingBuffer);
                        m_tempStagingBuffer = buffer;

                        m_frame.registerCompletionCallback([buffer]() { buffer->returnToPool(); });
                    }
                }

                // upload data to staging buffer
                {
                    PC_SCOPE_LVL2(ProcessWrites);
                    for (auto& write : info.m_writes)
                        m_tempStagingBuffer->writeData(write.offset, write.size, write.data);
                }

                // finish writes
                if (m_tempStagingBuffer)
                {
                    PC_SCOPE_LVL2(FlushWrites);
                    m_tempStagingBuffer->flushWrites();
                }

                // copy data to other buffers
                if (m_tempConstantBuffer)
                {
                    PC_SCOPE_LVL2(CopyBuffers);
                    for (auto& copy : info.m_copies)
                    {
                        switch (copy.targetType)
                        {
                            case TempBufferType::Constants: 
                                m_tempConstantBuffer->copyDataFrom(m_tempStagingBuffer, copy.sourceOffset, copy.targetOffset, copy.size);
                                break;
                        }
                    }

                    // end copying
                    m_tempConstantBuffer->flushCopies();
                }

                // resolve views
                /*{
                    PC_SCOPE_LVL2(ResolveViews);

                    // copy mapping
                    frame->m_resolvedTransientBuffers.reserve(info.m_mapping.size());
                    //TRACE_INFO("Transients {}", info.m_mapping.size());
                    for (auto& mapping : info.m_mapping)
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
                }*/
            }

            //--

            RuntimeDataAllocations::RuntimeDataAllocations()
            {}

            uint32_t RuntimeDataAllocations::allocStagingData(uint32_t size)
            {
                auto offset = base::Align<uint32_t>(m_requiredStagingBuffer, 256);
                m_requiredStagingBuffer = offset + size;
                return offset;
            }

            void RuntimeDataAllocations::reportConstantsBlockSize(uint32_t size)
            {
                auto constsOffset = base::Align<uint32_t>(m_requiredConstantsBuffer, 256);
                m_requiredConstantsBuffer = constsOffset + size;
                m_constantsDataOffsetInStaging = allocStagingData(size);

                auto& copy = m_copies.emplaceBack();
                copy.size = size;
                copy.sourceOffset = m_constantsDataOffsetInStaging;
                copy.targetOffset = constsOffset;
                copy.targetType = TempBufferType::Constants;
            }

            void RuntimeDataAllocations::reportConstData(uint32_t offset, uint32_t size, const void* dataPtr, uint32_t& outOffsetInBigBuffer)
            {
                auto& write = m_writes.emplaceBack();
                write.size = size;
                write.offset = offset + m_constantsDataOffsetInStaging;
                write.data = dataPtr;

                outOffsetInBigBuffer = m_constantsDataOffsetInStaging + offset;
            }

            /*void RuntimeDataAllocations::reportBuffer(const rendering::TransientBufferView& buffer, const void* initalData, uint32_t initialUploadSize)
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
                    auto& write = m_writes.emplaceBack();
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
            }*/

            void RuntimeDataAllocations::reportBufferUpdate(const void* updateData, uint32_t updateSize, uint32_t& outStagingOffset)
            {
                // allocate space in the storage
                outStagingOffset = base::Align<uint32_t>(m_requiredStagingBuffer, 16);
                m_requiredStagingBuffer = outStagingOffset + updateSize;

                // write update data to storage
                auto& write = m_writes.emplaceBack();
                write.offset = outStagingOffset;
                write.size = updateSize;
                write.data = updateData;

                ASSERT((uint64_t)updateData >= 0x1000000);
            }

            //--

        } // exec
    } // gl4
} // rendering