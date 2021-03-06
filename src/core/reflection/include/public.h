/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

// Glue code
#include "core_reflection_glue.inl"

// Reflection builders are public as they are really used everywhere
#include "reflectionMacros.h"

BEGIN_BOOMER_NAMESPACE()

//--

class Variant;
class VariantTable;

//--

template<typename T>
struct PrintableConverter<T, typename std::enable_if<std::is_enum<T>::value>::type >
{
    static void Print(IFormatStream& s, const T& val)
    {
        auto type = GetTypeObject<T>();
        if (type && type->metaType() == MetaType::Enum)
        {
            s.append(GetEnumValueName(val));
        }
        else
        {
			s.append(TempString("(enum: {})", (int64_t)val));
        }
    }
};

//--

// try to read data from a data view into a provided typed container
template< typename T >
INLINE bool ReadViewData(StringView viewPath, const void* viewData, T& targetData)
{
    return ReadViewData(viewPath, viewData, &targetData, GetTypeObject<T>());
}

// try to write data into a data view from a provided typed container
template< typename T >
INLINE bool WriteViewData(StringView viewPath, void* viewData, const T& sourceData)
{
    return WriteViewData(viewPath, viewData, &sourceData, GetTypeObject<T>());
}

//--

    // NOTE: the following functions are rtti-aware extension of a typical ToString/FromString

// try to parse a data of given type from a string
// NOTE: we try to parse all content provided, ie. we want cut just "5" from "5 cats" and think it's a number
// NOTE: obviously the outData must point a memory that contains a valid (constructed) object of type
extern CORE_REFLECTION_API bool ParseFromString(StringView txt, Type xpectedType, void* outData);

// try to parse a data of given type from a string into a variant data holder
extern CORE_REFLECTION_API bool ParseFromString(StringView txt, Type expectedType, Variant& outVariant);

// try to parse a data of given type from a string
template< typename T >
INLINE bool ParseFromString(StringView txt, T& outData)
{
    return ParseFromString(txt, GetTypeObject<T>(), &outData);
}

//--

// print data to a general string representation that will be parsable back via ParseFromString
// e.g.: 
//   5
//   NameFromStringID
//   AStringBuf
//   A longer string with spaces
//   null
//   Mesh$engine/meshes/box.obj
//   (x=1)(y=2)(z=3)
extern CORE_REFLECTION_API void PrintToString(IFormatStream& f, Type dataType, const void* data);

// print data to a general string representation that will be parsable back via ParseFromString
// called from Variant print() itself so this is the main work horse -> redirects to the type, data version
extern CORE_REFLECTION_API void PrintToString(IFormatStream& f, const Variant& data);

// prints a general type T (that must be known to RTTI)
template< typename T >
INLINE void PrintToString(IFormatStream& f, const T& data)
{
    return PrintToString(f, GetTypeObject<T>(), &data);
}

//--

END_BOOMER_NAMESPACE()


// TODO: change names to avoid potential conflicts
#define TYPE_OF(x) GetTypeObject<decltype(x)>()
#define TYPE_NAME_OF(x) GetTypeName<decltype(x)>()

#include "variant.h"
