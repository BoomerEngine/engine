/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

template< class T >
INLINE bool IObject::is() const
{
    return is(T::GetStaticClass());
}

template< typename T >
INLINE bool IObject::is(SpecificClassType<T> objectClass) const
{
    return is(objectClass.ptr());
}

template< class T >
INLINE RefPtr<T> IObject::as() const
{
    return is<T>() ? sharedFromThisType<T>() : nullptr;
}

template< class T >
INLINE RefPtr<T> IObject::asChecked() const
{
    ASSERT_EX(is<T>(), "Object is not of expected type");
    return is<T>() ? sharedFromThisType<T>() : nullptr;
}

END_BOOMER_NAMESPACE(base)