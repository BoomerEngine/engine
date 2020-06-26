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

    template<typename T>
    INLINE Array<T>::Array()
    {};

    template<typename T>
    INLINE Array<T>::Array(const T* ptr, uint32_t size)
    {
		auto elems  = allocateUninitialized(size);
		std::uninitialized_copy_n(ptr, size, elems);
    }

	template<typename T>
	INLINE Array<T>::Array(void* ptr, uint32_t maxCapcity, EInplaceArrayData)
		: BaseArray(ptr, maxCapcity)
	{}

    template<typename T>
    Array<T>::Array(const Array<T> &other)
    {
		pushBack(other.typedData(), other.size());
    }

    template<typename T>
    INLINE Array<T>::Array(Array<T> &&other)
    {
		if (other.isLocal())
		{
            suckFrom(other);
		}
		else
		{
			reserve(other.size());

			auto elems  = allocateUninitialized(other.size());
			std::uninitialized_move_n(other.typedData(), other.size(), elems);

			other.reset();
		}
    }

    template<typename T>
    INLINE Array<T>::~Array()
    {
        clear();
    }

    template<typename T>
    INLINE uint32_t Array<T>::size() const
    {
        return BaseArray::size();
    }

    template<typename T>
    INLINE int Array<T>::lastValidIndex() const
    {
        return (int)BaseArray::size() - 1;
    }

    template<typename T>
    INLINE uint32_t Array<T>::capacity() const
    {
        return BaseArray::capacity();
    }

    template<typename T>
    INLINE void* Array<T>::data()
    {
        return BaseArray::data();
    }

    template<typename T>
    INLINE const void* Array<T>::data() const
    {
        return BaseArray::data();
    }

    template<typename T>
    INLINE uint32_t Array<T>::dataSize() const
    {
        return size() * sizeof(T);
    }

    template<typename T>
    INLINE const T* Array<T>::typedData() const
    {
        return (const T*)data();
    }

    template<typename T>
    INLINE T* Array<T>::typedData()
    {
        return (T*)data();
    }

    template<typename T>
    INLINE bool Array<T>::empty() const
    {
        return BaseArray::empty();
    }

    template<typename T>
    INLINE bool Array<T>::full() const
    {
        return BaseArray::full();
    }

    template<typename T>
    INLINE T& Array<T>::operator[](uint32_t index)
    {
        checkIndex(index);
        return typedData()[index];
    }

    template<typename T>
    INLINE const T& Array<T>::operator[](uint32_t index) const
    {
        checkIndex(index);
        return typedData()[index];
    }

    template<typename T>
    INLINE void Array<T>::popBack()
    {
        checkIndex(size() - 1);
        changeSize(size() - 1);
        std::destroy_n(typedData() + size(), 1);
    }

    template<typename T>
    INLINE T& Array<T>::back()
    {
        checkIndex(size() - 1);
        return typedData()[size() - 1];
    }

    template<typename T>
    INLINE const T& Array<T>::back() const
    {
        checkIndex(size() - 1);
        return typedData()[size() - 1];
    }

    template<typename T>
    INLINE T& Array<T>::front()
    {
        checkIndex(0);
        return typedData()[0];
    }

    template<typename T>
    INLINE const T& Array<T>::front() const
    {
        checkIndex(0);
        return typedData()[0];
    }

    template<typename T>
    INLINE void Array<T>::shrink()
    {
		// do not shrink non-local arrays
		if (isLocal())
			BaseArray::changeCapacity(POOL_CONTAINERS, size(), sizeof(T), __alignof(T), typeid(typename std::remove_cv<T>::type).name());
    }

    template<typename T>
    void Array<T>::resizeWith(uint32_t newSize, const T& elementTemplate)
    {
        if (newSize < size())
        {
            std::destroy(begin() + newSize, end());
        }
        else if (newSize > size())
        {
            reserve(newSize);

            std::uninitialized_fill_n(begin() + size(), newSize - size(), elementTemplate);
        }

        BaseArray::changeSize(newSize);
    }

    template<typename T>
    void Array<T>::resize(uint32_t newSize)
    {
        if (newSize < size())
        {
            std::destroy(begin() + newSize, end());
        }
        else if (newSize > size())
        {
            reserve(newSize);

            std::uninitialized_default_construct_n(begin() + size(), newSize - size());
        }

        BaseArray::changeSize(newSize);
    }

    template<typename T>
    void Array<T>::prepare(uint32_t minimalSize)
    {
        if (minimalSize > size())
        {
            reserve(minimalSize);
			std::uninitialized_default_construct_n(begin() + size(), minimalSize - size());
			BaseArray::changeSize(minimalSize);
        }
    }

    template<typename T>
    void Array<T>::prepareWith(uint32_t minimalSize, const T& elementTemplate /*= T()*/)
    {
        if (minimalSize > size())
        {
            reserve(minimalSize);
            std::uninitialized_fill_n(begin() + size(), minimalSize - size(), elementTemplate);
			BaseArray::changeSize(minimalSize);
		}
    }

    template<typename T>
    void Array<T>::reserve(uint32_t newSize)
    {
        if (newSize > capacity())
        {
            auto newCapacity  = BaseArray::CalcNextCapacity(capacity(), sizeof(T));
            while (newCapacity < newSize)
                newCapacity = BaseArray::CalcNextCapacity(newCapacity, sizeof(T));

			if (isLocal())
                BaseArray::changeCapacity(POOL_CONTAINERS, newCapacity, sizeof(T), __alignof(T), "");// , typeid(typename std::remove_cv<T>::type).name());
			else
                BaseArray::makeLocal(POOL_CONTAINERS, newCapacity, sizeof(T), __alignof(T), "");// , typeid(typename std::remove_cv<T>::type).name());
        }
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::contains(const FK& element) const
    {
        auto it  = std::find(begin(), end(), element);
        return (it != end());
    }

    template<typename T>
    template<typename FK>
    int Array<T>::find(const FK& element) const
    {
        auto it  = std::find(begin(), end(), element);
        return (it != end()) ? std::distance(begin(), it) : INDEX_NONE;
    }

    template<typename T>
    template<typename FK>
    int Array<T>::findIf(const FK& element) const
    {
        auto it = std::find_if(begin(), end(), element);
        return (it != end()) ? std::distance(begin(), it) : INDEX_NONE;
    }

    template<typename T>
    template<typename FK>
    const T& Array<T>::findIfOrDefault(const FK& func, const T& defaultValue /*= T()*/) const
    {
        auto it = std::find_if(begin(), end(), element);
        if (it != end())
            return *it;
        return defaultValue;
    }

    template<typename T>
    void Array<T>::erase(uint32_t index, uint32_t count)
    {
        checkIndexRange(index, count);

		std::destroy_n(begin() + index, count);
		memmove(typedData() + index, typedData() + index + count, (size() - (index + count)) * sizeof(T));
        
        BaseArray::changeSize(size() - count);
    }

    template<typename T>
    void Array<T>::eraseUnordered(uint32_t index, uint32_t count)
    {
        checkIndexRange(index, count);

		std::destroy_n(begin() + index, count);
		memmove(typedData() + index, typedData() + size() - count, count * sizeof(T));

        BaseArray::changeSize(size() - count);
    }

    template<typename T>
    void Array<T>::clear()
    {
        std::destroy(begin(), end());
        BaseArray::changeSize(0);

		if (isLocal())
			BaseArray::changeCapacity(POOL_CONTAINERS, 0, sizeof(T), __alignof(T), typeid(typename std::remove_cv<T>::type).name());
    }

    template<typename T>
    void Array<T>::clearPtr()
    {
        std::for_each(begin(), end(), [](T& val) { MemDelete(val); });
        std::destroy(begin(), end());
        BaseArray::changeSize(0);

		if (isLocal())
			BaseArray::changeCapacity(POOL_CONTAINERS, 0, sizeof(T), __alignof(T), typeid(typename std::remove_cv<T>::type).name());
    }

    template<typename T>
    void Array<T>::reset()
    {
        std::destroy(begin(), end());
        BaseArray::changeSize(0);
    }

    template<typename T>
    Array<T>& Array<T>::operator=(const Array<T> &other)
    {
        if (this != &other)
        {
			reset();
			std::uninitialized_copy_n(other.begin(), other.size(), allocateUninitialized(other.size()));
        }

        return *this;
    }

    template<typename T>
    INLINE Array<T>& Array<T>::operator=(Array<T> &&other)
    {
        if (this != &other)
        {
            auto old  = std::move(*this);

			if (other.isLocal())
			{
                suckFrom(other);
			}
			else
			{
				reserve(other.size());

				auto elems  = allocateUninitialized(other.size());
				std::uninitialized_move_n(other.typedData(), other.size(), typedData());

				other.reset();
			}
        }

        return *this;
    }

    template<typename T>
    bool Array<T>::operator==(const Array<T> &other) const
    {
        return std::equal(begin(), end(), other.begin(), other.end());
    }

    template<typename T>
    bool Array<T>::operator!=(const Array<T> &other) const
    {
        return !operator==(other);
    }

    template<typename T>
    INLINE void Array<T>::pushBack(const T &item)
    {
        new (allocateUninitialized(1)) T(item);
    }

    template<typename T>
    void Array<T>::pushBack(const T* items, uint32_t count)
    {
        std::uninitialized_copy_n(items, count, allocateUninitialized(count));
    }

    template<typename T>
    INLINE void Array<T>::pushBack(T &&item)
    {
        new (allocateUninitialized(1)) T(std::move(item));
    }

    template<typename T>
    template<typename ...Args>
    INLINE T& Array<T>::emplaceBack(Args&&... args)
    {
        return *new (allocateUninitialized(1)) T(std::forward<Args>(args)...);
    }

    template<typename T>
    INLINE void Array<T>::pushBackUnique(const T &element)
    {
        if (!contains(element))
            pushBack(element);
    }

    template<typename T>
    void Array<T>::insert(uint32_t index, const T &item)
    {
        insert(index, &item, 1);
    }

    template<typename T>
    void Array<T>::insertWith(uint32_t index, uint32_t count, const T& itemTemplate)
    {
        if (count == 0)
            return;

        auto prevSize  = size();
        allocateUninitialized(count);

        // just move the memory, item's MUST be movable like that
        if (index < prevSize)
        {
            auto moveCount  = prevSize - index;
            memmove(typedData() + index + count, typedData() + index, moveCount * sizeof(T));

#ifndef PLATFORM_RELEASE
            memset(typedData() + index, 0xCD, count * sizeof(T));
#endif
        }

        // initialize new items as if the memory was trash
        for (uint32_t i=0; i<count; ++i)
            std::uninitialized_copy_n(&itemTemplate, 1, typedData() + index + i);
    }

    template<typename T>
    void Array<T>::insert(uint32_t index, const T* items, uint32_t count)
    {
		if (count == 0)
			return;

		auto prevSize  = size();
		allocateUninitialized(count);

		// just move the memory, item's MUST be movable like that
		if (index < prevSize)
		{
			auto moveCount  = prevSize - index;
			memmove(typedData() + index + count, typedData() + index, moveCount * sizeof(T));

#ifndef PLATFORM_RELEASE
			memset(typedData() + index, 0xCD, count * sizeof(T));
#endif
		}

		// initialize new items as if the memory was trash
		std::uninitialized_copy_n(items, count, typedData() + index);
    }

    template<typename T>
    template< typename FK >
    uint32_t Array<T>::remove(const FK &item)
    {
        auto it  = std::remove(begin(), end(), item);
        if (it == end())
            return 0;

		auto count  = std::distance(it, end());

        std::destroy(it, end());
        BaseArray::changeSize(std::distance(begin(), it));
        return count;
    }

    template<typename T>
    template< typename FK >
    uint32_t Array<T>::removeUnordered(const FK &item)
    {
        // TODO: more optimal
        return remove(item);
    }

    template<typename T>
    INLINE ArrayIterator<T> Array<T>::begin()
    {
        return ArrayIterator<T>(typedData());
    }

    template<typename T>
    INLINE ArrayIterator<T> Array<T>::end()
    {
        return ArrayIterator<T>(typedData() + size());
    }

    template<typename T>
    INLINE ConstArrayIterator<T> Array<T>::begin() const
    {
        return ConstArrayIterator<T>(typedData());
    }

    template<typename T>
    INLINE ConstArrayIterator<T> Array<T>::end() const
    {
        return ConstArrayIterator<T>(typedData() + size());
    }

    template<typename T>
    T* Array<T>::allocateUninitialized(uint32_t count)
    {
        auto newSize  = size() + count;
        reserve(newSize);
        return typedData() + BaseArray::changeSize(newSize);
    }

    template<typename T>
    INLINE T* Array<T>::allocate(uint32_t count)
    {
        auto newSize  = size() + count;
        reserve(newSize);
        std::uninitialized_default_construct_n(typedData() + size(), count);
        return typedData() + BaseArray::changeSize(newSize);
    }

    template<typename T>
    INLINE T* Array<T>::allocateWith(uint32_t count, const T& templateElement)
    {
        auto newSize  = size() + count;
        reserve(newSize);
        std::uninitialized_fill_n(typedData() + size(), count, templateElement);
        return typedData() + BaseArray::changeSize(newSize);
    }

    template<typename T>
    void Array<T>::makeLocal()
    {
        BaseArray::makeLocal(POOL_CONTAINERS, size(), sizeof(T), __alignof(T), typeid(typename std::remove_cv<T>::type).name());
    }

    template<typename T>
    Buffer Array<T>::createBuffer(base::mem::PoolID poolID, uint32_t forcedAlignment) const
    {
        if (empty())
            return Buffer();

        return Buffer::Create(poolID, dataSize(), std::max<uint32_t>(alignof(T), forcedAlignment), data());
    }

    static void NoFreeFunc(mem::PoolID pool, void* memory, uint64_t size)
    {
        // nothing
    }
    template<typename T>
    Buffer Array<T>::createAliasedBuffer() const
    {
        if (empty())
            return Buffer();

        return Buffer::CreateExternal(POOL_TEMP, dataSize(), (void*) data(), &NoFreeFunc);
    }

} // base
