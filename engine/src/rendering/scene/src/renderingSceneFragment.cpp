/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\fragments #]
***/

#include "build.h"
#include "renderingSceneFragment.h"

namespace rendering
{
    namespace scene
    {
        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IFragmentHandler);
        RTTI_END_TYPE();

        IFragmentHandler::IFragmentHandler(FragmentHandlerType type)
            : m_type(type)
        {}

        IFragmentHandler::~IFragmentHandler()
        {}

        void IFragmentHandler::handleInit(Scene* scene)
        {}

        void IFragmentHandler::handleSceneLock()
        {}

        void IFragmentHandler::handleSceneUnlock()
        {}

        void IFragmentHandler::handlePrepare(command::CommandWriter& cmd)
        {}

        //--

    } // scene
} // rendering
