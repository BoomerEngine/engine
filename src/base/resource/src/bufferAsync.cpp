/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: buffer #]
***/

#include "build.h"
#include "bufferAsync.h"

#include "base/object/include/streamOpcodeWriter.h"
#include "base/object/include/streamOpcodeReader.h"

namespace base
{
    namespace res
    {

        AsyncBuffer::AsyncBuffer()
        {
        }

        AsyncBuffer::~AsyncBuffer()
        {
        }

        AsyncBuffer::AsyncBuffer(const AsyncBuffer& other)
            : m_access(other.m_access)
        {}

        AsyncBuffer::AsyncBuffer(AsyncBuffer&& other)
            : m_access(std::move(other.m_access))
        {}

        AsyncBuffer& AsyncBuffer::operator=(const AsyncBuffer& other)
        {
            m_access = other.m_access;
            return *this;
        }

        AsyncBuffer& AsyncBuffer::operator=(AsyncBuffer&& other)
        {
            m_access = std::move(other.m_access);
            return *this;
        }

        bool AsyncBuffer::operator==(const AsyncBuffer& other) const
        {
            return m_access == other.m_access;
        }

        bool AsyncBuffer::operator!=(const AsyncBuffer& other) const
        {
            return m_access != other.m_access;
        }

        void AsyncBuffer::reset()
        {
            m_access.reset();
        }

        void AsyncBuffer::bind(const stream::DataBufferLatentLoaderPtr& source)
        {
            m_access = source;
        }

        void AsyncBuffer::bind(const void* data, uint32_t dataSize, bool compress /*= true*/)
        {
            if (data && dataSize)
            {
                CRC64 crc;
                crc.append(data, dataSize);

                //auto buffer = Buffer::Create(POOL_ASYNC_BUFFER, dataSize, 16, data);
                //m_access = RefNew<stream::PreloadedBufferLatentLoader>(buffer, crc.crc());
            }
            else
            {
                reset();
            }
        }

        Buffer AsyncBuffer::load(const PoolTag poolID /*= POOL_ASYNC_BUFFER*/) const
        {
            if (!m_access)
                return nullptr;

            return m_access->loadAsync();
        }

        //--

        namespace prv
        {

            void WriteAsyncBuffer(rtti::TypeSerializationContext& typeContext, stream::OpcodeWriter& stream, const void* data, const void* defaultData)
            {
                auto& asyncBuffer = *(AsyncBuffer*)data;

                /*// load the content of the buffer
                auto content = asyncBuffer.load();

                // buffer that are big cannot be saved
                if (content && content.size() > MAX_IO_SIZE)
                {
                    TRACE_WARNING("Trying to save buffer of size {} that is larger than the recommended IO size of {}", content.size(), MAX_IO_SIZE);
                }

                // if we are using mapped data for the buffers save the flag to indicate that
                // only mapped buffers are truly as
                uint8_t mapped = (stream.m_mapper != nullptr) ? 1 : 0;
                stream.writeValue(mapped);

                // the method of saving depends on mapping
                if (mapped)
                {
                    // in the mapped mode the async buffer is saved as index
                    stream::MappedBufferIndex index = 0;
                    stream.m_mapper->mapBuffer(content, index);
                    stream.writeValue(index);
                }
                else
                {
                    // save the size of the content
                    uint32_t size = range_cast<uint32_t>(content ? content.size() : 0);
                    stream.writeValue(size);

                    // save the CRC of the data
                    uint64_t crc = asyncBuffer.crc();
                    stream.writeValue(crc);

                    // save the data directly
                    if (size > 0)
                        stream.write(content.data(), size);
                }

                return true;*/
            }

            void ReadAsyncBuffer(rtti::TypeSerializationContext& typeContext, stream::OpcodeReader& stream, void* data)
            {
                auto& asyncBuffer = *(AsyncBuffer*)data;
                /*
                // get the flag determining how was the data stored
                uint8_t mapped = 0;
                stream.readValue(mapped);

                // if the data was mapped but we don't have the mapper we will fail
                if (mapped && !stream.m_unmapper)
                {
                    TRACE_ERROR("Trying to load mapped buffer without the unampper, the data will not be loaded");
                    return false;
                }

                // load the data
                if (mapped)
                {
                    // load the buffer index
                    stream::MappedBufferIndex index = 0;
                    stream.readValue(index);

                    // allow the unmapper to provide the buffer access object
                    stream::DataBufferLatentLoaderPtr accessPtr;
                    stream.m_unmapper->unmapBuffer(index, accessPtr);
                    asyncBuffer.bind(accessPtr);
                }
                else
                {
                    // load buffer size
                    uint32_t size = 0;
                    stream.readValue(size);

                    // load buffer crc
                    uint64_t crc = 0;
                    stream.readValue(crc);

                    // create local buffer
                    if (size > 0)
                    {
                        // load data
                        auto buffer = Buffer::Create(POOL_ASYNC_BUFFER, size, 16);
                        stream.read((void*)buffer.data(), size);

                        // validate CRC
#ifdef BUILD_DEBUG
                        auto actualBufferCRC = CRC64().append(buffer.data(), buffer.size()).crc();
                        if (actualBufferCRC != crc)
                        {
                            TRACE_ERROR("Corrupted data buffer content, CRC mismatch");
                            return false;
                        }
#endif

                        asyncBuffer.bind(base::RefNew<stream::PreloadedBufferLatentLoader>(buffer, crc));
                    }
                }

                // loaded
                return true;*/
            }

        } // prv

        //--

        RTTI_BEGIN_CUSTOM_TYPE(AsyncBuffer);
            RTTI_BIND_NATIVE_CTOR_DTOR(AsyncBuffer);
            RTTI_BIND_NATIVE_COMPARE(AsyncBuffer);
            RTTI_BIND_NATIVE_COPY(AsyncBuffer);
            RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteAsyncBuffer, &prv::ReadAsyncBuffer);
        RTTI_END_TYPE();


    } // res
} // base
