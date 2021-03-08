/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#pragma once

#include "reference.h"

BEGIN_BOOMER_NAMESPACE()

///--------

/// base concept of deferred loading reference to resource
class CORE_RESOURCE_API BaseAsyncReference
{
public:
    BaseAsyncReference();
    BaseAsyncReference(const BaseAsyncReference& other);
    BaseAsyncReference(BaseAsyncReference&& other);
    BaseAsyncReference(const ResourceID& key);
    ~BaseAsyncReference();

    BaseAsyncReference& operator=(const BaseAsyncReference& other);
    BaseAsyncReference& operator=(BaseAsyncReference&& other);

    ///--

    // get ID of assigned resource
    INLINE const ResourceID& id() const { return m_id; }

    // is this an empty reference ? empty reference is one without key and without object
    INLINE bool empty() const { return m_id.empty(); }

    // check if assigned
    INLINE operator bool() const { return !empty(); }

    //---

    // clear reference
    void reset();

    //---

    // load the resource into a "loaded" reference, a custom loader can be provided 
    BaseReference load() const CAN_YIELD;

    //--

    /// test references for equality, if key is given is checked first, if no key is given then and only then the object pointers are checked
    bool operator==(const BaseAsyncReference& other) const;
    bool operator!=(const BaseAsyncReference& other) const;

    //--

    // print to text
    void print(IFormatStream& f) const;

    //--

protected:
    ResourceID m_id;

    static const BaseAsyncReference& EMPTY_HANDLE();
};

///--------

/// async reference to resource
template< class T >
class ResourceAsyncRef : public BaseAsyncReference
{
public:
    INLINE ResourceAsyncRef() = default;
    INLINE ResourceAsyncRef(const ResourceAsyncRef<T>& other) = default;
    INLINE ResourceAsyncRef(ResourceAsyncRef<T>&& other) = default;
    INLINE ResourceAsyncRef& operator=(const ResourceAsyncRef<T>& other) = default;
    INLINE ResourceAsyncRef& operator=(ResourceAsyncRef<T> && other) = default;

    INLINE ResourceAsyncRef(const ResourceID& id)
        : BaseAsyncReference(id)
    {}

    INLINE bool operator==(const ResourceAsyncRef<T>& other) const { return BaseAsyncReference::operator==(other); }
    INLINE bool operator!=(const ResourceAsyncRef<T>& other) const { return BaseAsyncReference::operator!=(other); }

    template< typename U = T >
    INLINE ResourceRef<U> load() const CAN_YIELD
    {
        static_assert(std::is_base_of<T, U>::value || std::is_base_of<U, T>::value, "Types are unrelated");
        const auto loaded = rtti_cast<U>(BaseAsyncReference::load().resource());
        return ResourceRef<U>(path(), loaded);
    }
};

///--------

// type naming, generate a rtti type name for a Ref<T> with given class
extern CORE_RESOURCE_API StringID FormatAsyncRefTypeName(StringID className);

// get a reference type name
template< typename T >
INLINE StringID FormatAsyncRefTypeName() {
    static const const typeName = FormatAsyncRefTypeName(T::GetStaticClass()->name());
    return typeName;
}

//--------

namespace resolve
{
    // type name resolve for strong handles
    template<typename T>
    struct TypeName<ResourceAsyncRef<T>>
    {
        static StringID GetTypeName()
        {
            static auto cachedTypeName = FormatAsyncRefTypeName(TypeName<T>::GetTypeName());
            return cachedTypeName;
        }
    };
}

//--------

END_BOOMER_NAMESPACE()
