/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "meshPreviewPanel.h"

#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace ed
{

    //--
     
    RTTI_BEGIN_TYPE_CLASS(MeshPreviewPanel);
    RTTI_END_TYPE();

    MeshPreviewPanel::MeshPreviewPanel()
    {}

    MeshPreviewPanel::~MeshPreviewPanel()
    {}

    void MeshPreviewPanel::previewMesh(const rendering::MeshPtr& ptr)
    {
        if (m_mesh != ptr)
        {
            destroyPreviewElements();
            m_mesh = ptr;
            createPreviewElements();
        }
    }

    void MeshPreviewPanel::previewSettings(const MeshPreviewPanelSettings& settings)
    {
        destroyPreviewElements();
        m_previewSettings = settings;
        createPreviewElements();
    }

    void MeshPreviewPanel::previewMaterial(base::StringID name, rendering::MaterialPtr data)
    {
        if (m_previewMaterials[name] != data)
        {
            destroyPreviewElements();
            if (data)
                m_previewMaterials[name] = data;
            else
                m_previewMaterials.remove(name);
            createPreviewElements();
        }
    }

    void MeshPreviewPanel::handleRender(rendering::scene::FrameParams& frame)
    {
        TBaseClass::handleRender(frame);

        if (m_mesh)
        {
            if (m_previewSettings.showBounds)
            {
                rendering::scene::DebugLineDrawer lines(frame.geometry.solid);
                lines.color(base::Color::YELLOW);
                lines.box(m_mesh->bounds().box);
            }
        }
    }

    void MeshPreviewPanel::destroyPreviewElements()
    {
        if (m_mainProxy)
        {
            renderingScene()->proxyDestroy(m_mainProxy);
            m_mainProxy.reset();
        }
    }

    void MeshPreviewPanel::createPreviewElements()
    {
        if (m_mesh)
        {
            rendering::scene::ProxyMeshDesc desc;
            desc.mesh = m_mesh;
            desc.forcedLodLevel = m_previewSettings.forceLod;

            desc.materialOverrides.reserve(m_previewMaterials.size());
            m_previewMaterials.forEach([&desc](const base::StringID& name, const rendering::MaterialPtr& material)
                {
                    if (material)
                        desc.materialOverrides[name] = material;
                });

            m_mainProxy = renderingScene()->proxyCreate(desc);
        }
    }

    //--
    
} // ed
