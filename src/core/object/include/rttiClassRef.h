/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\typeref #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

template< class BaseClass >
class SpecificClassType;

/// a reference to a rtti class type
class CORE_OBJECT_API ClassType
{
public:
    INLINE ClassType() : m_classType(nullptr) {}
    INLINE ClassType(const IClassType* type) : m_classType(type) {}
    INLINE ClassType(std::nullptr_t) : m_classType(nullptr) {}

    INLINE ClassType(const ClassType& other) = default;
    INLINE ClassType(ClassType&& other) = default;
    INLINE ClassType& operator=(const ClassType& other) = default;
    INLINE ClassType& operator=(ClassType&& other) = default;
    INLINE ~ClassType() = default;

    template< typename T >
    INLINE ClassType& operator=(const SpecificClassType<T>& other);

    template< typename T >
    INLINE ClassType(const SpecificClassType<T>& other);

    //--

    // simple comparisons
    INLINE bool operator==(const ClassType& other) const { return m_classType == other.m_classType; }
    INLINE bool operator!=(const ClassType& other) const { return m_classType != other.m_classType; }
    INLINE bool operator==(const IType* other) const { return (const IType*)m_classType == other; }
    INLINE bool operator!=(const IType* other) const { return (const IType*)m_classType != other; }

    template< typename T >
    INLINE bool operator==(const SpecificClassType<T>& other) const;
    
    template< typename T >
    INLINE bool operator!=(const SpecificClassType<T>& other) const;

    //--


    //! is this an empty type reference ?
    INLINE bool empty() const { return m_classType == nullptr; }

    //! cast to bool - true if not null
    INLINE operator bool() const { return m_classType != nullptr; }

    // get referenced type
    INLINE const IClassType* ptr() const { return m_classType; }

    // get referenced type directly
    INLINE const IClassType* operator->() const { return m_classType; }

    //--

    // check if this class is of required type
    bool is(ClassType baseClassType) const;

    // check if this class is of required type
    template< typename T >
    INLINE bool is() const { return is(T::GetStaticClass()); }

    //--

    // cast to a specific class type, "meta cast", will null the class if the type does not match
    ClassType cast(ClassType baseClass) const;

    // cast to a specific class type, "meta cast"
    // ie. m_genericRef.cast<Entity> -> SpecificClassType<Entity>
    template< typename T >
    INLINE SpecificClassType<T> cast() const;

    //--

    // get empty class reference
    static const ClassType& EMPTY();

    //--

    // get name of the class
    StringID name() const;

    // get a string representation (for debug and error printing)
    void print(IFormatStream& f) const;
        
    //--

    // create object from this class type, object will be created only if class is of sufficient type T and not abstract
    template< typename T >
    INLINE RefPtr<T> create() const;

    //--

    // get hash of the type reference
    INLINE static uint32_t CalcHash(ClassType key) { return std::hash<const void*>{}(key.m_classType); }

protected:
    const IClassType* m_classType;
};

//----

// a template implementation of ClassType that constrains class type to given base class
// NOTE: base class is NOT CHECKED anywhere, having invalid class stored will cause runtime errors (asserts)
template< class BaseClass >
class SpecificClassType
{
public:
    INLINE SpecificClassType() = default;
    INLINE SpecificClassType(const IClassType* type);
    INLINE SpecificClassType(const IClassType& type); // does not test
    //INLINE SpecificClassType(const ClassType& type) { ValidateClassType(type, BaseClass::GetStaticClass()); m_classType = type.ptr(); };
    INLINE SpecificClassType(std::nullptr_t) {};

    INLINE SpecificClassType(const SpecificClassType& other) = default;
    INLINE SpecificClassType(SpecificClassType&& other) = default;
    INLINE SpecificClassType& operator=(const SpecificClassType& other) = default;
    INLINE SpecificClassType& operator=(SpecificClassType&& other) = default;
    INLINE ~SpecificClassType() = default;

    template< typename U >
    INLINE SpecificClassType(const SpecificClassType<U>& other);

    template< typename U >
    INLINE SpecificClassType& operator=(const SpecificClassType<U>& pointer);

    //--

    // simple comparisons
    INLINE bool operator==(const ClassType& other) const { return ptr() == other.ptr(); }
    INLINE bool operator!=(const ClassType& other) const { return ptr() != other.ptr(); }
    INLINE bool operator==(const IType* other) const { return ptr() == other; }
    INLINE bool operator!=(const IType* other) const { return ptr() != other; }

    template< typename U >
    INLINE bool operator==(const SpecificClassType<U>& other) const { return ptr() == other.ptr(); }

    template< typename U >
    INLINE bool operator!=(const SpecificClassType<U>& other) const { return ptr() != other.ptr(); }

    //--
        
    //! is this an empty type reference ?
    INLINE bool empty() const { return m_classType.empty(); }

    //! cast to bool - true if not null
    INLINE operator bool() const { return !m_classType.empty(); }

