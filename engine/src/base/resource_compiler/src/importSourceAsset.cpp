/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importSourceAsset.h"

namespace base
{
    namespace res
    {

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISourceAsset);
        RTTI_END_TYPE();

        ISourceAsset::ISourceAsset()
        {}

        ISourceAsset::~ISourceAsset()
        {}
        
        //--

    } // res
} // base
