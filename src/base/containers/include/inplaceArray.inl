/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

template < class T, uint32_t N >
INLINE InplaceArray<T,N>::InplaceArray()
    : Array<T>(BaseArrayBuffer(m_storage, N, false))
{}

template < class T, uint32_t N >
INLINE InplaceArray<T, N>::InplaceArray(const Array<T>& other)
	: Array<T>(BaseArrayBuffer(m_storage, N, false))
{
	auto data  = allocateUninitialized(other.size());
	std::uninitialized_copy_n(other.typedData(), other.size(), data);
}

template < class T, uint32_t N >
INLINE InplaceArray<T, N>::InplaceArray(Array<T>&& other)
	: Array<T>(BaseArrayBuffer(m_storage, N, false))
{
	*this = std::move(other);
}

template < class T, uint32_t N >
INLINE InplaceArray<T, N>::InplaceArray(const InplaceArray<T,N>& other)
	: Array<T>(BaseArrayBuffer(m_storage, N, false))
{
	auto data  = allocateUninitialized(other.size());
	std::uninitialized_copy_n(other.typedData(), other.size(), data);
}

template < class T, uint32_t N >
INLINE InplaceArray<T, N>::InplaceArray(InplaceArray<T, N>&& other)
	: Array<T>(BaseArrayBuffer(m_storage, N, false))
{
	*this = std::move(other);
}

template < class T, uint32_t N >
INLINE InplaceArray<T, N>::InplaceArray(const T* ptr, uint32_t size)
	: Array<T>(BaseArrayBuffer(m_storage, N, false))
{
	auto data  = allocateUninitialized(size);
	std::uninitialized_copy_n(ptr, size, data);
}

//--

template < class T, uint32_t N >
INLINE InplaceArray<T, N>& InplaceArray<T, N>::operator=(const Array<T>& other)
{
	Array<T>::operator=(other);
	return *this;
}

template < class T, uint32_t N >
INLINE InplaceArray<T, N>& InplaceArray<T, N>::operator=(Array<T>&& other)
{
	Array<T>::operator=(other);
	return *this;
}

template < class T, uint32_t N >
INLINE InplaceArray<T, N>& InplaceArray<T, N>::operator=(const InplaceArray<T, N>& other)
{
	Array<T>::operator=(other);
	return *this;		
}

template < class T, uint32_t N >
INLINE InplaceArray<T, N>& InplaceArray<T, N>::operator=(InplaceArray<T, N>&& other)
{
	Array<T>::operator=(other);
	return *this;
}

END_BOOMER_NAMESPACE(base)
