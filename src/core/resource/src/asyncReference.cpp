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
    : m_id(other.m_id)
{
}

BaseAsyncReference::BaseAsyncReference(const ResourceID& path)
    : m_id(path)
{}

BaseAsyncReference::BaseAsyncReference(BaseAsyncReference&& other)
    : m_id(std::move(other.m_id))
{}

BaseAsyncReference::~BaseAsyncReference()
{
}

BaseAsyncReference& BaseAsyncReference::operator=(const BaseAsyncReference& other)
{
    if (this != &other)
    {
        m_id = other.m_id;
    }

    return *this;
}

BaseAsyncReference& BaseAsyncReference::operator=(BaseAsyncReference&& other)
{
    if (this != &other)
    {
        m_id = std::move(other.m_id);
    }

    return *this;
}

void BaseAsyncReference::reset()
{
    m_id = ResourceID();
}

BaseReference BaseAsyncReference::load() const
{
    const auto loaded = LoadResource(m_id);
    return BaseReference(m_id, loaded);
}

const BaseAsyncReference& BaseAsyncReference::EMPTY_HANDLE()
{
    static BaseAsyncReference theEmptyHandle;
    return theEmptyHandle;
}

bool BaseAsyncReference::operator==(const BaseAsyncReference& other) const
{
    return m_id == other.m_id;
}

bool BaseAsyncReference::operator!=(const BaseAsyncReference& other) const
{
    return !operator==(other);
}

void BaseAsyncReference::print(IFormatStream& f) const
{
    if (m_id)
        f << m_id;
    else
        f << "null";
}

//--

END_BOOMER_NAMESPACE()
