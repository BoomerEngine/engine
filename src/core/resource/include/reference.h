/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#pragma once

#include "path.h"

BEGIN_BOOMER_NAMESPACE()

///--------

/// base concept of serializable resource reference
class CORE_RESOURCE_API BaseReference
{
public:
    BaseReference();
    BaseReference(const BaseReference& other);
    BaseReference(const ResourcePath& key, IResource* ptr=nullptr);
    BaseReference(BaseReference&& other);
    BaseReference(std::nullptr_t);
    ~BaseReference();

    BaseReference& operator=(const BaseReference& other);
    BaseReference& operator=(BaseReference&& other);

    ///--

    // get resource "key" -> load class + resource path
    // NOTE: this is not set for path-less embedded resources
    INLINE const ResourcePath& path() const { return m_path; }

    // peak current handle, do not load when missing
    INLINE const ResourcePtr& resource() const { return m_handle; }

    // is this an empty reference ? empty reference is one without key and without object
    INLINE bool empty() const { return m_path.empty(); }

    // validate
    INLINE operator bool() const { return !m_path.empty(); }

    //---

    // clear reference
    void reset();

    //--

    /// test references for equality, only PATHS are checked
    bool operator==(const BaseReference& other) const;
    bool operator!=(const BaseReference& other) const;

    //--

    // print to text
    // for a key reference it prints: "Texture$engine/textures/lena.png"
    // for inlined objects this just prints: "Texture"
    void print(IFormatStream& f) const;

    //--

protected:
    ResourcePath m_path; // never empty if reference is valid
    ResourcePtr m_handle; // may be empty if resource is missing or we setup a unloaded ref

    static const ResourcePtr& EMPTY_HANDLE();
};

///--------

/// reference to resource, preserves the path even if the actual resource was not loaded (errors)
template< class T >
class ResourceRef : public BaseReference
{
public:
    INLINE ResourceRef() = default;
    INLINE ResourceRef(std::nullptr_t) {};
    INLINE ResourceRef(ResourceRef<T>&& other) = default;
    INLINE ResourceRef& operator=(const ResourceRef<T>& other) = default;
    INLINE ResourceRef& operator=(ResourceRef<T> && other) = default;
    INLINE ResourceRef(const ResourceRef<T>& other) = default;
    INLINE ResourceRef(const ResourcePath& key, IResource* ptr = nullptr) : BaseReference(key, ptr) {};

    template< typename U >
    INLINE ResourceRef(const ResourceRef<U>& other)
        : BaseReference(other)
    {
        ASSERT_EX(0 == (int64_t) static_cast<T*>((U*)nullptr), "Address to object in the shared pointer cannot change due to case");
        static_assert(std::is_base_of<T, U>::value, "Cannot down-cast a pointer through construction, use rtti_cast");
    }

    // peak current handle, do not load when missing
    INLINE RefPtr<T> resource() const { return rtti_cast<T>(BaseReference::resource()); }

    ///---

    // check for equality
    INLINE bool operator==(const ResourceRef<T>& other) const { return BaseReference::operator==(other); }

    // check for in equality
    INLINE bool operator!=(const ResourceRef<T>& other) const { return BaseReference::operator!=(other); }

    // validate
    INLINE operator bool() const { return !empty(); }
};

///--------

// type naming, generate a rtti type name for a ResourceRef<T> with given class
extern CORE_RESOURCE_API StringID FormatRefTypeName(StringID className);

// get a reference type name
template< typename T >
INLINE StringID FormatRefTypeName() {
    static const const typeName = FormatRefTypeName(T::GetStaticClass()->name());
    return typeName;
}

//--------

END_BOOMER_NAMESPACE()

//------

