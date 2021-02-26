/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#pragma once

#include "core/containers/include/stringID.h"
#include "core/containers/include/inplaceArray.h"

#include "core/object/include/rttiTypeSystem.h"
#include "core/object/include/rttiType.h"
#include "core/object/include/rttiClassRef.h"

BEGIN_BOOMER_NAMESPACE_EX(reflection)

namespace resolve
{
    // given a type extract the RTTI type name for it
    // NOTE: this may compile but may return empty name
    template<typename T>
    struct TypeName
    {
        static StringID GetTypeName()
        {
            static auto cachedTypeName = RTTI::GetInstance().mapNativeTypeName(typeid(typename std::remove_cv<T>::type).hash_code());
            DEBUG_CHECK_EX(cachedTypeName, TempString("Expected native type was not recognized: '{}', hash {}", typeid(typename std::remove_cv<T>::type).name(), typeid(typename std::remove_cv<T>::type).hash_code()));
            return cachedTypeName;
        }
    };

    // type name resolve for strong handles
    template<typename T>
    struct TypeName<RefPtr<T>>
    {
        static StringID GetTypeName()
        {
            static auto cachedTypeName = rtti::FormatStrongHandleTypeName(TypeName<T>::GetTypeName());
            return cachedTypeName;
        }
    };

    // type name resolve for weak handles
    template<typename T>
    struct TypeName<RefWeakPtr<T>>
    {
        static StringID GetTypeName()
        {
            static auto cachedTypeName = rtti::FormatWeakHandleTypeName(TypeName<T>::GetTypeName());
            return cachedTypeName;
        }
    };

    // type name resolve for dynamic arrays
    template<typename T>
    struct TypeName<Array<T>>
    {
        static StringID GetTypeName()
        {
            static auto cachedTypeName = rtti::FormatDynamicArrayTypeName(TypeName<T>::GetTypeName());
            return cachedTypeName;
        }
    };

	// type name resolve for dynamic arrays
	template<typename T, uint32_t N>
	struct TypeName<InplaceArray<T, N>>
	{
		static StringID GetTypeName()
		{
			static auto cachedTypeName = rtti::FormatDynamicArrayTypeName(TypeName<T>::GetTypeName());
			return cachedTypeName;
		}
	};

    // type name resolve for native arrays
    template<typename T, uint32_t MAX_SIZE>
    struct TypeName<T[MAX_SIZE]>
    {
        static StringID GetTypeName()
        {
            static auto cachedTypeName = rtti::FormatNativeArrayTypeName(TypeName<T>::GetTypeName(), MAX_SIZE);
            return cachedTypeName;
        }
    };

    // type name resolve for class references arrays
    template<typename T>
    struct TypeName<SpecificClassType<T>>
    {
        static StringID GetTypeName()
        {
            static auto cachedTypeName = rtti::FormatClassRefTypeName(TypeName<T>::GetTypeName());
            return cachedTypeName;
        }
    };

} // resolve

template< typename T >
INLINE static StringID GetTypeName()
{
    return resolve::TypeName<T>::GetTypeName();
}

template< typename T >
INLINE static Type GetTypeObject()
{
    static auto cachedTypeObject = RTTI::GetInstance().findType(GetTypeName<T>());
    DEBUG_CHECK_EX(cachedTypeObject != nullptr, TempString("Native type '{}' not recognized by RTTI - was it registered?", typeid(T).name()));
    return cachedTypeObject;
}

template< typename T >
INLINE static SpecificClassType<T> ClassID()
{
    static auto cachedTypeObject = RTTI::GetInstance().findClass(GetTypeName<T>());
    return *(SpecificClassType<T>*) &cachedTypeObject;
}

template< typename T >
INLINE const char* GetEnumValueName(T enumValue)
{
    auto enumType = GetTypeObject<T>();
    if (!enumType)
        return "InvalidType";
    else if (enumType->metaType() != rtti::MetaType::Enum)
        return "NotAnEnum";
    else
        return rtti::GetEnumValueName(static_cast<const rtti::EnumType*>(enumType.ptr()), (int64_t)enumValue);
}

template< typename T >
INLINE bool GetEnumNameValue(StringID name, T& outEnumValue)
{
    auto enumType  = GetTypeObject<T>();
    if (!enumType || enumType->metaType() != rtti::MetaType::Enum)
        return false;

    int64_t value = 0;
    if (!rtti::GetEnumNameValue(static_cast<const rtti::EnumType*>(enumType.ptr()), name, value))
        return false;

    outEnumValue = (T)value;
    return true;
}

END_BOOMER_NAMESPACE_EX(reflection)
