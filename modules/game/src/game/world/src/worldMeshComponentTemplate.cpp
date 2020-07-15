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
#include "worldMeshComponentTemplate.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace game
{
    //--

    base::res::StaticResource<rendering::Mesh> resDefaultMesh("engine/meshes/cube.v4mesh");
    //--

    RTTI_BEGIN_TYPE_CLASS(MeshComponentTemplate);
        RTTI_CATEGORY("Rendering");
        RTTI_PROPERTY(m_mesh).editable("Mesh asset to render");
        RTTI_CATEGORY("Visibility");
        RTTI_PROPERTY(m_visibilityDistanceForced).editable();
        RTTI_PROPERTY(m_visibilityDistanceMultiplier).editable();
        RTTI_CATEGORY("Style");
        RTTI_PROPERTY(m_color).editable("Internal color, passed to material shaders to be used by various effects");
        RTTI_PROPERTY(m_colorEx).editable("Internal color, passed to material shaders to be used by various effects");
        RTTI_PROPERTY(m_castShadows).editable("Should the mesh cast shadows");
        RTTI_PROPERTY(m_receiveShadows).editable("Should the mesh receive shadows");
    RTTI_END_TYPE();

    MeshComponentTemplate::MeshComponentTemplate()
    {}

    ComponentPtr MeshComponentTemplate::createComponent() const
    {
        auto mc = base::CreateSharedPtr<MeshComponent>();

        // load specified mesh
        auto mesh = m_mesh.load().cast<rendering::Mesh>();
        if (!mesh)
            mesh = resDefaultMesh.loadAndGetAsRef();

        // set the common stuff
        // TODO: overrides
        mc->m_mesh = mesh;
        mc->m_castShadows = m_castShadows;
        mc->m_forceDetailLevel = m_forceDetailLevel;
        mc->m_receiveShadows = m_receiveShadows;
        mc->m_forceDetailLevel = m_forceDetailLevel;
        mc->m_color = m_color;
        mc->m_colorEx = m_colorEx;

        return mc;
    }

    void MeshComponentTemplate::applyOverrides(Component* c) const
    {
        if (auto mc = base::rtti_cast<MeshComponent>(c))
        {
            // TODO: apply overrides, this will be tedious code
        }
    }

    //--
        
} // game