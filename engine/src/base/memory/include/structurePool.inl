/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\pools #]
***/

#pragma once

namespace base
{
    namespace mem
    {

		//---

		/// a simple pool for structures
		template<typename T>
		INLINE StructurePool<T>::StructurePool(PoolTag pool, uint32_t elementsPerPage)
			: StructurePoolBase(pool, sizeof(T), alignof(T), elementsPerPage)
		{}

		template<typename T>
		template<typename... Args >
		INLINE T* StructurePool<T>::create(Args&& ... args)
		{
			void* mem = StructurePoolBase::alloc();
			return new (mem) T(std::forward< Args >(args)...);
		}

		template<typename T>
		INLINE void StructurePool<T>::free(T* ptr)
		{
			((T*)ptr)->~T();
			StructurePoolBase::free(ptr);
		}

		//---

	} // mem
} // base