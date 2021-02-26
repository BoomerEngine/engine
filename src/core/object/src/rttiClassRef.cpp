/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\typeref #]
***/

#include "build.h"
#include "rttiClassType.h"
#include "rttiClassRef.h"
#include "rttiTypeSystem.h"

BEGIN_BOOMER_NAMESPACE()

//---

bool ClassType::is(ClassType baseClassType) const
{
    if (!m_classType)
        return !baseClassType;
    return m_classType->is(baseClassType.ptr());
}

ClassType ClassType::cast(ClassType baseClass) const
{
    if (m_classType && m_classType->is(baseClass.ptr()))
        return *this;
    else
        return ClassType();
}
    
const ClassType& ClassType::EMPTY()
{
    static ClassType theNullClass;
    return theNullClass;
}

StringID ClassType::name() const
{
    if (m_classType)
        //return m_classType->shortName() ? m_classType->shortName() : m_classType->name();
        return m_classType->name();
    else
        return StringID();
}

void ClassType::print(IFormatStream& f) const
{
    if (m_classType)
        f << name().view().afterLastOrFull("::");
    else
        f << "null";
}

//---

/*   const IClassType* ClassRefBase::ResolveBaseClassTypeRef(uint64_t nativeTypeHash)
{
    auto resolvedType  = RTTI::GetInstance().mapNativeType(nativeTypeHash);
    ASSERT_EX(resolvedType && resolvedType->metaType() == MetaType::Class, "Unable to resolve class used in the ClassRef. Make sure the class is properly registered in the RTTI.");
    return static_cast<const IClassType*>(resolvedType);
}*/

//----

END_BOOMER_NAMESPACE()
