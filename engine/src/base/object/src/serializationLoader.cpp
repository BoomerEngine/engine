/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#include "build.h"
#include "serializationLoader.h"


namespace base
{
    namespace stream
    {

        //----

        LoadingContext::LoadingContext()
            : m_parent(nullptr)
            , m_loadImports(true)
            , m_resourceLoader(nullptr)
            , m_selectiveLoadingClass(nullptr)
        {
        }

        //----

        LoadingResult::LoadingResult()
        {
        }

        LoadingResult::~LoadingResult()
        {
        }

        //----  

        ILoader::ILoader()
        {
        }

        ILoader::~ILoader()
        {
        }       

        //----  

    } // stream
} // base