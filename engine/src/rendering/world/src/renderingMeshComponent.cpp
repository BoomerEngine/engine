/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: components\mesh #]
*
***/

#include "build.h"
#include "renderingMeshComponent.h"
#include "renderingSystem.h"

#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/scene/include/renderingScene.h"

namespace rendering
{

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

    scene::ProxyHandle MeshComponent::handleCreateProxy(scene::Scene* scene) const
    {
        if (const auto meshData = m_mesh.acquire())
        {
            rendering::scene::ProxyMeshDesc desc;
            desc.forcedLodLevel = m_forceDetailLevel;
            desc.castsShadows = m_castShadows;
            desc.receiveShadows = m_receiveShadows;
            desc.localToScene = localToWorld();
            desc.color = m_color;
            desc.colorEx = m_colorEx;
            desc.mesh = meshData;
            desc.meshBounds = meshData->bounds();
            desc.selected = selected();
            desc.selectable = selectable();
            return scene->proxyCreate(desc);
        }

        return scene::ProxyHandle();
    }

    base::Box MeshComponent::calcBounds() const
    {
        if (auto mesh = m_mesh.acquire())
        {
            if (!mesh->bounds().empty())
            {
                return localToWorld().transformBox(mesh->bounds());
            }
        }

        return TBaseClass::calcBounds();
    }

    void MeshComponent::mesh(const rendering::MeshPtr& mesh)
    {
        if (m_mesh != mesh)
        {
            m_mesh = mesh;
            recreateRenderingProxy();
        }
    }

    //--
        
} // rendering