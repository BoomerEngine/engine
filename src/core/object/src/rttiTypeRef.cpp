/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\typeref #]
***/

#include "build.h"
#include "rttiTypeRef.h"
#include "rttiType.h"
#include "rttiTypeSystem.h"
#include "rttiClassRef.h"
#include "rttiClassType.h"
#include "rttiClassRefType.h"
#include "rttiArrayType.h"
#include "rttiHandleType.h"

#include "core/containers/include/stringBuf.h"

BEGIN_BOOMER_NAMESPACE()

//--

ClassType Type::toClass() const
{
    return isClass() ? ClassType(static_cast<const rtti::IClassType*>(m_type)) : ClassType();
}

Type Type::innerType() const
{
    switch (metaType())
    {
    case rtti::MetaType::Array:
        return static_cast<const rtti::IArrayType*>(m_type)->innerType();
    case rtti::MetaType::ClassRef:
        return static_cast<const rtti::ClassRefType*>(m_type)->baseClass();
    case rtti::MetaType::WeakHandle:
    case rtti::MetaType::StrongHandle:
        return static_cast<const rtti::IHandleType*>(m_type)->pointedClass();
    }

    return nullptr;
}

ClassType Type::referencedClass() const
{
    switch (metaType())
    {
    case rtti::MetaType::ClassRef:
        return static_cast<const rtti::ClassRefType*>(m_type)->baseClass();
    case rtti::MetaType::WeakHandle:
    case rtti::MetaType::StrongHandle:
        return static_cast<const rtti::IHandleType*>(m_type)->pointedClass();
    }

    return nullptr;
}

//--

void Type::print(IFormatStream& f) const
{
    if (m_type == nullptr)
    {
        f << "null";
    }
    else
    {
        f << m_type->name();

        switch (m_type->metaType())
        {
            case rtti::MetaType::Enum: f.append(" (enum)"); break;
            case rtti::MetaType::Bitfield: f.append(" (bitfield)"); break;
            case rtti::MetaType::Class: f.append(" (class)"); break;
        }        
    }
}

uint32_t Type::CalcHash(const Type& type)
{
    return StringID::CalcHash(type ? type->name() : StringID());
}

uint32_t Type::CalcHash(const StringID& type)
{
    return StringID::CalcHash(type);
}

//--

const Type& Type::EMPTY()
{
    static Type theEmptyTypeRef;
    return theEmptyTypeRef;
}

//--
    
END_BOOMER_NAMESPACE()
