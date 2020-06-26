/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers\dynamic #]
***/

#pragma once

#include "arrayIterator.h"
#include "baseArray.h"

namespace base
{

	enum EInplaceArrayData { InplaceArrayData };

    /// Dynamic array class
    /*
        A template version of dynamic array that can hold elements of type T.
        
        This array DOES NOT behave in the same way as the std::vector do. The main difference
        is items internally are held in a void* array and the interpretation is done as needed.
        This allows the type of the element to be forward declared.

        Array buffer is also allocated using normal memory operations (realloc) instead of using
        new/delete. Array elements are constructed using the placement new when needed - ie. when they are added to the array.

        This code also assumes that classes stored in the array can be RELOCATED without calling 
        their's copy constructor/assignment operator code. This is faster that doing proper copying.
    */
    template < class T >
    class Array : protected BaseArray
    {
    public:
        Array();
        Array(const Array<T>& other);
		Array(const T* ptr, uint32_t size); // copy data
        Array(Array<T>&& other);
        ~Array();

        Array<T>& operator=(const Array<T>& other);
        Array<T>& operator=(Array<T>&& other);

        //--

        //! returns true if array is empty (no elements)
        bool empty() const;

        //! returns true if array is full (size() == capacity(), next push will resize)
        bool full() const;

        //! returns the number of elements in the array
        uint32_t size() const;

        //! returns UNSIGNED maximum number of elements the array can hold before resizing
        uint32_t capacity() const;

        //! returns SIGNED last valid element index of the array, can be used for reverse iteration
        int lastValidIndex() const; // size - 1

        //! get untyped pointer to array storage
        void* data();

        //! get untyped read-only pointer to array storage
        const void* data() const;

        //! get total size of data held by this array (in bytes)
        uint32_t dataSize() const;

        //! get typed read-only pointer to array storage
        const T* typedData() const;

        //! get type pointer to array storage
        T* typedData();

        //! get last element of array, illegal if array is empty
        T& back();

        //! get last element of array, illegal if array is empty
        const T& back() const;

        //! get last element of array, illegal if array is empty
        T& front();

        //!gGet last element of array, illegal if array is empty
        const T& front() const;

        //! access array element, asserts if index is invalid
        //! NOTE: provides basic index check if not in Release build
        T& operator[](uint32_t index);

        //! access array element, asserts if index is invalid
        //! NOTE: provides basic index check if not in Release build
        const T &operator[](uint32_t index) const;

        ///-

        //! allocate new initialized (default constructed elements)
        T* allocate(uint32_t count);

        //! allocate new elements initialized with copy constructor from the template
        T* allocateWith(uint32_t count, const T& templateElement);

        //! allocate memory for new elements but do not initialize anything
        //! NOTE: this is the fastest way to grow a vector, should be used when adding bulk of POD data to the array
        T* allocateUninitialized(uint32_t count);

        //! add a single element at the end of the array and initialize it with copy constructor from provided template
        //! NOTE: is illegal to add element from the same array (arr.pushBack(arr[5]))
        void pushBack(const T &item);

        //! add a single element at the end of the array and initialize it with move constructor from provided template potentially destroying the source element
        //! NOTE: is illegal to add element from the same array (arr.pushBack(arr[5]))
        void pushBack(T &&item);

        //! push back many elements, the great thing is that it this function reserves memory only once and can use fast copy, great to merge POD arrays
        void pushBack(const T* items, uint32_t count);

        //! add element at the end of the array but only if it's not already added
        //! NOTE: this is a convenience function, not for serious use
        void pushBackUnique(const T &element);

        //! add a single element at the end of the array and construct it directly
        template<typename ...Args>
        T& emplaceBack(Args&&... args);

        //! insert a single element at given index in the array, the new elements becomes the one at specified index
        void insert(uint32_t index, const T &item);

        //! insert copy of multiple elements at given index in the array
        void insert(uint32_t index, const T* items, uint32_t count);

        //! insert copy of multiple elements at given index in the array
        void insertWith(uint32_t index, uint32_t count, const T& itemTemplate);

        //--

        //! remove last element from array, illegal if array is empty
        void popBack();

        //! remove (stable) elements at given index from the array, order is preserved
        void erase(uint32_t index, uint32_t count = 1);

        //! remove elements at given index from the array, order is not preserved but this is faster as this moves elements from the end of the array to fill the space
        void eraseUnordered(uint32_t index, uint32_t count = 1);

        //! find (via linear search) and remove all matching elements from the array, returns number of elements removed, order of elements is preserved
        template< typename FK >
        uint32_t remove(const FK &item);

        //! find (via linear search) and remove all matching elements from the array, returns number of elements removed, order of elements is NOT preserved but the function is parsed
        template< typename FK >
        uint32_t removeUnordered(const FK &item);

        //--

        //! Check if array contains given element (linear search)
        template< typename FK >
        bool contains(const FK& element) const;

        //! Find index of given element in the array (linear search) returns -1 if the element was not found
        template< typename FK >
        int find(const FK& element) const;

        //! Find index of given element in the array matching given predicate, returns -1 if the element was not found
        template< typename FK >
        int findIf(const FK& func) const;

        //! Find index of given element in the array matching given predicate, return the value if found or default if not found
        template< typename FK >
        const T& findIfOrDefault(const FK& func, const T& defaultValue = T()) const;

        //--

        //! Get iterator to start of the array
        ArrayIterator<T> begin();

        //! Get iterator to end of the array
        ArrayIterator<T> end();

        //! Get read only iterator to start of the array
        ConstArrayIterator<T> begin() const;

        //! Get read only iterator to end of the array
        ConstArrayIterator<T> end() const;

        //--

        //! reset array count to zero, destroys existing elements but does not free memory
        void reset();

        //! clear array, destroy all elements, release all memory
        void clear();

        //! clear pointer array, call MemDelete() on all members, release all memory
        void clearPtr();

        //! reduce array memory buffer to fit only actual array size
        void shrink();

        //! reserve space in array for AT LEAST that many elements, does reduce size, does not call constructor
        void reserve(uint32_t newSize);

        //! resize array to EXACTLY the specific size, call constructors/destructor on elements accordingly, resizes memory block, new elements are initialized with default constructor
        void resize(uint32_t newSize);

        //! resize array to specific size, if any new elements are created initialize them with the template
        void resizeWith(uint32_t newSize, const T& elementTemplate = T());

        //! ensure array has at AT LEAST given count of initialized element, similar to resize but the buffer can be bigger and we don't reduce/free memory. new elements are initialized with default constructor
        void prepare(uint32_t minimalSize);

        //! ensure array has at AT LEAST given count of initialized element, similar to resize but the buffer can be bigger and we don't free memory. new elements are initialized with provided template
        void prepareWith(uint32_t minimalSize, const T& elementTemplate = T());

        //--

        //! Compare array content (checks all elements). Returns true if arrays are equal.
        bool operator==(const Array<T> &other) const;

        //! Compare array content (checks all elements). Returns true if arrays are not equal.
        bool operator!=(const Array<T> &other) const;

        //--

        //! Create a buffer with copy of the data
        Buffer createBuffer(mem::PoolID poolID = POOL_TEMP, uint32_t forcedAlignment=0) const;

        //! Create a special aliased buffer that will point to the same memory
        Buffer createAliasedBuffer() const;

	protected:
		Array(void* ptr, uint32_t maxCapcity, EInplaceArrayData flag); // copy data

        void makeLocal();
    };

} // base

#include "array.inl"