/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "core/containers/include/blockPool.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//---

/// a buffer that holds entries of constant size that can be updated frequently
/// NOTE: this buffer has a backing CPU storage
/// NOTE: this buffer does not grow
class GPU_DEVICE_API ManagedBuffer : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_GPU_RUNTIME)

public:
    ManagedBuffer(const BufferCreationInfo& info);
    virtual ~ManagedBuffer();

    //---

    /// buffer object
    INLINE const BufferObjectPtr& bufferObject() { return m_bufferObject; }

    // backing CPU data buffer, read only
    template< typename T >
    INLINE const T* typedData() const { return (const T*)m_backingStorage; }

    //---

    /// prepare buffer for use, write update commands
    /// NOTE: we may send more than one update if the data is far apart
    void update(CommandWriter& cmd);

    //---

    /// update data in the buffer
    void writeData(uint32_t offset, uint32_t size, const void* data);

    /// write at index (for structured buffers)
    template< typename T >
    INLINE void writeAtIndex(uint32_t index, const T& data) { writeData(index * sizeof(T), sizeof(T), &data); }

    //---

private:
    BufferObjectPtr m_bufferObject;

    uint8_t* m_backingStorage = nullptr;
    uint8_t* m_backingStorageEnd = nullptr;

    uint32_t m_dirtyRegionStart = 0;
    uint32_t m_dirtyRegionEnd = 0;

    uint32_t m_structureGranularity = 0;

    SpinLock m_stateLock;
};

//--

END_BOOMER_NAMESPACE_EX(gpu)