    // get referenced type
    INLINE const IClassType* ptr() const { return m_classType.ptr(); }

    // get referenced type directly
    INLINE const IClassType* operator->() const { return m_classType.ptr(); }

    // get hash of the type reference
    INLINE uint64_t hash() const { return m_classType.hash(); }

    //--

    // check if this class is of required type
    INLINE bool is(ClassType baseClassType) const { return m_classType.is(baseClassType); }

    // check if this class is of required type
    template< typename T >
    INLINE bool is() const { return m_classType.is(T::GetStaticClass()); }

    //--

    // cast to a specific class type, "meta cast"
    // ie. m_genericRef.cast<Entity> -> SpecificClassType<Entity>
    template< typename T >
    INLINE SpecificClassType<T> cast() const { return m_classType.cast<T>(); }

    //--

    // get name of the class
    INLINE StringID name() const { return m_classType.name(); }

    // get a string representation (for debug and error printing)
    INLINE void print(IFormatStream& f) const { m_classType.print(f); }

    //--

    // create object from this class type, object will be created only if class is of sufficient type T and not abstract
    // SpecificClassType<Entity> eclass;
    // eclass.create() -> RefPtr<Entity>
    INLINE RefPtr<BaseClass> create() const { return m_classType.create<BaseClass>(); }

    //--

    // get hash of the type reference
    INLINE static uint32_t CalcHash(ClassType key) { return ClassType::CalcHash(key.ptr()); }

    INLINE static uint32_t CalcHash(SpecificClassType<BaseClass> key) { return ClassType::CalcHash(key.ptr()); }

private:
    ClassType m_classType;
};

//----

template< typename T >
INLINE bool ClassType::operator==(const SpecificClassType<T>& other) const { return m_classType == other.ptr(); }

template< typename T >
INLINE bool ClassType::operator!=(const SpecificClassType<T>& other) const { return m_classType != other.ptr(); }

template< typename T >
INLINE ClassType& ClassType::operator=(const SpecificClassType<T>& other)
{
    m_classType = other.ptr();
    return *this;
}

template< typename T >
INLINE ClassType::ClassType(const SpecificClassType<T>& other)
    : m_classType(other.ptr())
{}

template< typename BaseClass >
INLINE SpecificClassType<BaseClass>::SpecificClassType(const IClassType* type)
{
    DEBUG_CHECK_EX(!type || type->is<BaseClass>(), "Trying to initialize a specific class from incompatible class type");
    if (type->is<BaseClass>()) // to prevent weird bugs it's better not to assign
        m_classType = type; 
}

template< typename BaseClass >
INLINE SpecificClassType<BaseClass>::SpecificClassType(const IClassType& type)
    : m_classType(&type)
{
}

template< typename BaseClass >
template< typename U >
INLINE SpecificClassType<BaseClass>::SpecificClassType(const SpecificClassType<U>& other)
{
    static_assert(std::is_base_of<BaseClass, U>::value, "Cannot down-cast a pointer through construction, use rtti_cast");
    m_classType = other.ptr();
}

template< typename BaseClass >
template< typename U >
INLINE SpecificClassType<BaseClass>& SpecificClassType<BaseClass>::operator=(const SpecificClassType<U>& other)
{
    static_assert(std::is_base_of<BaseClass, U>::value, "Cannot down-cast a pointer through construction, use rtti_cast");
    m_classType = other.ptr();
    return *this;
}

//----

template< typename T >
INLINE SpecificClassType<T> ClassType::cast() const
{
    if (m_classType && m_classType->is(T::GetStaticClass()))
        return SpecificClassType<T>(m_classType);
    else
        return SpecificClassType<T>(nullptr);
}

//---

// class ref cast for RTTI SpecificClassType, will null the class ref if class does not match
// ie. SpecificClassType<Entity> entityClass = rtti_cast<Entity>(anyClass);
template< class _DestType >
INLINE SpecificClassType< _DestType > rtti_cast(const ClassType& srcClassType)
{
    return srcClassType.cast<_DestType>();
}

//---

// class ref cast for RTTI SpecificClassType, will null the class ref if class does not match
// ie. SpecificClassType<Entity> entityClass = rtti_cast<Entity>(anyClass);
template< class _DestType, class _SrcType >
INLINE SpecificClassType< _DestType > rtti_cast(const SpecificClassType< _SrcType>& srcClassType)
{
    static_assert(std::is_base_of<_DestType, _SrcType>::value || std::is_base_of<_SrcType, _DestType>::value, "Cannot cast between unrelated class types");
    return srcClassType.cast<_DestType>();
}

//---
    
// create instance of object
template< typename T >
INLINE RefPtr<T> ClassType::create() const
{
    if (m_classType && !m_classType->isAbstract() && m_classType->is(T::GetStaticClass()))
        return m_classType->create<T>();
    else
        return nullptr;
}

//--

END_BOOMER_NAMESPACE()
