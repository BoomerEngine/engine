/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#include "build.h"
#include "mesh.h"
#include "meshStreamIterator.h"

namespace base
{
    namespace mesh
    {
        //---

        MeshDataIteratorBase::MeshDataIteratorBase(MeshStreamType tag, uint32_t numElements, uint32_t dataStride, void* ptr)
            : m_tag(tag)
            , m_numElements(numElements)
            , m_stride(dataStride)
        {
            m_base = (uint8_t*)ptr;
            m_cur = m_base;
            m_end = m_base + m_numElements * m_stride;
        }

        MeshDataIteratorBase::MeshDataIteratorBase(const MeshModelChunkData& chunk)
            : m_tag(chunk.type)
            , m_stride(GetMeshStreamStride(chunk.type))
         {
            m_numElements = chunk.data.size() / GetMeshStreamStride(chunk.type);
            m_base = chunk.data.data();
            m_cur = m_base;
            m_end = m_base + m_numElements * m_stride;
        }

        //---
 
    } // mesh
} // base