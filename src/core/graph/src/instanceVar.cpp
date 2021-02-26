/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: instancing #]
***/

#include "build.h"
#include "instanceVar.h"

BEGIN_BOOMER_NAMESPACE()

InstanceVarBase::InstanceVarBase(Type type)
    : m_type(type)
    , m_offset(INVALID_OFFSET) // invalid until assigned in the builder
{}

InstanceVarBase::~InstanceVarBase()
{}

void InstanceVarBase::reset()
{
    m_offset = INVALID_OFFSET;
}

void InstanceVarBase::alias(const InstanceVarBase& other)
{
    ASSERT_EX(&other != this, "Cannot alias instance variable with itself");
    ASSERT_EX(m_type == other.m_type, "Cannot alias values of different type");
    ASSERT_EX(other.m_offset != INVALID_OFFSET, "Cannot alias with an invalid value");
    m_offset = other.m_offset;
}

END_BOOMER_NAMESPACE()
