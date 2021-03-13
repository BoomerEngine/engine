/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#include "build.h"
#include "tags.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(ResourceDataVersionMetadata);
RTTI_END_TYPE();

ResourceDataVersionMetadata::ResourceDataVersionMetadata()
    : m_version(0)
{}

//---

RTTI_BEGIN_TYPE_CLASS(ResourceDescriptionMetadata);
RTTI_END_TYPE();

ResourceDescriptionMetadata::ResourceDescriptionMetadata()
    : m_desc("")
{}

//--

RTTI_BEGIN_TYPE_CLASS(ResourceImportedClassMetadata);
RTTI_END_TYPE();

ResourceImportedClassMetadata::ResourceImportedClassMetadata()
{}

//--

RTTI_BEGIN_TYPE_CLASS(ResourceSourceFormatMetadata);
RTTI_END_TYPE();

ResourceSourceFormatMetadata::ResourceSourceFormatMetadata()
{}

//--

RTTI_BEGIN_TYPE_CLASS(ResourceCookerVersionMetadata);
RTTI_END_TYPE();

ResourceCookerVersionMetadata::ResourceCookerVersionMetadata()
    : m_version(0)
{}

//--

END_BOOMER_NAMESPACE()
