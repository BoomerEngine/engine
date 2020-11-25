/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\buffers #]
***/

#include "build.h"
#include "glBuffer.h"
#include "glDevice.h"
#include "glUtils.h"

#include "base/memory/include/buffer.h"
#include "glDeviceThreadCopy.h"

namespace rendering
{
    namespace gl4
    {
        //--



		static GLuint DetermineBestUsageFlag(const BufferCreationInfo& info)
		{
			GLuint flags = 0;

			if (info.allowDynamicUpdate)
				flags |= GL_DYNAMIC_STORAGE_BIT;
			
			return flags;
		}

        //--

        Buffer::Buffer(Device* drv, const BufferCreationInfo &setup)
            : Object(drv, ObjectType::Buffer)
            , m_size(setup.size)
			, m_stride(setup.stride)
        {
			// determine read/write capabilities
			m_shaderReadable = setup.allowShaderReads;
			m_uavWritable = setup.allowUAV;
			m_constantReadable = setup.allowCostantReads;

			// determine resource usage
			m_glUsage = DetermineBestUsageFlag(setup);
        }

        Buffer::~Buffer()
        {
            // release the buffer object
            if (m_glBuffer)
            {
                GL_PROTECT(glDeleteBuffers(1, &m_glBuffer));
                m_glBuffer = 0;
            }

            // update stats
            base::mem::PoolStats::GetInstance().notifyAllocation(m_poolTag, m_size);
        }

        //--		

        void Buffer::finalizeCreation()
        {
            ASSERT_EX(m_glBuffer == 0, "Buffer already created");
            PC_SCOPE_LVL1(BufferCreate);

            // create buffer
            GLuint buffer = 0;
            GL_PROTECT(glCreateBuffers(1, &buffer));
            m_glBuffer = buffer;

            // label the object
            if (m_label)
                GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, m_label.length(), m_label.c_str()));

