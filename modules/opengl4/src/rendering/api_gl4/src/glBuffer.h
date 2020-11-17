/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\objects\buffers #]
***/

#pragma once

#include "glObject.h"
#include "rendering/driver/include/renderingResources.h"

namespace rendering
{
    namespace gl4
    {
        //---

        /// a key for a typed view resolver
        struct BufferTypedViewKey
        {
            ImageFormat dataFormat = ImageFormat::UNKNOWN;
            uint32_t dataOffset = 0;

            INLINE static uint32_t CalcHash(const BufferTypedViewKey& key)
            {
                return base::CRC32() << (uint16_t)key.dataFormat << key.dataOffset;
            }

            INLINE bool operator==(const BufferTypedViewKey& other) const
            {
                return (dataFormat == other.dataFormat) && (dataOffset == other.dataOffset);
            }
        };

        ///---

        /// resolved buffer view information (not provided via buffer view)
        struct ResolvedBufferView
        {
            GLuint glBuffer = 0;
            uint32_t offset = 0;
            uint32_t size = 0;

            INLINE ResolvedBufferView() {};
            INLINE ResolvedBufferView(GLuint glBuffer_, uint32_t offset_, uint32_t size_)
                : glBuffer(glBuffer_)
                , offset(offset_)
                , size(size_)
            {}

            INLINE bool operator==(const ResolvedBufferView& other) const { return (glBuffer == other.glBuffer) && (offset == other.offset) && (size == other.size); }
            INLINE bool operator!=(const ResolvedBufferView& other) const { return !operator==(other); }

            INLINE bool empty() const { return (glBuffer == 0); }
            INLINE operator bool() const { return (glBuffer != 0); }
        };

        ///---

        /// resolved typed buffer view information (not provided via buffer view)
        struct ResolvedFormatedView
        {
            GLuint glBufferView = 0;
            GLuint glViewFormat = 0;

            INLINE ResolvedFormatedView() {};
            INLINE ResolvedFormatedView(GLuint glBufferView_, GLuint glFormat_)
                : glBufferView(glBufferView_)
                , glViewFormat(glFormat_)
            {}

            INLINE bool operator==(const ResolvedFormatedView& other) const { return (glBufferView == other.glBufferView) && (glViewFormat == other.glViewFormat); }
            INLINE bool operator!=(const ResolvedFormatedView& other) const { return !operator==(other); }

            INLINE bool empty() const { return (glBufferView == 0); }
            INLINE operator bool() const { return (glBufferView != 0); }
        };

        ///---

        /// wrapper for persistent buffers
        class Buffer : public Object
        {
        public:
            Buffer(Driver* drv, const BufferCreationInfo& setup, const SourceData* initialData);
            virtual ~Buffer();

            //--

            // get size of used memory
            INLINE uint32_t dataSize() const { return m_size; }

            // get the label
            INLINE const base::StringBuf& label() const { return m_label; }

            //--

            static bool CheckClassType(ObjectType type);

            //---

            // resolve untyped buffer view
            ResolvedBufferView resolveUntypedView(const BufferView& view);

            // resolve typed buffer view
            ResolvedFormatedView resolveTypedView(const BufferView& view, ImageFormat dataFormat);

            //---

            // create a vertex buffer (always in the device memory), can be updated only via the staging buffer and transfer queue
            static Buffer* CreateBuffer(Driver* drv, const BufferCreationInfo& setup, const SourceData* initializationData);

        private:
            GLuint m_glBuffer;

            uint32_t m_size;
            base::StringBuf m_label;

            base::HashMap<uint16_t, GLuint> m_baseTypedViews;
            base::HashMap<BufferTypedViewKey, GLuint> m_offsetTypedViews; // typed views with offsets

            SourceData m_initData;

            PoolTag m_poolId;

            //--

            void finalizeCreation();
        };

    } // gl4
} // driver
