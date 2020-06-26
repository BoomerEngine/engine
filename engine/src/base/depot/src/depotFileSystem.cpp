/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: depot\filesystem #]
***/

#include "build.h"
#include "base/depot/include/depotFileSystem.h"

namespace base
{
    namespace depot
    {

        IFileSystem::~IFileSystem()
        {}

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IFileSystemProvider);
        RTTI_END_TYPE();

        IFileSystemProvider::~IFileSystemProvider()
        {}

        IFileSystemNotifier::~IFileSystemNotifier()
        {}

    } // depot
} // base