            // setup data with buffer
            GL_PROTECT(glNamedBufferStorage(buffer, m_size, nullptr, m_glUsage));
        }

		void Buffer::copyFromBuffer(const ResolvedBufferView& view, const ResourceCopyRange& range)
		{
			PC_SCOPE_LVL1(BufferAsyncCopy);

			if (m_glBuffer == 0)
				finalizeCreation();

			const auto copySize = std::min<uint32_t>(range.buffer.size, view.size);
			GL_PROTECT(glCopyNamedBufferSubData(view.glBuffer, m_glBuffer, view.offset, range.buffer.offset, copySize));
		}

		//--

        ResolvedBufferView Buffer::resolveUntypedView(uint32_t offset /*=0*/, uint32_t size /*= INDEX_MAX*/)
        {
            if (m_glBuffer == 0)
            {
                finalizeCreation();
                DEBUG_CHECK_RETURN_V(m_glBuffer != 0, ResolvedBufferView());
            }

            return ResolvedBufferView(m_glBuffer, offset, size, m_stride);
        }

		BufferUntypedView* Buffer::createConstantView_ClientAPI(uint32_t offset, uint32_t size) const
		{
			DEBUG_CHECK_RETURN_V(m_stride == 0, nullptr);

			DEBUG_CHECK_RETURN_V((offset & 15) == 0, nullptr);
			DEBUG_CHECK_RETURN_V((size & 15) == 0, nullptr);

			DEBUG_CHECK_RETURN_V(offset < m_size, nullptr);
			DEBUG_CHECK_RETURN_V(size <= m_size, nullptr);
			DEBUG_CHECK_RETURN_V(offset + size <= m_size, nullptr);

			DEBUG_CHECK_RETURN_V(m_constantReadable, nullptr);


			return new BufferUntypedView(device(), const_cast<Buffer*>(this), offset, size, 0, false);
		}

        BufferTypedView* Buffer::createTypedView_ClientAPI(ImageFormat format, uint32_t offset, uint32_t size, bool writable) const
        {
            DEBUG_CHECK_RETURN_V(offset < m_size, nullptr);
            DEBUG_CHECK_RETURN_V(size <= m_size, nullptr);
            DEBUG_CHECK_RETURN_V(offset + size <= m_size, nullptr);
            DEBUG_CHECK_RETURN_V(format != ImageFormat::UNKNOWN, nullptr);

			// TODO: check format compatibility

			DEBUG_CHECK_RETURN_V(!GetImageFormatInfo(format).compressed, nullptr);
			const auto formatSize = GetImageFormatInfo(format).bitsPerPixel / 8;
			DEBUG_CHECK_RETURN_V((offset % formatSize) == 0, nullptr);
			DEBUG_CHECK_RETURN_V((size % formatSize) == 0, nullptr);

			if (writable)
			{
				DEBUG_CHECK_RETURN_V(m_uavWritable, nullptr);
			}
			else
			{
				DEBUG_CHECK_RETURN_V(m_shaderReadable, nullptr);
			}

            return new BufferTypedView(device(), const_cast<Buffer*>(this), format, offset, size, writable);
        }

        BufferUntypedView* Buffer::createUntypedView_ClientAPI(uint32_t offset, uint32_t size, bool writable) const
        {
			DEBUG_CHECK_RETURN_V(m_stride != 0, nullptr);

			DEBUG_CHECK_RETURN_V((offset % m_stride == 0), nullptr);
			DEBUG_CHECK_RETURN_V((size % m_stride == 0), nullptr);

            DEBUG_CHECK_RETURN_V(offset < m_size, nullptr);
            DEBUG_CHECK_RETURN_V(size <= m_size, nullptr);
            DEBUG_CHECK_RETURN_V(offset + size <= m_size, nullptr);

			if (writable)
			{
				DEBUG_CHECK_RETURN_V(m_uavWritable, nullptr);
			}
			else
			{
				DEBUG_CHECK_RETURN_V(m_shaderReadable, nullptr);
			}

            return new BufferUntypedView(device(), const_cast<Buffer*>(this), offset, size, m_stride, writable);
        }

        //--

        BufferTypedView::BufferTypedView(Device* drv, Buffer* buffer, ImageFormat dataFormat, uint32_t offset, uint32_t size, bool writable)
            : Object(drv, ObjectType::BufferTypedView)
            , m_buffer(buffer)
            , m_offset(offset)
            , m_size(size)
			, m_writable(writable)
        {
            DEBUG_CHECK_EX(dataFormat != ImageFormat::UNKNOWN, "Trying to resolve a typed view without specifying a type");
            m_glViewFormat = TranslateImageFormat(dataFormat);
        }

        BufferTypedView::~BufferTypedView()
        {
            if (m_glTextureView)
            {
                GL_PROTECT(glDeleteTextures(1, &m_glTextureView));
                m_glTextureView = 0;
            }
        }

        ResolvedFormatedView BufferTypedView::resolve()
        {
            if (m_glTextureView == 0)
                finalizeCreation();

            return ResolvedFormatedView(m_glTextureView, m_glViewFormat);
        }

        void BufferTypedView::finalizeCreation()
        {
            PC_SCOPE_LVL1(BufferTypedView);

            DEBUG_CHECK_RETURN(m_glTextureView == 0);
            DEBUG_CHECK_RETURN(m_glViewFormat != 0);

            // make sure buffer itself is created
            if (0 == m_buffer->m_glBuffer)
            {
                m_buffer->finalizeCreation();
                DEBUG_CHECK_RETURN(m_buffer->m_glBuffer != 0);
            }

            // we should have a typed view of the buffer
            GL_PROTECT(glCreateTextures(GL_TEXTURE_BUFFER, 1, &m_glTextureView));
            GL_PROTECT(glTextureBufferRange(m_glTextureView, m_glViewFormat, m_buffer->m_glBuffer, m_offset, m_size));
            TRACE_SPAM("GL: Created typed buffer view {} of {}, offset {}, size {}", m_glTextureView, m_buffer->m_glBuffer, m_offset, m_size);
        }

        //--

        BufferUntypedView::BufferUntypedView(Device* drv, Buffer* buffer, uint32_t offset, uint32_t size, uint32_t stride, bool writable)
            : Object(drv, ObjectType::BufferUntypedView)
            , m_buffer(buffer)
            , m_offset(offset)
            , m_size(size)
            , m_stride(stride)
			, m_writable(writable)
        {}

        BufferUntypedView::~BufferUntypedView()
        {}

        ResolvedBufferView BufferUntypedView::resolve()
        {
			return m_buffer->resolveUntypedView(m_offset, m_size);
        }

        //--

    } // gl4
} // rendering
