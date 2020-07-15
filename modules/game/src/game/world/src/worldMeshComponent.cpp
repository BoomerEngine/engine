/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity\components\mesh #]
*
***/

#include "build.h"
#include "worldMeshComponent.h"
#include "worldRenderingSystem.h"

#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/scene/include/renderingScene.h"

namespace game
{
    //--

    /*class MeshComponentTemplate //: public ComponentTemplate
    {
        TemplateProperty<MeshAsyncRef> m_mesh;
        TemplateProperty<char> m_forceLodLevel;
        TemplateProperty<bool> m_castShadows;
        TemplateProperty<bool> m_receiveShadows;

        virtual void applyProperties(MeshComponent* comp)
        {
            TBaseClass::applyProperties(comp);
            comp->
        }

        virtual MeshComponentPtr compile()
        {
            
        }
    };*/



    //--

    RTTI_BEGIN_TYPE_CLASS(MeshComponent);
        RTTI_PROPERTY(m_mesh);
        RTTI_PROPERTY(m_castShadows);
        RTTI_PROPERTY(m_receiveShadows);
        RTTI_PROPERTY(m_forceDetailLevel);
        RTTI_PROPERTY(m_forcedDistance);
        RTTI_PROPERTY(m_color);
        RTTI_PROPERTY(m_colorEx);
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
        if (m_castShadows != flag)
        {
            m_castShadows = flag;
            // TODO: refresh proxy via command
        }
    }

    void MeshComponent::receivingShadows(bool flag)
    {
        if (m_receiveShadows != flag)
        {
            m_receiveShadows = flag;
            // TODO: refresh proxy via command
        }
    }

    void MeshComponent::forceDetailLevel(char level)
    {
        if (m_forceDetailLevel != level)
        {
            m_forceDetailLevel = level;
            recreateRenderingProxy(); // TODO: maybe that's to harsh ?
        }
    }

    void MeshComponent::forceDistance(float distance)
    {
        const auto dist = (uint16_t)std::clamp<float>(distance, 0, 32767.0f); // leave higher bits for something in the future
        m_forcedDistance = dist;
        recreateRenderingProxy(); // TODO: maybe that's to harsh ?
    }

    void MeshComponent::color(base::Color color)
    {
        if (m_color != color)
        {
            m_color = color;
        }
    }

    void MeshComponent::colorEx(base::Color color)
    {
        if (m_colorEx != color)
        {
            m_colorEx = color;
        }
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

    void MeshComponent::recreateRenderingProxy()
    {
        if (auto* scene = system<WorldRenderingSystem>())
        {
            destroyRenderingProxy();
            createRenderingProxy();
        }
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
                desc.color = m_color;
                desc.colorEx = m_colorEx;
                desc.mesh = m_mesh.acquire();

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

    void MeshComponent::handleTransformUpdate(const Component* source, const base::AbsoluteTransform& parentTransform, const base::Matrix& parentToWorld)
    {
        TBaseClass::handleTransformUpdate(source, parentTransform, parentToWorld);

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