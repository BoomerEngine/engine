/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

#include "array.h"
#include "arrayIterator.h"

BEGIN_BOOMER_NAMESPACE(base)

//---

/// inplace array, contains some internal buffer with elements
template < class T, uint32_t N >
class InplaceArray : public Array<T>
{
public:
    InplaceArray();
	InplaceArray(const Array<T>& other);
	InplaceArray(Array<T>&& other);
	InplaceArray(const T* ptr, uint32_t size); // makes a copy of the data
	InplaceArray(const InplaceArray<T, N>& other);
	InplaceArray(InplaceArray<T, N>&& other);

	InplaceArray& operator=(const Array<T>& other);
	InplaceArray& operator=(Array<T>&& other);
	InplaceArray& operator=(const InplaceArray<T, N>& other);
	InplaceArray& operator=(InplaceArray<T, N>&& other);

private:
    alignas(alignof(T)) uint8_t m_storage[N * sizeof(T)]; // internal storage buffer
};

//---

END_BOOMER_NAMESPACE(base)

#include "inplaceArray.inl"