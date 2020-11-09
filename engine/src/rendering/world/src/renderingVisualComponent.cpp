/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: components #]
*
***/

#include "build.h"
#include "renderingVisualComponent.h"
#include "renderingSystem.h"

#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneCommand.h"

namespace rendering
{

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IVisualComponent);
    RTTI_END_TYPE();

    IVisualComponent::IVisualComponent()
    {}

    void IVisualComponent::bindSelectable(const scene::Selectable& data)
    {
        if (m_selectable != data)
        {
            m_selectable = data;
            recreateRenderingProxy();
        }
    }
    
    void IVisualComponent::handleAttach(base::world::World* world)
    {
        TBaseClass::handleAttach(world);
        createRenderingProxy();
    }

    void IVisualComponent::handleDetach(base::world::World* world)
    {
        TBaseClass::handleDetach(world);
        destroyRenderingProxy();
    }

    void IVisualComponent::recreateRenderingProxy()
    {
        if (auto* scene = system<RenderingSystem>())
        {
            destroyRenderingProxy();
            createRenderingProxy();
        }
    }
    
    void IVisualComponent::createRenderingProxy()
    {
        DEBUG_CHECK_RETURN(!m_proxy);

        if (auto* scene = system<RenderingSystem>())
            m_proxy = handleCreateProxy(scene->scene());
    }

    void IVisualComponent::destroyRenderingProxy()
    {
        if (m_proxy)
        {
            if (auto* scene = system<RenderingSystem>())
                scene->scene()->proxyDestroy(m_proxy);

            m_proxy.reset();
        }
    }

    void IVisualComponent::handleTransformUpdate(const Component* source, const base::AbsoluteTransform& parentTransform, const base::Matrix& parentToWorld)
    {
        TBaseClass::handleTransformUpdate(source, parentTransform, parentToWorld);

        if (m_proxy)
        {
            if (auto* scene = system<RenderingSystem>())
            {
                rendering::scene::CommandMoveProxy cmd;
                cmd.localToScene = localToWorld();
                scene->scene()->proxyCommand(m_proxy, cmd);
            }
        }
    }

    void IVisualComponent::handleSelectionHighlightChanged()
    {
        TBaseClass::handleSelectionHighlightChanged();

        if (m_proxy)
        {
            if (auto* scene = system<RenderingSystem>())
            {
                rendering::scene::CommandEffectSelectionHighlight cmd;
                cmd.flag = selected();
                scene->scene()->proxyCommand(m_proxy, cmd);
            }
        }
    }

    //--
        
} // rendering