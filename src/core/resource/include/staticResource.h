/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

///---

namespace prv
{
    class StaticResourceRegistry;
} // prv

/// static (engine global) resource reference
/// has the benefit of being directly loadable
class CORE_RESOURCE_API IStaticResource : public NoCopy
{
public:
    IStaticResource(const char* path);
    virtual ~IStaticResource();

    /// get the resource path
    INLINE StringView path() const { return m_path; }

    //--

    /// load the resource and get the loaded object
    BaseReference load() const;

    //---

    // collect all static resources that are defined in C++ code
    static void CollectAllResources(Array<IStaticResource*>& outResources);

private:
    const char* m_path = nullptr;
    IStaticResource* m_next;
    IStaticResource* m_prev;

    friend class prv::StaticResourceRegistry;
};

///---

/// typed static resource, can be directly loaded
template< typename T >
class StaticResource : public IStaticResource
{
public:
    INLINE StaticResource(const char* path)
        : IStaticResource(path)
    {}

    /// load the resource and get the loaded object
    INLINE ResourceRef<T> load() const
    {
        const auto baseRef = IStaticResource::load();
        return ResourceRef<T>(baseRef.id(), rtti_cast<T>(baseRef.resource()));
    }
};

END_BOOMER_NAMESPACE()
