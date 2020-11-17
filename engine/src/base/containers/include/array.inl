/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#pragma once

namespace base
{

    template<typename T>
    ALWAYS_INLINE Array<T>::Array()
    {};

    template<typename T>
    Array<T>::Array(const T* ptr, Count size)
    {
		auto elems  = allocateUninitialized(size);
		std::uninitialized_copy_n(ptr, size, elems);
    }

	template<typename T>
	Array<T>::Array(BaseArrayBuffer&& buffer)
		: BaseArray(std::move(buffer))
	{}

    template<typename T>
    Array<T>::Array(const Array<T> &other)
    {
		pushBack(other.typedData(), other.size());
    }

    template<typename T>
    Array<T>::Array(Array<T> &&other)
    {
        *this = std::move(other);
    }

    template<typename T>
    Array<T>::~Array()
    {
        clear();
    }

    template<typename T>
    ALWAYS_INLINE Count Array<T>::size() const
    {
        return BaseArray::size();
    }

    template<typename T>
    ALWAYS_INLINE int Array<T>::lastValidIndex() const
    {
        return (int)BaseArray::size() - 1;
    }

    template<typename T>
    ALWAYS_INLINE Count Array<T>::capacity() const
    {
        return BaseArray::capacity();
    }

    template<typename T>
    ALWAYS_INLINE void* Array<T>::data()
    {
        return BaseArray::data();
    }

    template<typename T>
    ALWAYS_INLINE const void* Array<T>::data() const
    {
        return BaseArray::data();
    }

    template<typename T>
    ALWAYS_INLINE uint64_t Array<T>::dataSize() const
    {
        return ((uint64_t)size()) * sizeof(T);
    }

    template<typename T>
    ALWAYS_INLINE const T* Array<T>::typedData() const
    {
        return (const T*)data();
    }

    template<typename T>
    ALWAYS_INLINE T* Array<T>::typedData()
    {
        return (T*)data();
    }

    template<typename T>
    ALWAYS_INLINE bool Array<T>::empty() const
    {
        return BaseArray::empty();
    }

    template<typename T>
    ALWAYS_INLINE bool Array<T>::full() const
    {
        return BaseArray::full();
    }

    template<typename T>
    ALWAYS_INLINE T& Array<T>::operator[](Index index)
    {
        checkIndex(index);
        return typedData()[index];
    }

    template<typename T>
    ALWAYS_INLINE const T& Array<T>::operator[](Index index) const
    {
        checkIndex(index);
        return typedData()[index];
    }

    template<typename T>
    ALWAYS_INLINE PointerRange<T> Array<T>::pointerRange()
    {
        return PointerRange<T>(typedData(), size());
    }

    template<typename T>
    ALWAYS_INLINE const PointerRange<T> Array<T>::pointerRange() const
    {
        return PointerRange<T>(typedData(), size());
    }

    template<typename T>
    ALWAYS_INLINE IndexRange Array<T>::indexRange() const
    {
        return IndexRange(0, size());
    }

    template<typename T>
    ALWAYS_INLINE BasePointerRange Array<T>::bytes() const
    {
        return BasePointerRange(data(), (const uint8_t*)data() + dataSize());
    }

    template<typename T>
    void Array<T>::popBack()
    {
        checkIndex(size() - 1);
        changeSize(size() - 1);
        std::destroy_n(typedData() + size(), 1);
    }

    template<typename T>
    ALWAYS_INLINE T& Array<T>::back()
    {
        checkIndex(size() - 1);
        return typedData()[size() - 1];
    }

    template<typename T>
    ALWAYS_INLINE const T& Array<T>::back() const
    {
        checkIndex(size() - 1);
        return typedData()[size() - 1];
    }

    template<typename T>
    ALWAYS_INLINE T& Array<T>::front()
    {
        checkIndex(0);
        return typedData()[0];
    }

    template<typename T>
    ALWAYS_INLINE const T& Array<T>::front() const
    {
        checkIndex(0);
        return typedData()[0];
    }

    template<typename T>
    void Array<T>::shrink()
    {
        BaseArray::changeCapacity(size(), sizeof(T) * capacity(), sizeof(T) * size(), __alignof(T), typeid(typename std::remove_cv<T>::type).name());
    }

    template<typename T>
    void Array<T>::resizeWith(Count newSize, const T& elementTemplate)
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
    void Array<T>::resize(Count newSize)
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
    void Array<T>::prepare(Count minimalSize)
    {
        if (minimalSize > size())
        {
            reserve(minimalSize);
			std::uninitialized_default_construct_n(begin() + size(), minimalSize - size());
			BaseArray::changeSize(minimalSize);
        }
    }

