/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: instancing #]
***/

#include "build.h"
#include "instanceBufferLayout.h"
#include "instanceBuffer.h"

#include "base/containers/include/stringBuilder.h"

namespace base
{

    InstanceBufferLayout::InstanceBufferLayout()
        : m_size(0)
        , m_alignment(0)
        , m_complexVarsList(nullptr)
    {
    }

    InstanceBufferLayout::~InstanceBufferLayout()
    {
    }

    void InstanceBufferLayout::initializeBuffer(void* bufferMemory) const
    {
        memzero(bufferMemory, m_size);

        auto var  = m_complexVarsList;
        while (var != nullptr)
        {
            auto varData  = base::OffsetPtr(bufferMemory, var->m_offset);
            var->m_type->construct(varData);
            var = var->m_nextComplexType;
        }
    }

    void InstanceBufferLayout::destroyBuffer(void* bufferMemory) const
    {
        auto var  = m_complexVarsList;
        while (var != nullptr)
        {
            auto varData  = base::OffsetPtr(bufferMemory, var->m_offset);
            var->m_type->destruct(varData);
            var = var->m_nextComplexType;
        }
    }

    void InstanceBufferLayout::copyBufer(void* destBufferMemory, const void* srcBufferMemory) const
    {
        memcpy(destBufferMemory, srcBufferMemory, m_size);

        auto var  = m_complexVarsList;
        while (var != nullptr)
        {
            auto varData  = base::OffsetPtr(destBufferMemory, var->m_offset);
            var->m_type->construct(varData);

            auto srcVarData  = base::OffsetPtr(srcBufferMemory, var->m_offset);
            var->m_type->copy(varData, srcVarData);
            var = var->m_nextComplexType;
        }
    }

    InstanceBufferPtr InstanceBufferLayout::createInstance(PoolTag poolID)
    {
        void* ptr = mem::AllocateBlock(poolID, size(), alignment(), "InstanceBuffer");
        initializeBuffer(ptr);
        return RefNew<InstanceBuffer>(AddRef(this), ptr, size(), poolID);
    }

    InstanceBufferPtr CreateBufferInstance(const InstanceBufferLayoutPtr& bufferLayout, const PoolTag poolID)
    {
        if (!bufferLayout)
            return InstanceBufferPtr();
        return bufferLayout->createInstance(poolID);
    }

} // base
