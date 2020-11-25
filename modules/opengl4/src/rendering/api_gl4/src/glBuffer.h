/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\buffers #]
***/

#pragma once

#include "glObject.h"
#include "rendering/device/include/renderingResources.h"

namespace rendering
{
    namespace gl4
    {
        ///---

        class BufferTypedView;
        class BufferUntypedView;

        ///---



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
            Buffer(Device* drv, const BufferCreationInfo& setup);
            virtual ~Buffer();

            static const auto STATIC_TYPE = ObjectType::Buffer;

            //--

            // get size of used memory
            INLINE uint32_t dataSize() const { return m_size; }

            // get the label
            INLINE const base::StringBuf& label() const { return m_label; }

            //---

            // directly resolve untyped buffer view of the whole buffer
            ResolvedBufferView resolveUntypedView(uint32_t offset = 0, uint32_t size = INDEX_MAX);

            //---

			// create a constant (uniform) view of the buffer
			BufferUntypedView* createConstantView_ClientAPI(uint32_t offset, uint32_t size) const;

            // create typed view object
            // NOTE: function might be called from outside render thread
            BufferTypedView* createTypedView_ClientAPI(ImageFormat format, uint32_t offset, uint32_t size, bool writable) const;

            // create untyped view object
            // NOTE: function might be called from outside render thread
            BufferUntypedView* createUntypedView_ClientAPI(uint32_t offset, uint32_t size, bool writable) const;

            //---

			// copy staging data into this buffer
			void copyFromBuffer(const ResolvedBufferView& view, const ResourceCopyRange& range);

        private:
            GLuint m_glBuffer = 0;
			GLuint m_glUsage = 0;

            uint32_t m_size = 0;
            uint32_t m_stride = 0;

			bool m_shaderReadable = false;
			bool m_uavWritable = false;
			bool m_constantReadable = false;

            PoolTag m_poolTag;
            base::StringBuf m_label;

            //--

            void finalizeCreation();

            friend class BufferTypedView;
            friend class BufferUntypedView;
        };

        //---

		// typed (formated) view of the buffer
        class BufferTypedView : public Object
        {
        public:
            BufferTypedView(Device* drv, Buffer* buffer, ImageFormat format, uint32_t offset, uint32_t size, bool writable);
            virtual ~BufferTypedView();

            static const auto STATIC_TYPE = ObjectType::BufferTypedView;

            ResolvedFormatedView resolve();

			INLINE bool writable() const { return m_writable; }

        private:
            Buffer* m_buffer = nullptr;

            uint32_t m_offset = 0;
            uint32_t m_size = 0;

            GLuint m_glViewFormat = 0;
            GLuint m_glTextureView = 0;

			bool m_writable = false;

            //--

            void finalizeCreation();
        };

        //---

        class BufferUntypedView : public Object
        {
        public:
            BufferUntypedView(Device* drv, Buffer* buffer, uint32_t offset, uint32_t size, uint32_t stride, bool writable);
            virtual ~BufferUntypedView();

            static const auto STATIC_TYPE = ObjectType::BufferUntypedView;

            ResolvedBufferView resolve();

			INLINE uint32_t stride() const { return m_stride; }
			INLINE bool writable() const { return m_writable; }

        private:
            Buffer* m_buffer = nullptr;

            uint32_t m_offset = 0;
            uint32_t m_size = 0;
            uint32_t m_stride = 0;

			bool m_writable = false;
        };


        //---

    } // gl4
} // rendering
