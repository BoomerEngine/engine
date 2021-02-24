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
#include "resourceAsyncReference.h"
#include "resourceLoader.h"

BEGIN_BOOMER_NAMESPACE(base::res)

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

void BaseAsyncReference::loadAsync(const std::function<void(const BaseReference&)>& onLoadedFunc) const
{
    if (m_path)
    {
        // use general "global" loading function
        LoadResourceAsync(m_path, onLoadedFunc);
    }
    else
    {
        // empty ref, still we need to call the handler
        onLoadedFunc(BaseReference());
    }
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

END_BOOMER_NAMESPACE(base::res)
