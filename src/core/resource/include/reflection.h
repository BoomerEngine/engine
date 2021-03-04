/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#pragma once

#include "reference.h"
#include "asyncReference.h"

BEGIN_BOOMER_NAMESPACE()

namespace resolve
{

    // type name resolve for strong handles
    template<typename T>
    struct TypeName<ResourceRef<T>>
    {
        static StringID GetTypeName()
        {
            static auto cachedTypeName = FormatRefTypeName(TypeName<T>::GetTypeName());
            return cachedTypeName;
        }
    };

    // type name resolve for strong handles
    template<typename T>
    struct TypeName<ResourceAsyncRef<T>>
    {
        static StringID GetTypeName()
        {
            static auto cachedTypeName = FormatAsyncRefTypeName(TypeName<T>::GetTypeName());
            return cachedTypeName;
        }
    };

} // resolve

END_BOOMER_NAMESPACE()
