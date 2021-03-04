/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importFileSystem.h"

BEGIN_BOOMER_NAMESPACE()

//--

ISourceAssetFileSystem::~ISourceAssetFileSystem()
{}

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISourceAssetFileSystemFactory);
RTTI_END_TYPE();

ISourceAssetFileSystemFactory::~ISourceAssetFileSystemFactory()
{}

//--

END_BOOMER_NAMESPACE()
