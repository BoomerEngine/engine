/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\static #]
***/

#include "build.h"
#include "resource.h"
#include "staticResource.h"
#include "loader.h"

BEGIN_BOOMER_NAMESPACE()

//--

namespace prv
{
    class StaticResourceRegistry : public ISingleton
    {
        DECLARE_SINGLETON(StaticResourceRegistry);

    public:
        INLINE StaticResourceRegistry()
            : m_listHead(nullptr)
        {
        }

        INLINE void registerStaticResource(IStaticResource* resource)
        {
            auto lock = CreateLock(m_lock);

            if (m_listHead)
                m_listHead->m_prev = resource;
            resource->m_next = m_listHead;
            m_listHead = resource;
        }

        INLINE void unregisterStaticResource(IStaticResource* resource)
        {
            auto lock = CreateLock(m_lock);

            if (resource->m_prev != nullptr)
                resource->m_prev->m_next = resource->m_next;
            else
                m_listHead = resource->m_next;

            if (resource->m_next != nullptr)
                resource->m_next->m_prev = resource->m_prev;
        }

        INLINE void collect(Array<IStaticResource*>& outResources)
        {
            auto lock = CreateLock(m_lock);

            for (auto cur  = m_listHead; cur; cur = cur->m_next)
                outResources.pushBack(cur);
        }

    private:
        SpinLock m_lock;
        IStaticResource* m_listHead;

        virtual void deinit() override
        {
        }
    };
} // prv

IStaticResource::IStaticResource(const char* path)
    : m_path(path)
    , m_next(nullptr)
    , m_prev(nullptr)
{
    ASSERT_EX(m_path != nullptr && *m_path != 0, "Static resource must always have valid path");
    prv::StaticResourceRegistry::GetInstance().registerStaticResource(this);
}

IStaticResource::~IStaticResource()
{
    prv::StaticResourceRegistry::GetInstance().unregisterStaticResource(this);
}

BaseReference IStaticResource::load() const
{
    const auto loaded = LoadResource(m_path);
    return BaseReference(m_path, loaded);
}

void IStaticResource::CollectAllResources(Array<IStaticResource*>& outResources)
{
    prv::StaticResourceRegistry::GetInstance().collect(outResources);
}

END_BOOMER_NAMESPACE()
