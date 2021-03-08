/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: buffer #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

// loader of async buffer content
// usually stores file reference/metadata
class CORE_OBJECT_API IAsyncFileBufferLoader : public IReferencable
{
    RTTI_DECLARE_POOL(POOL_ASYNC_BUFFER);

public:
    virtual ~IAsyncFileBufferLoader();

    // size of the uncompressed data
    virtual uint64_t size() const = 0;

    // content CRC of the uncompressed data, used as a key
    virtual uint64_t crc() const = 0;

    // is the data in the memory? 
    virtual bool resident() const = 0;

    // extract buffer raw content (may be compressed) without all the decompression, used mostly when saving resource back
    // NOTE: may still need to load the buffer
    virtual CAN_YIELD bool extract(Buffer& outCompressedData, CompressionType& outCompressionType) const = 0;

    // load content of the buffer (not cached, each call will start new loading)
    // NOTE: buffer MAY be compressed and decompression will be preformed
    // NOTE: we MAY have to wait for free IO memory before loading starts (to limit the memory spikes)
    // NOTE: call on a fiber for best results
    virtual CAN_YIELD Buffer load(PoolTag tag = POOL_ASYNC_BUFFER, uint32_t alignment=16) const = 0;

    // load part of the data
    virtual CAN_YIELD bool loadInto(uint64_t offset, void* targetMemory, uint64_t targetSize) const = 0;

    //--

    // create resident buffer, will compress the data
    static AsyncFileBufferLoaderPtr CreateResidentBufferFromUncompressedData(Buffer uncompressedData, CompressionType ct = CompressionType::Uncompressed, uint64_t knownCRC=0);

    // create resident buffer from already compressed data
    static AsyncFileBufferLoaderPtr CreateResidentBufferFromAlreadyCompressedData(Buffer compressedData, uint64_t uncompressedSize, uint64_t uncompressedCRC, CompressionType ct);
};

//--

// asynchronous buffer, can be loaded via async IO any time (even when the resources is unloaded)
// each buffer is identified in a file by the CRC of the data and there's a table of buffers in the header of the file
// NOTE: text files do not support async buffers and the content will be always loaded to memory
// NOTE: when an async buffer is modified the content will be retained in memory until the file is save with a context that has m_flushAsyncFileBuffers
class CORE_OBJECT_API AsyncFileBuffer
{
public:
    AsyncFileBuffer();
    AsyncFileBuffer(const AsyncFileBuffer& other);
    AsyncFileBuffer(AsyncFileBuffer&& other);
    explicit AsyncFileBuffer(IAsyncFileBufferLoader* loader);
    ~AsyncFileBuffer();

    AsyncFileBuffer& operator=(const AsyncFileBuffer& other);
    AsyncFileBuffer& operator=(AsyncFileBuffer&& other);

    bool operator==(const AsyncFileBuffer& other) const;
    bool operator!=(const AsyncFileBuffer& other) const;

    //--

    // get size of the data in this buffer
    INLINE uint64_t size() const { return m_loader ? m_loader->size() : 0; }

    // get the CRC of the data stored in the buffer (without loading it)
    // NOTE: the CRC can act as a hash in some other data structures
    INLINE uint64_t crc() const { return m_loader ? m_loader->crc() : 0; }

    // check if the buffer is resident in the memory (and does not have to be loaded)
    INLINE bool resident() const { return m_loader ? m_loader->resident() : true; } // empty buffers are resident

    // returns true if buffer is empty (does not contain any data)
    INLINE bool empty() const { return !m_loader; }

    // check if buffer is valid
    INLINE operator bool() const { return m_loader; }

    // get internal loader, sometimes it's better to use it 
    INLINE AsyncFileBufferLoaderPtr loader() const { return m_loader; }

    //--

    // clear data in the buffer
    void reset();

    // set new data for the data buffer, if data is not indicated as compressed it will be compressed
    // NOTE: if data is not compressed on bind it will not be compressed automatically (we assume the buffer creator knows best)
    // NOTE: data MAY be recompressed only during the final cooking phase
    // NOTE: the source buffer MUST NOT be modified after binding
    void bind(Buffer uncompressedData, CompressionType compression = CompressionType::Uncompressed, PoolTag tag = POOL_ASYNC_BUFFER);

    //--

    // load content of the async buffer from whatever is the source of the data into a in-memory buffer, buffer may be decompressed internally if it's compressed
    // each call will open a new access request so you need to limit them at the higher level
    // NOTE: deleting the file OR modifying the file content while the async request is in flight has UNDEFINED behavior but the DepotService has some internal locks to prevent most common issues
    // NOTE: if the buffer is already loaded the existing content will be returned ASAP
    CAN_YIELD Buffer load(PoolTag tag = POOL_ASYNC_BUFFER, uint32_t alignment = 16) const;

    // load content (or part of it) into provided memory buffer, best for textures/meshes on UMA architectures
    // NOTE: if buffer is uncompressed the content will be copied
    CAN_YIELD bool loadInto(uint64_t offset, void* targetMemory, uint64_t targetSize) const;

    //---

private:
    AsyncFileBufferLoaderPtr m_loader;
};

//--

END_BOOMER_NAMESPACE()
