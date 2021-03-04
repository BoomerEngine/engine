/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#include "build.h"

#include "resource.h"
#include "reference.h"
#include "loader.h"

BEGIN_BOOMER_NAMESPACE()

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

BaseReference::BaseReference(const ResourcePath& key, IResource* ptr)
    : m_path(key)
    , m_handle(AddRef(ptr))
{
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

const ResourcePtr& BaseReference::EMPTY_HANDLE()
{
    static ResourcePtr theEmptyHandle;
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

END_BOOMER_NAMESPACE()
