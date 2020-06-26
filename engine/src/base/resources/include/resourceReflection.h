/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#pragma once

#include "resourceReference.h"

namespace base
{
    namespace reflection
    {
        namespace resolve
        {

            // type name resolve for strong handles
            template<typename T>
            struct TypeName<res::Ref<T>>
            {
                static StringID GetTypeName()
                {
                    static auto cachedTypeName = res::FormatRefTypeName(TypeName<T>::GetTypeName());
                    return cachedTypeName;
                }
            };

        } // resolve
    } // reflection
} // base
