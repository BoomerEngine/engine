/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneCulling.h"
#include "renderingSceneProxy.h"

namespace rendering
{
    namespace scene
    {
        //----

        IProxy::IProxy(ProxyType type)
            : proxyType(type)
        {}

        IProxy::~IProxy()
        {
        }

        ///----

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IProxyHandler);
        RTTI_END_TYPE();

        IProxyHandler::IProxyHandler(ProxyType type)
            : m_type(type)
        {}

        IProxyHandler::~IProxyHandler()
        {}

        void IProxyHandler::handleInit(Scene* scene)
        {
            m_scene = scene;
        }

        void IProxyHandler::handleSceneLock()
        {
        }

        void IProxyHandler::handleSceneUnlock()
        {
        }

        void IProxyHandler::handlePrepare(command::CommandWriter& cmd)
        {
        }

        //----

    } // scene
} // rendering
