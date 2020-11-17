/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\transient #]
***/

#pragma once

#include "glTransientBufferAllocator.h"
#include "glImage.h"
#include "glBuffer.h"

#include "rendering/driver/include/renderingImageView.h"
#include "rendering/driver/include/renderingConstantsView.h"
#include "rendering/driver/include/renderingBufferView.h"
#include "base/containers/include/inplaceArray.h"

namespace rendering
{
    namespace gl4
    {

        class Image;
        class TransientAllocator;
        class TransientFrame;
        class TransientFrameBuilder;

        ///--

        /// a frame with all allocated transient objects
        class TransientFrame : public base::NoCopy, public base::mem::GlobalPoolObject<POOL_API_OBJECTS>
        {
        public:
            ~TransientFrame();

            //---

            // resolve a view of the constant buffer
            ResolvedBufferView resolveConstants(const rendering::ConstantsView& constantsView) const;

            /// resolve a view to the buffer
            /// NOTE: buffer must have been previously reported to the builder
            ResolvedBufferView resolveUntypedBufferView(const rendering::BufferView& view) const;

            /// resolve a typed view to the buffer
            /// NOTE: buffer must have been previously reported to the builder
            ResolvedFormatedView resolveTypedBufferView(const rendering::BufferView& view, const ImageFormat format) const;

            /// resolve the staging area
            ResolvedBufferView resolveStagingArea(uint32_t stagingOffset, uint32_t size) const;

        private:
            TransientFrame();

            //--

            // the transient buffers
            TransientBuffer* m_constantsBuffer;
            TransientBuffer* m_stagingBuffer; // we hold on to it until we are done
            TransientBuffer* m_geometryBuffer;
            TransientBuffer* m_storageBuffer;

            struct BufferView
            {
                ResolvedBufferView untypedView;
                TransientBuffer* bufferPtr = nullptr;
            };

            // placed transient buffers
            typedef base::HashMap<uint64_t, BufferView> TBufferMap;
            TBufferMap m_resolvedTransientBuffers;

            friend class TransientAllocator;
            friend class TransientFrameBuilder;
        };

        //---

        ///---

        /// builder of the transient frame
        class TransientFrameBuilder : public base::NoCopy
        {
        public:
            TransientFrameBuilder();

            /// report a constants block of data
            void reportConstantsBlockSize(uint32_t size);

            /// write data
            void reportConstData(uint32_t offset, uint32_t size, const void* dataPtr, uint32_t& outOffsetInBigBuffer);

            /// report a buffer, with or without data
            void reportBuffer(const rendering::TransientBufferView& buffer, const void* initalData, uint32_t initialUploadSize);

            /// report a buffer update, returns the source offset in the staging buffer
            void reportBufferUpdate(const void* updateData, uint32_t updateSize, uint32_t& outStagingOffset);

            //--

        private:
            uint32_t m_requiredConstantsBuffer;
            uint32_t m_requiredStagingBuffer;
            uint32_t m_requiredGeometryBuffer;
            uint32_t m_requiredStorageBuffer;

            uint32_t m_constantsDataOffsetInStaging;

            struct Write
            {
                uint32_t m_offset;
                uint32_t m_size;
                const void* m_data;
            };

            struct Copy
            {
                TransientBufferType m_targetType;
                uint32_t m_sourceOffset;
                uint32_t m_targetOffset;
                uint32_t m_size;
            };

            struct Mapping
            {
                ObjectID m_id;
                TransientBufferType m_type;
                uint32_t m_offset;
                uint32_t m_size;
            };

            base::InplaceArray<Write, 1024> m_writes;
            base::InplaceArray<Copy, 1024> m_copies;
            base::InplaceArray<Mapping, 1024> m_mapping;

            uint32_t allocStagingData(uint32_t size);

            friend class TransientAllocator;
        };

        //---

        /// allocator of transient objects
        /// can free all related transient objects once the frame was submitted
        class TransientAllocator : public base::NoCopy, public base::mem::GlobalPoolObject<POOL_API_RUNTIME>
        {
        public:
            TransientAllocator(Driver* drv);
            ~TransientAllocator(); // frees all allocated objects

            // allocate a frame
            TransientFrame* buildFrame(const TransientFrameBuilder& info);

        private:
            Driver* m_driver;

            // pools for different types of buffers
            TransientBufferAllocator* m_stagingBufferPool;
            TransientBufferAllocator* m_geometryBufferPool;
            TransientBufferAllocator* m_storageBufferPool;
            TransientBufferAllocator* m_constantsBufferPool;

            // VBO layouts

        };

    } // gl4
} // driver

