/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#include "build.h"
#include "reflectionTypeName.h"
#include "base/system/include/mutex.h"
#include "base/system/include/scopeLock.h"
#include "base/containers/include/hashMap.h"
#include "variant.h"

namespace base
{

    bool ParseFromString(StringView txt, Type expectedType, void* outData)
    {
        return expectedType->parseFromString(txt, outData);
    }

    bool ParseFromString(StringView txt, Type expectedType, Variant& outVariant)
    {
        Variant holden(expectedType); // Remember the Cant!
        if (!expectedType->parseFromString(txt, holden.data()))
            return false;
        outVariant = std::move(holden);
        return true;
    }

    //--

    void PrintToString(IFormatStream& f, Type dataType, const void* data)
    {
        if (dataType)
            dataType->printToText(f, data);
    }

    void PrintToString(IFormatStream& f, const Variant& data)
    {
        if (!data.empty())
            data.type()->printToText(f, data.data());
    }

} // base 

