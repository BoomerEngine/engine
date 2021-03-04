/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#pragma once

#include "path.h"
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
    BaseAsyncReference(const ResourcePath& key);
    ~BaseAsyncReference();

    BaseAsyncReference& operator=(const BaseAsyncReference& other);
    BaseAsyncReference& operator=(BaseAsyncReference&& other);

    ///--

    // get resource path bound to this reference
    // NOTE: this is not set for path-less embedded resources
    INLINE const ResourcePath& path() const { return m_path; }

    // is this a valid reference ? we need either a key or an object
    INLINE bool valid() const { return !m_path.empty(); }

    // is this an empty reference ? empty reference is one without key and without object
    INLINE bool empty() const { return m_path.empty(); }

    //---

    // clear reference
    void reset();

    // setup reference to a given resource key
    void set(const ResourcePath& key);

    //---

    // load the resource into a "loaded" reference, a custom loader can be provided 
    ResourcePtr load() const CAN_YIELD;

    //---

    // load asynchronously, call the callback when loaded
    // basically a nice wrapper for a fiber job with a fast short circuit if the resource is already loaded
    // NOTE: THIS DOES NOT CHANGE THE STATE OF THE OBJECT, just calls the callback, use the ensureLoaded() to do that
    void loadAsync(const std::function<void(const ResourcePtr&)>& onLoadedFunc) const; // <--- look here, a "const"

    //--

    /// test references for equality, if key is given is checked first, if no key is given then and only then the object pointers are checked
    bool operator==(const BaseAsyncReference& other) const;
    bool operator!=(const BaseAsyncReference& other) const;

    //--

    // print to text
    void print(IFormatStream& f) const;

    //--

protected:
    ResourcePath m_path;

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

    INLINE ResourceAsyncRef(const ResourcePath& path)
        : BaseAsyncReference(path)
    {}

    INLINE bool operator==(const ResourceAsyncRef<T>& other) const { return BaseAsyncReference::operator==(other); }
    INLINE bool operator!=(const ResourceAsyncRef<T>& other) const { return BaseAsyncReference::operator!=(other); }

    template< typename U = T >
    INLINE RefPtr<U> load() const CAN_YIELD
    {
        static_assert(std::is_base_of<T, U>::value || std::is_base_of<U, T>::value, "Types are unrelated");
        return rtti_cast<U>(BaseAsyncReference::load());
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

END_BOOMER_NAMESPACE()
