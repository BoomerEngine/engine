/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\metadata #]
***/

#include "build.h"
#include "resourceSerializationMetadata.h"
#include "base/object/include/serializationLoader.h"
#include "base/object/include/serializationSaver.h"

namespace base
{

    ///---

    RTTI_BEGIN_TYPE_CLASS(SerializationLoaderMetadata);
    RTTI_END_TYPE();

    SerializationLoaderMetadata::SerializationLoaderMetadata()
    {}

    SerializationLoaderMetadata::~SerializationLoaderMetadata()
    {}

    RefPtr<stream::ILoader> SerializationLoaderMetadata::createLoader() const
    {
        return m_createLoader();
    }

    ///---

    RTTI_BEGIN_TYPE_CLASS(SerializationSaverMetadata);
    RTTI_END_TYPE();

    SerializationSaverMetadata::SerializationSaverMetadata()
    {}

    SerializationSaverMetadata::~SerializationSaverMetadata()
    {}

    RefPtr<stream::ISaver> SerializationSaverMetadata::createSaver() const
    {
        return m_createSaver();
    }

    ///----
}
