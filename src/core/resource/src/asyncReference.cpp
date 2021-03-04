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
#include "asyncReference.h"
#include "loader.h"

BEGIN_BOOMER_NAMESPACE()

//---

BaseAsyncReference::BaseAsyncReference()
{
}

BaseAsyncReference::BaseAsyncReference(const BaseAsyncReference& other)
    : m_path(other.m_path)
{
}

BaseAsyncReference::BaseAsyncReference(const ResourcePath& path)
    : m_path(path)
{}

BaseAsyncReference::BaseAsyncReference(BaseAsyncReference&& other)
    : m_path(std::move(other.m_path))
{}

BaseAsyncReference::~BaseAsyncReference()
{
}

BaseAsyncReference& BaseAsyncReference::operator=(const BaseAsyncReference& other)
{
    if (this != &other)
    {
        m_path = other.m_path;
    }

    return *this;
}

BaseAsyncReference& BaseAsyncReference::operator=(BaseAsyncReference&& other)
{
    if (this != &other)
    {
        m_path = std::move(other.m_path);
    }

    return *this;
}

void BaseAsyncReference::reset()
{
    m_path = ResourcePath();
}

void BaseAsyncReference::set(const ResourcePath& path)
{
    m_path = path;
}

ResourcePtr BaseAsyncReference::load() const
{
    if (m_path)
        return LoadResource(m_path);
    return nullptr;
}

void BaseAsyncReference::loadAsync(const std::function<void(const ResourcePtr&)>& onLoadedFunc) const
{
    if (m_path)
        LoadResourceAsync(m_path, onLoadedFunc);
    else
        onLoadedFunc(nullptr);
}
        
const BaseAsyncReference& BaseAsyncReference::EMPTY_HANDLE()
{
    static BaseAsyncReference theEmptyHandle;
    return theEmptyHandle;
}

bool BaseAsyncReference::operator==(const BaseAsyncReference& other) const
{
    return m_path == other.m_path;
}

bool BaseAsyncReference::operator!=(const BaseAsyncReference& other) const
{
    return !operator==(other);
}

void BaseAsyncReference::print(IFormatStream& f) const
{
    if (m_path)
        f << m_path;
    else
        f << "null";
}

//--

END_BOOMER_NAMESPACE()
