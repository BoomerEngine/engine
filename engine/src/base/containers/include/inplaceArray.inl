/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers\dynamic #]
***/

#pragma once

namespace base
{

    template < class T, uint32_t N >
    INLINE InplaceArray<T,N>::InplaceArray()
        : Array<T>((T*) &m_storage[0], N, InplaceArrayData)
    {}

	template < class T, uint32_t N >
	INLINE InplaceArray<T, N>::InplaceArray(const Array<T>& other)
		: Array<T>((T*)& m_storage[0], N, InplaceArrayData)
	{
		auto data  = allocateUninitialized(other.size());
		std::uninitialized_copy_n(other.typedData(), other.size(), data);
	}

	template < class T, uint32_t N >
	INLINE InplaceArray<T, N>::InplaceArray(Array<T>&& other)
		: Array<T>((T*)& m_storage[0], N, InplaceArrayData)
	{
		*this = std::move(other);
	}

	template < class T, uint32_t N >
	INLINE InplaceArray<T, N>::InplaceArray(const InplaceArray<T,N>& other)
		: Array<T>((T*)& m_storage[0], N, InplaceArrayData)
	{
		auto data  = allocateUninitialized(other.size());
		std::uninitialized_copy_n(other.typedData(), other.size(), data);
	}

	template < class T, uint32_t N >
	INLINE InplaceArray<T, N>::InplaceArray(InplaceArray<T, N>&& other)
		: Array<T>((T*)& m_storage[0], N, InplaceArrayData)
	{
		*this = std::move(other);
	}

	template < class T, uint32_t N >
	INLINE InplaceArray<T, N>::InplaceArray(const T* ptr, uint32_t size)
		: Array<T>((T*)& m_storage[0], N, InplaceArrayData)
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

} // base