    template<typename T>
    void Array<T>::prepareWith(Count minimalSize, const T& elementTemplate /*= T()*/)
    {
        if (minimalSize > size())
        {
            reserve(minimalSize);
            std::uninitialized_fill_n(begin() + size(), minimalSize - size(), elementTemplate);
			BaseArray::changeSize(minimalSize);
		}
    }

    template<typename T>
    void Array<T>::reserve(Count newSize)
    {
        if (newSize > capacity())
        {
            const auto requiresBufferSize = sizeof(T) * newSize;
            const auto currentBufferSize = sizeof(T) * capacity();
            auto allocatedBufferSize = currentBufferSize;
            while (allocatedBufferSize < requiresBufferSize)
                allocatedBufferSize = BaseArray::CalcNextBufferSize(allocatedBufferSize);

            auto newCapacity = allocatedBufferSize / sizeof(T);
            ASSERT_EX(newCapacity >= newSize, "Computation overflow");

			BaseArray::changeCapacity(newCapacity, currentBufferSize, allocatedBufferSize, __alignof(T), typeid(typename std::remove_cv<T>::type).name());
        }
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::contains(const FK& element) const
    {
        return pointerRange().contains(element);
    }

    template<typename T>
    template<typename FK>
    Index Array<T>::find(const FK& element) const
    {
        Index ret = -1;
        pointerRange().findFirst(element, ret);
        return ret;
    }

    template<typename T>
    template<typename FK>
    Index Array<T>::findLast(const FK& func) const
    {
        Index ret = size();
        if (!pointerRange().findLast(element, ret))
            return Index();
        return ret;
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::find(const FK& key, Index& outFoundIndex) const
    {
        return pointerRange().findFirst(key, outFoundIndex);
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::findLast(const FK& key, Index& outFoundIndex) const
    {
        return pointerRange().findLast(key, outFoundIndex);
    }

    template<typename T>
    void Array<T>::erase(Index index, Count count)
    {
        checkIndexRange(index, count);

		std::destroy_n(begin() + index, count);
		memmove(typedData() + index, typedData() + index + count, (size() - (index + count)) * sizeof(T));
        
        BaseArray::changeSize(size() - count);
    }

    template<typename T>
    void Array<T>::eraseUnordered(Index index)
    {
        checkIndex(index);

        const auto last = lastValidIndex();
        if (index < last)
            std::swap(typedData()[index], typedData()[last]);

		std::destroy_n(begin() + last, 1);

        BaseArray::changeSize(size() - 1);
    }

    template<typename T>
    void Array<T>::clear()
    {
        std::destroy(begin(), end());
        BaseArray::changeSize(0);
	    BaseArray::changeCapacity(0, capacity() * sizeof(T), 0, __alignof(T), typeid(typename std::remove_cv<T>::type).name());
    }

    template<typename T>
    void Array<T>::clearPtr()
    {
        std::for_each(begin(), end(), [](T& val) { MemDelete(val); });
        std::destroy(begin(), end());
        BaseArray::changeSize(0);
        BaseArray::changeCapacity(0, capacity() * sizeof(T), 0, __alignof(T), typeid(typename std::remove_cv<T>::type).name());
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
			reset(); // destroy elements without freeing memory
			std::uninitialized_copy_n(other.begin(), other.size(), allocateUninitialized(other.size()));
        }

        return *this;
    }

    template<typename T>
    Array<T>& Array<T>::operator=(Array<T> &&other)
    {
        if (this != &other)
        {
            if (other.owned())
            {
                clear();

                m_buffer = std::move(other.m_buffer);
                m_size = other.m_size;
                other.m_size = 0;
            }
            else
            {
                reset();
                std::uninitialized_move_n(other.typedData(), other.size(), allocateUninitialized(other.size()));

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
    void Array<T>::pushBack(const T &item)
    {
        new (allocateUninitialized(1)) T(item);
    }

    template<typename T>
    void Array<T>::pushBack(const T* items, Count count)
    {
        std::uninitialized_copy_n(items, count, allocateUninitialized(count));
    }

    template<typename T>
    void Array<T>::pushBack(T &&item)
    {
        new (allocateUninitialized(1)) T(std::move(item));
    }

    template<typename T>
    template<typename ...Args>
    ALWAYS_INLINE T& Array<T>::emplaceBack(Args&&... args)
    {
        return *new (allocateUninitialized(1)) T(std::forward<Args>(args)...);
    }

    template<typename T>
    void Array<T>::pushBackUnique(const T &element)
    {
        if (!contains(element))
            pushBack(element);
    }

    template<typename T>
    void Array<T>::insert(Index index, const T &item)
    {
        insert(index, &item, 1);
    }

    template<typename T>
    void Array<T>::insertWith(Index index, Count count, const T& itemTemplate)
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
        for (Count i=0; i<count; ++i)
            std::uninitialized_copy_n(&itemTemplate, 1, typedData() + index + i);
    }

    template<typename T>
    void Array<T>::insert(Index index, const T* items, Count count)
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
    template<typename FK>
    Count Array<T>::removeAll(const FK &item)
    {
        auto it  = std::remove(begin(), end(), item);
        if (it == end())
            return 0;

		auto count = std::distance(it, end());

        std::destroy(it, end());
        BaseArray::changeSize(std::distance(begin(), it));
        return count;
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::remove(const FK& item)
    {
        Index index = -1;
        if (pointerRange().findFirst(item, index))
        {
            erase(index);
            return true;
        }

        return false;
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::removeLast(const FK& item)
    {
        Index index = size();
        if (pointerRange().findLast(item, index))
        {
            erase(index);
            return true;
        }

        return false;
    }

    template<typename T>
    template<typename FK>
    Count Array<T>::removeUnorderedAll(const FK &item)
    {
        Count removed = 0;

        Index cur = -1;
        while (pointerRange().findFirst(item, cur))
        {
            eraseUnordered(cur);
            removed += 1;
        }

        return removed;
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::removeUnordered(const FK& item)
    {
        Index index = -1;
        if (pointerRange().findFirst(item, index))
        {
            eraseUnordered(index);
            return true;
        }

        return false;
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::removeUnorderedLast(const FK& item)
    {
        Index index = size();
        if (pointerRange().findLast(item, index))
        {
            eraseUnordered(index);
            return true;
        }

        return false;
    }

    template<typename T>
    template<typename FK>
    Count Array<T>::replaceAll(const FK& item, const T& itemTemplate)
    {
        return pointerRange().replaceAll(item, itemTemplate);
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::replace(const FK& item, T&& itemTemplate)
    {
        return pointerRange().replaceFirst(item, itemTemplate);
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::replaceLast(const FK& item, T&& itemTemplate)
    {
        return pointerRange().replaceLast(item, itemTemplate);
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::replace(const FK& item, const T& itemTemplate)
    {
        return pointerRange().replaceFirst(item, itemTemplate);
    }

    template<typename T>
    template<typename FK>
    bool Array<T>::replaceLast(const FK& item, const T& itemTemplate)
    {
        return pointerRange().replaceLast(item, itemTemplate);
    }

    template<typename T>
    ArrayIterator<T> Array<T>::begin()
    {
        return ArrayIterator<T>(typedData());
    }

    template<typename T>
    ArrayIterator<T> Array<T>::end()
    {
        return ArrayIterator<T>(typedData() + size());
    }

    template<typename T>
    ConstArrayIterator<T> Array<T>::begin() const
    {
        return ConstArrayIterator<T>(typedData());
    }

    template<typename T>
    ConstArrayIterator<T> Array<T>::end() const
    {
        return ConstArrayIterator<T>(typedData() + size());
    }

    template<typename T>
    T* Array<T>::allocateUninitialized(Count count)
    {
        auto newSize  = size() + count;
        reserve(newSize);
        return typedData() + BaseArray::changeSize(newSize);
    }

    template<typename T>
    T* Array<T>::allocate(Count count)
    {
        auto newSize  = size() + count;
        reserve(newSize);
        std::uninitialized_default_construct_n(typedData() + size(), count);
        return typedData() + BaseArray::changeSize(newSize);
    }

    template<typename T>
    T* Array<T>::allocateWith(Count count, const T& templateElement)
    {
        auto newSize  = size() + count;
        reserve(newSize);
        std::uninitialized_fill_n(typedData() + size(), count, templateElement);
        return typedData() + BaseArray::changeSize(newSize);
    }

    template<typename T>
    Buffer Array<T>::createBuffer(PoolTag poolID, uint64_t forcedAlignment) const
    {
        if (empty())
            return Buffer();

        return Buffer::Create(poolID, dataSize(), std::max(alignof(T), forcedAlignment), data());
    }

    static void NoFreeFunc(PoolTag pool, void* memory, uint64_t size)
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
