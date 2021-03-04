/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#pragma once

#include "fileHandle.h"
#include "asyncFileHandle.h"

BEGIN_BOOMER_NAMESPACE()

//--

// an simple wrapper for file handle interface that allows read from memory block
class CORE_IO_API MemoryReaderFileHandle : public IReadFileHandle
{
public:
    MemoryReaderFileHandle(const void* memory, uint64_t size);
    MemoryReaderFileHandle(const Buffer& buffer);
    virtual ~MemoryReaderFileHandle();

    //----

    /// IFileHandler
    virtual uint64_t size() const override final;
    virtual uint64_t pos() const override final;
    virtual bool pos(uint64_t newPosition) override final;
    virtual uint64_t readSync(void* data, uint64_t size) override final;

protected:
    uint64_t m_pos = 0;
    uint64_t m_size = 0;

    const uint8_t* m_data = nullptr;
    Buffer m_buffer;
};

//--

// an simple wrapper for async file handle interface that allows read from memory block
class CORE_IO_API MemoryAsyncReaderFileHandle : public IAsyncFileHandle
{
public:
    MemoryAsyncReaderFileHandle(const void* memory, uint64_t size);
    MemoryAsyncReaderFileHandle(const Buffer& buffer);
    virtual ~MemoryAsyncReaderFileHandle();

    //----

    /// IFileHandler
    virtual uint64_t size() const override final;
    virtual CAN_YIELD uint64_t readAsync(uint64_t offset, uint64_t size, void* readBuffer) override final;

protected:
    uint64_t m_size = 0;

    const uint8_t* m_data = nullptr;
    Buffer m_buffer;
};

//--

// an simple wrapper for file handle interface that allows read from memory block
class CORE_IO_API MemoryWriterFileHandle : public IWriteFileHandle
{
public:
    MemoryWriterFileHandle(uint64_t initialSize = 1U << 20);
    virtual ~MemoryWriterFileHandle();

    //----

    /// was the data discarded ?
    INLINE bool discarded() const { return m_discarded; }

    ///---

    /// get data as system memory buffer
    Buffer extract();

    //----

    /// IFileHandler
    virtual uint64_t size() const override final;
    virtual uint64_t pos() const override final;
    virtual bool pos(uint64_t newPosition) override final;
    virtual uint64_t writeSync(const void* data, uint64_t size) override final;
    virtual void discardContent() override final;

protected:
    uint8_t* m_base = nullptr;
    uint8_t* m_pos = nullptr;
    uint8_t* m_end = nullptr;
    uint8_t* m_max = nullptr;

    bool m_discarded = false;

    void freeBuffer();
    bool increaseCapacity(uint64_t minSize);
}; 

END_BOOMER_NAMESPACE()
