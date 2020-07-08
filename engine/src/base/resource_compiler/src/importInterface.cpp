/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importInterface.h"

namespace base
{
    namespace res
    {

        //--

        IResourceImporterInterface::~IResourceImporterInterface()
        {}

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceImporter);
        RTTI_END_TYPE();

        IResourceImporter::~IResourceImporter()
        {}

        //--

    } // res
} // base
