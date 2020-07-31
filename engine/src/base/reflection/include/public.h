/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

// Glue code
#include "base_reflection_glue.inl"

// Reflection builders are public as they are really used everywhere
#include "reflectionMacros.h"

namespace base
{

    //--

    class Variant;
    class VariantTable;

    //--

    template<typename T>
    struct PrintableConverter<T, typename std::enable_if<std::is_enum<T>::value>::type >
    {
        static void Print(IFormatStream& s, const T& val)
        {
            auto type = base::reflection::GetTypeObject<T>();
            if (type && type->metaType() == rtti::MetaType::Enum)
            {
                s.append(base::reflection::GetEnumValueName(val));
            }
            else
            {
				s.append(base::TempString("(enum: {})", (int64_t)val));
            }
        }
    };

    //--

    // try to read data from a data view into a provided typed container
    template< typename T >
    INLINE bool ReadViewData(StringView<char> viewPath, const void* viewData, T& targetData)
    {
        return base::ReadViewData(viewPath, viewData, &targetData, GetTypeObject<T>());
    }

    // try to write data into a data view from a provided typed container
    template< typename T >
    INLINE bool WriteViewData(StringView<char> viewPath, void* viewData, const T& sourceData)
    {
        return base::WriteViewData(viewPath, viewData, &sourceData, GetTypeObject<T>());
    }

    //--

     // NOTE: the following functions are rtti-aware extension of a typical ToString/FromString

    // try to parse a data of given type from a string
    // NOTE: we try to parse all content provided, ie. we want cut just "5" from "5 cats" and think it's a number
    // NOTE: obviously the outData must point a memory that contains a valid (constructed) object of type
    extern BASE_REFLECTION_API bool ParseFromString(StringView<char> txt, Type xpectedType, void* outData);

    // try to parse a data of given type from a string into a variant data holder
    extern BASE_REFLECTION_API bool ParseFromString(StringView<char> txt, Type expectedType, Variant& outVariant);

    // try to parse a data of given type from a string
    template< typename T >
    INLINE bool ParseFromString(StringView<char> txt, T& outData)
    {
        return ParseFromString(txt, reflection::GetTypeObject<T>(), &outData);
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
    extern BASE_REFLECTION_API void PrintToString(IFormatStream& f, Type dataType, const void* data);

    // print data to a general string representation that will be parsable back via ParseFromString
    // called from Variant print() itself so this is the main work horse -> redirects to the type, data version
    extern BASE_REFLECTION_API void PrintToString(IFormatStream& f, const Variant& data);

    // prints a general type T (that must be known to RTTI)
    template< typename T >
    INLINE void PrintToString(IFormatStream& f, const T& data)
    {
        return PrintToString(f, reflection::GetTypeObject<T>(), &data);
    }

    //--

} // base

// TODO: change names to avoid potential conflicts
#define TYPE_OF(x) base::reflection::GetTypeObject<decltype(x)>()
#define TYPE_NAME_OF(x) base::reflection::GetTypeName<decltype(x)>()

#include "propertyDecorators.h"
#include "variant.h"
