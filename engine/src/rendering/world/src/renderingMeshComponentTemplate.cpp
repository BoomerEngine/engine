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
#include "renderingMeshComponentTemplate.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace rendering
{
    //--

    base::res::StaticResource<rendering::Mesh> resDefaultMesh("/engine/meshes/cube.v4mesh");
    //--

    RTTI_BEGIN_TYPE_CLASS(MeshComponentTemplate);
        RTTI_CATEGORY("Rendering");
        RTTI_PROPERTY(m_mesh).editable("Mesh asset to render").overriddable();
        RTTI_CATEGORY("Visibility");
        RTTI_PROPERTY(m_visibilityDistanceForced).editable().overriddable();
        RTTI_PROPERTY(m_visibilityDistanceMultiplier).editable().overriddable();
        RTTI_CATEGORY("Style");
        RTTI_PROPERTY(m_color).editable("Internal color, passed to material shaders to be used by various effects").overriddable();
        RTTI_PROPERTY(m_colorEx).editable("Internal color, passed to material shaders to be used by various effects").overriddable();
        RTTI_PROPERTY(m_castShadows).editable("Should the mesh cast shadows").overriddable();
        RTTI_PROPERTY(m_receiveShadows).editable("Should the mesh receive shadows").overriddable();
    RTTI_END_TYPE();

    MeshComponentTemplate::MeshComponentTemplate()
    {}

    void MeshComponentTemplate::mesh(const rendering::MeshAsyncRef& meshRef, bool makeOverride/* = true*/, bool callEvent/*= true*/)
    {
        if (m_mesh != meshRef)
        {
            m_mesh = meshRef;

            if (makeOverride)
                markPropertyOverride("mesh"_id);

            if (callEvent)
                onPropertyChanged("mesh");
        }
    }

    void MeshComponentTemplate::color(const base::Color& color, bool makeOverride /*= true*/, bool callEvent /*= true*/)
    {
        if (m_color != color)
        {
            m_color = color;

            if (makeOverride)
                markPropertyOverride("color"_id);

            if (callEvent)
                onPropertyChanged("color");
        }
    }

    void MeshComponentTemplate::colorEx(const base::Color& color, bool makeOverride /*= true*/, bool callEvent /*= true*/)
    {
        if (m_colorEx != color)
        {
            m_colorEx = color;

            if (makeOverride)
                markPropertyOverride("color"_id);

            if (callEvent)
                onPropertyChanged("color");
        }
    }

    //--

    base::world::ComponentPtr MeshComponentTemplate::createComponent() const
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

    //--
        
} // rendering