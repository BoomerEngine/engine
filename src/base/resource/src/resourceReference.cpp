/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#include "build.h"

#include "resource.h"
#include "resourceReference.h"
#include "resourceLoader.h"

BEGIN_BOOMER_NAMESPACE(base::res)

//---

BaseReference::BaseReference()
{
}

BaseReference::BaseReference(std::nullptr_t)
{}

BaseReference::BaseReference(const BaseReference& other)
    : m_handle(other.m_handle)
    , m_path(other.m_path)
{
}

BaseReference::BaseReference(const ResourcePath& path)
    : m_path(path)
{}

BaseReference::BaseReference(const ResourcePtr& ptr)
{
    if (ptr)
    {
        DEBUG_CHECK_RETURN_EX(ptr->path(), "Unable to setup resource reference with non file based resource");

        m_handle = ptr;
        m_path = ptr->path();
    }
}

BaseReference::BaseReference(BaseReference&& other)
    : m_handle(std::move(other.m_handle))
    , m_path(std::move(other.m_path))
{
    other.reset();
}

BaseReference::~BaseReference()
{
}

BaseReference& BaseReference::operator=(const BaseReference& other)
{
    if (this != &other)
    {
        m_handle = other.m_handle;
        m_path = other.m_path;
    }

    return *this;
}

BaseReference& BaseReference::operator=(BaseReference&& other)
{
    if (this != &other)
    {
        m_handle = std::move(other.m_handle);
        m_path = std::move(other.m_path);
        other.reset();
    }

    return *this;
}

void BaseReference::reset()
{
    m_handle.reset();
    m_path = ResourcePath();
}

void BaseReference::set(const ResourceHandle& ptr)
{
    if (ptr)
    {
        DEBUG_CHECK_RETURN_EX(ptr->path(), "Unable to setup resource reference with non file based resource");
        m_handle = ptr;
        m_path = ptr->path();
    }
    else
    {
        m_handle.reset();
        m_path = ResourcePath();
    }
}

ResourceHandle BaseReference::load() const
{
    if (m_handle)
    {
        DEBUG_CHECK_EX(m_handle->path() == m_path, "Mismatched resource path");
        return m_handle;
    }

    if (!m_path)
        return nullptr;

    auto loaded = LoadResource(m_path);
    if (loaded)
        const_cast<BaseReference*>(this)->m_handle = loaded;

    return loaded;
}

const ResourceHandle& BaseReference::EMPTY_HANDLE()
{
    static ResourceHandle theEmptyHandle;
    return theEmptyHandle;
}

bool BaseReference::operator==(const BaseReference& other) const
{
    return m_path == other.m_path;
}

bool BaseReference::operator!=(const BaseReference& other) const
{
    return !operator==(other);
}

void BaseReference::print(IFormatStream& f) const
{
    f << m_path;
}

//--

END_BOOMER_NAMESPACE(base::res)
