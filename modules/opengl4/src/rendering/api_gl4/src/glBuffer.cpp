/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\objects\buffers #]
***/

#include "build.h"
#include "glBuffer.h"
#include "glDriver.h"
#include "glUtils.h"

#include "base/memory/include/buffer.h"

namespace rendering
{
    namespace gl4
    {
        //--

        base::mem::PoolID DetermineBestMemoryPool(const BufferCreationInfo& setup)
        {
            if (setup.allowVertex)
                return POOL_GL_VERTEX_BUFFER;
            if (setup.allowIndex)
                return POOL_GL_INDEX_BUFFER;
            if (setup.allowCostantReads)
                return POOL_GL_CONSTANT_BUFFER;
            if (setup.allowIndirect)
                return POOL_GL_INDIRECT_BUFFER;
            return POOL_GL_STORAGE_BUFFER;
        }

        //--

        Buffer::Buffer(Driver* drv, const BufferCreationInfo &setup, const SourceData* initialData)
            : Object(drv, ObjectType::Buffer)
            , m_glBuffer(0)
            , m_size(setup.size)
        {
            // setup initialization data
            if (initialData)
                m_initData = *initialData;
            
            // update stats
            m_poolId = DetermineBestMemoryPool(setup);
            base::mem::PoolStats::GetInstance().notifyAllocation(m_poolId, m_size);
        }

        Buffer::~Buffer()
        {
            // unregister from pool
            auto driver  = this->driver();
            if (driver != nullptr)
            {
                // release views
                for (auto typedView : m_baseTypedViews.values())
                    GL_PROTECT(glDeleteTextures(1, &typedView));
                m_baseTypedViews.clear();

                // release views
                for (auto typedView : m_offsetTypedViews.values())
                    GL_PROTECT(glDeleteTextures(1, &typedView));
                m_offsetTypedViews.clear();

                // release the buffer object
                GL_PROTECT(glDeleteBuffers(1, &m_glBuffer));
                m_glBuffer = 0;

                // update stats
                base::mem::PoolStats::GetInstance().notifyAllocation(m_poolId, m_size);
            }
        }

        bool Buffer::CheckClassType(ObjectType type)
        {
            return (type == ObjectType::Buffer);
        }

        Buffer *Buffer::CreateBuffer(Driver* drv, const BufferCreationInfo &setup, const SourceData* initializationData)
        {
            return MemNew(Buffer, drv, setup, initializationData);
        }

        //--

        void Buffer::finalizeCreation()
        {
            PC_SCOPE_LVL1(BufferUpload);

            ASSERT(m_glBuffer == 0);
            GL_PROTECT(glEnable(GL_DEBUG_OUTPUT));
            // create buffer
            GLuint buffer = 0;
            GL_PROTECT(glCreateBuffers(1, &buffer));
            m_glBuffer = buffer;

            // label the object
            if (m_label)
                GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, m_label.length(), m_label.c_str()));

            // setup buffer
            if (m_initData.data)
            {
                auto sourceData = m_initData.data.data() + m_initData.offset;
                GL_PROTECT(glNamedBufferData(buffer, m_size, sourceData, GL_STATIC_DRAW));
                m_initData = SourceData();
            }
            else
            {
                // determine buffer usage flags
                GLuint usageFlags = 0;

                // setup data with buffer
                GL_PROTECT(glNamedBufferStorage(buffer, m_size, nullptr, usageFlags));
            }
        }

        ResolvedBufferView Buffer::resolveUntypedView(const BufferView& view)
        {
            // initialize the buffer on first use
            if (m_glBuffer == 0)
                finalizeCreation();

            // create buffer view
            return ResolvedBufferView(m_glBuffer, view.offset(), view.size());
        }

        ResolvedFormatedView Buffer::resolveTypedView(const BufferView& view, const ImageFormat dataFormat)
        {
            // initialize the buffer on first use
            if (m_glBuffer == 0)
                finalizeCreation();

            // we should have a typed view of the buffer
            DEBUG_CHECK_EX(dataFormat != ImageFormat::UNKNOWN, "Trying to resolve a typed view without specifying a type");
            if (dataFormat == ImageFormat::UNKNOWN)
                return ResolvedFormatedView();

            // translate format
            auto imageFormat = TranslateImageFormat(dataFormat);

            // if we have 0 offset our job is a tid bit simpler
            if (view.offset() == 0)
            {
                // use existing view
                GLuint glTextureView = 0;
                if (m_baseTypedViews.find((uint16_t)dataFormat, glTextureView))
                    return ResolvedFormatedView(glTextureView, imageFormat);

                // create a view
                GL_PROTECT(glCreateTextures(GL_TEXTURE_BUFFER, 1, &glTextureView));
                GL_PROTECT(glTextureBufferRange(glTextureView, imageFormat, m_glBuffer, 0, m_size));
                m_baseTypedViews[(uint16_t)dataFormat] = glTextureView;

                return ResolvedFormatedView(glTextureView, imageFormat);
            }
            else
            {
                GLuint glTextureView = 0;
                BufferTypedViewKey key = { dataFormat, view.offset() };
                if (m_offsetTypedViews.find(key, glTextureView))
                    return ResolvedFormatedView(glTextureView, imageFormat);

                // create a view
                GL_PROTECT(glCreateTextures(GL_TEXTURE_BUFFER, 1, &glTextureView));
                GL_PROTECT(glTextureBufferRange(glTextureView, imageFormat, m_glBuffer, view.offset(), m_size - view.offset()));
                m_offsetTypedViews[key] = glTextureView;

                return ResolvedFormatedView(glTextureView, imageFormat);
            }
        }

        //--

    } // gl4
} // device
