/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: buffer #]
***/

#pragma once

namespace base
{
    namespace res
    {

        // asynchronous buffer, can be loaded via async IO any time (even when the resources is unloaded)
        // each buffer is identified in a file by the CRC of the data and there's a table of buffers in the header of the file
        // NOTE: text files do not support async buffers and the content will be always loaded to memory
        // NOTE: when an async buffer is modified the content will be retained in memory until the file is save with a context that has m_flushAsyncBuffers
        class BASE_RESOURCES_API AsyncBuffer
        {
        public:
            AsyncBuffer();
            AsyncBuffer(const AsyncBuffer& other);
            AsyncBuffer(AsyncBuffer&& other);
            ~AsyncBuffer();

            AsyncBuffer& operator=(const AsyncBuffer& other);
            AsyncBuffer& operator=(AsyncBuffer&& other);

            bool operator==(const AsyncBuffer& other) const;
            bool operator!=(const AsyncBuffer& other) const;

            //--

            // clear buffer content, releases the access proxy
            INLINE void clear() { m_access.reset(); }

            // get size of the data in this buffer
            INLINE uint32_t size() const { return m_access ? m_access->size() : 0; }

            // get the CRC of the data stored in the buffer (without loading it)
            // NOTE: the CRC can act as a hash in some other data structures
            INLINE uint64_t crc() const { return m_access ? m_access->crc() : 0; }

            // check if the buffer is resident in the memory (and does not have to be loaded)
            INLINE bool resident() const { return m_access ? m_access->resident() : true; } // emtpy buffers are resident}

            // returns true if buffer is empty (does not contain any data)
            INLINE bool empty() const { return !m_access; }

            // set new data for the data buffer
            // the interface is a little shitty as it requires to pass all the data at once
            // NOTE: data size is limited to 4GB (large buffers are very very rare but small buffers are very often, 64-bit size is overkill)
            // TODO: paged buffer interface
            void reset(const void* data, uint32_t dataSize);

            // bind to async data source
            void bind(const stream::DataBufferLatentLoaderPtr& source);

            // load content of the async buffer from whatever is the source of the data into a in-memory buffer
            // each call will open a new access request so you need to limit them at the higher level
            // NOTE: all pending saves are flushed before the load beings, also, while there's an active async loading no save can begin
            // NOTE: deleting the file OR modifying the file content while the async request is in flight has UNDEFINED behavior
            // NOTE: if the buffer is already loaded the existing content will be returned ASAP
            CAN_YIELD Buffer load(mem::PoolID poolID = POOL_ASYNC_BUFFER) const;

            //---

        private:
            static const uint16_t DEFAULT_ALIGNMENT = 16;

            // load content of the buffer, synchronously, very slow
            Buffer readBufferSync() const;

            // an way to load the content of the buffer
            stream::DataBufferLatentLoaderPtr m_access;
        };

    } // res
} // base
