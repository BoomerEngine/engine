/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\descriptor #]
***/

#pragma once

#include "renderingDescriptorID.h"

#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

    ///--

/// info about parameter descriptor
class GPU_DEVICE_API DescriptorInfo
{
public:
    DescriptorInfo();
    DescriptorInfo(const DescriptorInfo& other) = default;
    DescriptorInfo(const DescriptorInfo& other, DescriptorID id);
    DescriptorInfo& operator=(const DescriptorInfo& other) = default;

    // is the layout empty ? should not be the case for valid layouts
    INLINE bool empty() const { return m_entries.empty(); }

    // get number of entries in the layout
    INLINE uint32_t size() const { return m_entries.size(); }

    // get n-th entry
    INLINE const DeviceObjectViewType operator[](uint32_t index) const { return m_entries[index]; }

    // get the unique layout hash
    INLINE uint64_t hash() const { return m_hash; }

    // get the ID we are registered under
    INLINE DescriptorID id() const { return m_id; }

    // compare layouts for being equal
    bool operator==(const DescriptorInfo& other) const;

    // get a string representation of the layout, usually something like CCIIB, etc describing the descriptor layout
    void print(IFormatStream& f) const;

private:
    DescriptorID m_id;
    uint64_t m_hash = 0;

    InplaceArray<DeviceObjectViewType, 32> m_entries;

    //---

    friend class DescriptorID;
};

//--

END_BOOMER_NAMESPACE_EX(gpu)
