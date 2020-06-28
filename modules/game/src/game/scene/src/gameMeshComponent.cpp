/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity\components #]
*
***/

#include "build.h"
#include "gameMeshComponent.h"
#include "worldRenderingSystem.h"
#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/scene/include/renderingScene.h"

namespace game
{
    //--

    RTTI_BEGIN_TYPE_CLASS(MeshComponent);
        RTTI_PROPERTY(m_mesh).editable();
        RTTI_PROPERTY(m_castShadows).editable();
        RTTI_PROPERTY(m_receiveShadows).editable();
        RTTI_PROPERTY(m_forceDetailLevel).editable();
    RTTI_END_TYPE();

    MeshComponent::MeshComponent()
    {}

    MeshComponent::MeshComponent(const rendering::MeshPtr& mesh)
        : m_mesh(mesh)
    {}

    MeshComponent::~MeshComponent()
    {}

    void MeshComponent::castingShadows(bool flag)
    {
        m_castShadows = flag;
    }

    void MeshComponent::receivingShadows(bool flag)
    {
        m_receiveShadows = flag;
    }

    void MeshComponent::forceDetailLevel(char level)
    {
        m_forceDetailLevel = level;
    }

    void MeshComponent::handleAttach(World* world)
    {
        TBaseClass::handleAttach(world);
        createRenderingProxy();
    }

    void MeshComponent::handleDetach(World* world)
    {
        TBaseClass::handleDetach(world);
        destroyRenderingProxy();
    }

    void MeshComponent::createRenderingProxy()
    {
        if (auto* scene = system<WorldRenderingSystem>())
        {
            if (m_mesh)
            {
                rendering::scene::ProxyMeshDesc desc;
                desc.forcedLodLevel = m_forceDetailLevel;
                desc.castsShadows = m_castShadows;
                desc.receiveShadows = m_receiveShadows;
                desc.localToScene = localToWorld();
                desc.mesh = m_mesh;

                m_proxy = scene->scene()->proxyCreate(desc);
            }
        }
    }

    void MeshComponent::destroyRenderingProxy()
    {
        if (m_proxy)
        {
            if (auto* scene = system<WorldRenderingSystem>())
                scene->scene()->proxyDestroy(m_proxy);

            m_proxy.reset();
        }
    }

    void MeshComponent::handleTransformUpdate(const base::AbsoluteTransform& parentToWorld, Component* transformParentComponent)
    {
        TBaseClass::handleTransformUpdate(parentToWorld, transformParentComponent);

        if (m_proxy)
        {
            if (auto* scene = system<WorldRenderingSystem>())
            {
                rendering::scene::CommandMoveProxy cmd;
                cmd.localToScene = localToWorld();
                scene->scene()->proxyCommand(m_proxy, cmd);
            }
        }
    }

    void MeshComponent::mesh(const rendering::MeshPtr& mesh)
    {
        if (m_mesh != mesh)
        {
            if (attached())
            {
                destroyRenderingProxy();
                m_mesh = mesh;
                createRenderingProxy();
            }
            else
            {
                m_mesh = mesh;
            }
        }
    }

    //--
        
} // game