/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streams #]
***/

#pragma once

#include "renderingMeshStreamData.h"

BEGIN_BOOMER_NAMESPACE()
        
///-----

struct MeshRawChunkData;

/// more abstract (typeless) stream iterator for mesh data
class ENGINE_MESH_API MeshDataIteratorBase
{
public:
    MeshDataIteratorBase(MeshStreamType tag, uint32_t numElements, uint32_t dataStride, void* ptr);
    MeshDataIteratorBase(const MeshRawChunkData& chunk);

    INLINE MeshDataIteratorBase(const MeshDataIteratorBase& other) = default;
    INLINE MeshDataIteratorBase& operator=(const MeshDataIteratorBase& other) = default;
    INLINE MeshDataIteratorBase(MeshDataIteratorBase&& other) = default;
    INLINE MeshDataIteratorBase& operator=(MeshDataIteratorBase&& other) = default;
    INLINE ~MeshDataIteratorBase() = default;

    INLINE MeshStreamType tag() const
    {
        return m_tag;
    }

    INLINE void rewind()
    {
        m_cur = m_base;
    }

    INLINE bool empty() const
    {
        return m_numElements == 0;
    }

    INLINE uint64_t size() const
    {
        return m_numElements;
    }

    INLINE operator bool() const
    {
        return m_cur < m_end;
    }

    INLINE void operator++()
    {
        m_cur += m_stride;
    }

    INLINE void operator++(int)
    {
        m_cur += m_stride;
    }

    INLINE const void* ptr() const
    {
        return m_cur;
    }

    INLINE void* ptr()
    {
        return m_cur;
    }

    INLINE const void* base() const
    {
        return m_base;
    }

    INLINE void* base()
    {
        return m_base;
    }

    INLINE void seek(uint64_t index)
    {
        m_cur = m_base + (index * m_stride);
    }

    INLINE uint32_t stride() const
    {
        return m_stride;
    }

protected:
    uint8_t* m_base;
    uint8_t* m_cur;
    uint8_t* m_end;

    uint32_t m_stride;
    uint64_t m_numElements;
    MeshStreamType m_tag; // informative
};


///-----

/// stream data view and iterator (kind of mixed class)
template< typename T >
class MeshDataIterator : public MeshDataIteratorBase
{
public:
    INLINE MeshDataIterator(MeshStreamType tag, uint32_t numElements, uint32_t dataStride, void* ptr)
        : MeshDataIteratorBase(tag, numElements, dataStride, ptr)
    {
        DEBUG_CHECK_EX(sizeof(T) <= m_stride, "Iterator type does not match stream type");
    }

    INLINE MeshDataIterator(const MeshRawChunkData& chunk)
        : MeshDataIteratorBase(chunk)
    {
        DEBUG_CHECK_EX(sizeof(T) <= m_stride, "Iterator type does not match stream type");
    }

    INLINE const T& operator[](uint64_t index) const
    {
        return *(const T*)(m_base + index*m_stride);
    }

    INLINE T& operator[](uint64_t index)
    {
        return *(T*)(m_base + index*m_stride);
    }

    INLINE const T& operator*() const
    {
        return *(const T*)m_cur;
    }

    INLINE T& operator*()
    {
        return *(T*)m_cur;
    }
};

///-----

/// stream with one specific data for all elements
template< typename T >
class MeshDataIteratorSpecificDefaultValue : public MeshDataIteratorBase
{
public:
    INLINE MeshDataIteratorSpecificDefaultValue(MeshStreamType tag, uint32_t numElements, const T& defaultValue)
        : MeshDataIteratorBase(tag, numElements, 0, &m_default)
        , m_default(defaultValue)
    {
        DEBUG_CHECK_EX(sizeof(T) <= m_stride, "Iterator type does not match stream type");
    }

private:
    T m_default;
};

///-----

/// stream with default value for a given stream type, usually zeros but one for colors....
class ENGINE_MESH_API MeshDataIteratorDefaultValue : public MeshDataIteratorBase
{
public:
    MeshDataIteratorDefaultValue(MeshStreamType tag, uint32_t numElements);

private:
    uint8_t m_defaultData[32];
};

///-----

template< MeshStreamType ST >
struct MeshStreamTypeDataResolver
{};

template<> struct MeshStreamTypeDataResolver< MeshStreamType::Position_3F > { typedef Vector3 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::Normal_3F > { typedef Vector3 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::Tangent_3F > { typedef Vector3 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::Binormal_3F > { typedef Vector3 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::TexCoord0_2F > { typedef Vector2 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::TexCoord1_2F > { typedef Vector2 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::TexCoord2_2F > { typedef Vector2 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::TexCoord3_2F > { typedef Vector2 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::Color0_4U8 > { typedef Color TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::Color1_4U8 > { typedef Color TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::Color2_4U8 > { typedef Color TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::Color3_4U8 > { typedef Color TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::SkinningIndices_4U8 > { typedef Color TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::SkinningWeights_4F > { typedef Vector4 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::SkinningIndicesEx_4U8 > { typedef Color TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::SkinningWeightsEx_4F > { typedef Vector4 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::General0_F4 > { typedef Vector4 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::General1_F4 > { typedef Vector4 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::General2_F4 > { typedef Vector4 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::General3_F4 > { typedef Vector4 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::General4_F4 > { typedef Vector4 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::General5_F4 > { typedef Vector4 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::General6_F4 > { typedef Vector4 TYPE; };
template<> struct MeshStreamTypeDataResolver< MeshStreamType::General7_F4 > { typedef Vector4 TYPE; };

///----

END_BOOMER_NAMESPACE()
