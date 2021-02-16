/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#include "build.h"
#include "meshEntity.h"

#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/world/include/renderingSystem.h"
#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneObjects.h"

namespace game
{

    //--

    RTTI_BEGIN_TYPE_CLASS(MeshEntity);
        RTTI_CATEGORY("Mesh");
        RTTI_PROPERTY(m_castShadows).overriddable().editable("Should mesh cast shadows");
        RTTI_PROPERTY(m_receiveShadows).overriddable().editable("Should mesh receive shadows");
        RTTI_PROPERTY(m_forceDetailLevel).overriddable().editable("Force single LOD of the mesh");
        RTTI_PROPERTY(m_forceDistance).overriddable().editable("Force visibility distance for mesh");
        RTTI_CATEGORY("Effects");
        RTTI_PROPERTY(m_color).overriddable().editable("General effect color");
        RTTI_PROPERTY(m_colorEx).overriddable().editable("Secondary effect color");

        RTTI_PROPERTY(m_mesh);
    RTTI_END_TYPE();

    MeshEntity::MeshEntity()
    {}

    MeshEntity::~MeshEntity()
    {}

    void MeshEntity::castingShadows(bool flag)
    {
        if (m_castShadows != flag)
        {
            m_castShadows = flag;
            // TODO: refresh proxy via command
        }
    }

    void MeshEntity::receivingShadows(bool flag)
    {
        if (m_receiveShadows != flag)
        {
            m_receiveShadows = flag;
            // TODO: refresh proxy via command
        }
    }

    void MeshEntity::forceDetailLevel(char level)
    {
        if (m_forceDetailLevel != level)
        {
            m_forceDetailLevel = level;
            recreateRenderingProxy(); // TODO: maybe that's to harsh ?
        }
    }

    void MeshEntity::forceDistance(float distance)
    {
        const auto dist = (uint16_t)std::clamp<float>(distance, 0, 32767.0f); // leave higher bits for something in the future
        if (dist != m_forceDistance)
        {
            m_forceDistance = dist;
            recreateRenderingProxy(); // TODO: maybe that's to harsh ?
        }
    }

    void MeshEntity::color(base::Color color)
    {
        if (m_color != color)
        {
            m_color = color;
            recreateRenderingProxy(); // TODO: maybe that's to harsh ?
        }
    }

    void MeshEntity::colorEx(base::Color color)
    {
        if (m_colorEx != color)
        {
            m_colorEx = color;
            recreateRenderingProxy(); // TODO: maybe that's to harsh ?
        }
    }

    rendering::scene::ObjectProxyPtr MeshEntity::handleCreateProxy(rendering::scene::Scene* scene) const
    {
        if (const auto meshData = m_mesh.load())
        {
			rendering::scene::ObjectProxyMesh::Setup setup;
			setup.forcedLodLevel = m_forceDetailLevel;
			setup.mesh = meshData;

			if (auto proxy = rendering::scene::ObjectProxyMesh::Compile(setup))
			{
				if (m_castShadows)
					proxy->m_flags |= rendering::scene::ObjectProxyFlagBit::CastShadows;
				if (m_receiveShadows)
					proxy->m_flags |= rendering::scene::ObjectProxyFlagBit::ReceivesShadows;
				if (selected())
					proxy->m_flags |= rendering::scene::ObjectProxyFlagBit::Selected;

				proxy->m_color = m_color;
				proxy->m_colorEx = m_colorEx;
				proxy->m_localToWorld = localToWorld();
                proxy->m_selectable = rendering::scene::Selectable(selectionOwner());

				return proxy;
			}
        }

		return nullptr;
    }

    base::Box MeshEntity::calcBounds() const
    {
        if (auto mesh = m_mesh.load())
        {
            if (!mesh->bounds().empty())
            {
                return localToWorld().transformBox(mesh->bounds());
            }
        }

        return TBaseClass::calcBounds();
    }

    void MeshEntity::mesh(const rendering::MeshRef& mesh)
    {
        if (m_mesh != mesh)
        {
            m_mesh = mesh;
            recreateRenderingProxy();
        }
    }

    //--

    static base::res::StaticResource<rendering::Mesh> resDefaultMesh("/engine/meshes/box.v4mesh", true);

    void MeshEntity::queryTemplateProperties(base::ITemplatePropertyBuilder& outTemplateProperties) const
    {
        TBaseClass::queryTemplateProperties(outTemplateProperties);
        outTemplateProperties.prop<rendering::MeshAsyncRef>("Mesh"_id, "mesh"_id, rendering::MeshAsyncRef(), base::rtti::PropertyEditorData().comment("Mesh to render").primaryResource());
    }

    bool MeshEntity::initializeFromTemplateProperties(const base::ITemplatePropertyValueContainer& templateProperties)
    {
        if (!TBaseClass::initializeFromTemplateProperties(templateProperties))
            return false;

        m_mesh = templateProperties.compileValueOrDefault<rendering::MeshRef>("mesh"_id);
        if (!m_mesh)
            m_mesh = resDefaultMesh.loadAndGetAsRef();

        return true;
    }

    //--
        
} // game