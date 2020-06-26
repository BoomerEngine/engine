/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\mapping #]
***/

#pragma once

#include "base/containers/include/stringBuf.h"
#include "base/containers/include/stringID.h"

#include "rttiTypeRef.h"
#include "rttiClassRef.h"

namespace base
{
    class StringID;
    class IObject;

    namespace rtti
    {
        class IType;
        class Property;
    }

    namespace stream
    {
        class IBinaryWriter;

        class BASE_OBJECT_API IDataMapper
        {
        public:
            IDataMapper();
            virtual ~IDataMapper();

            virtual void mapName(StringID name, MappedNameIndex& outIndex) = 0;

            virtual void mapType(Type rttiType, MappedTypeIndex& outIndex) = 0;

            virtual void mapProperty(const rtti::Property* rttiProperty, MappedPropertyIndex& outIndex) = 0;

            virtual void mapPointer(const IObject* object, MappedObjectIndex& outIndex) = 0;

            virtual void mapResourceReference(StringView<char> path, ClassType resourceClass, bool async, MappedPathIndex& outIndex) = 0;

            virtual void mapBuffer(const Buffer& buffer, MappedBufferIndex& outIndex) = 0;
        };
    }

} // base
