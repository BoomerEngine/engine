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
#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/scene/include/renderingSceneObjects.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "rendering/scene/include/renderingFrameDebug.h"

namespace ed
{

    //--
     
    RTTI_BEGIN_TYPE_CLASS(MeshPreviewPanel);
    RTTI_END_TYPE();

    MeshPreviewPanel::MeshPreviewPanel()
    {
        m_previewSettings.showBounds = true;
        m_panelSettings.cameraForceOrbit = false;// true;
    }

    MeshPreviewPanel::~MeshPreviewPanel()
    {}

    void MeshPreviewPanel::configSave(const ui::ConfigBlock& block) const
    {
        TBaseClass::configSave(block);
    }

    void MeshPreviewPanel::configLoad(const ui::ConfigBlock& block)
    {
        TBaseClass::configLoad(block);
    }

    void MeshPreviewPanel::previewMesh(const rendering::MeshPtr& ptr)
    {
        if (m_mesh != ptr)
        {
            destroyPreviewElements();
            m_mesh = ptr;
            createPreviewElements();

            if (m_mesh && !m_mesh->bounds().empty())
            {
                if (m_lastBounds.empty() || !m_lastBounds.contains(m_mesh->bounds()))
                {
                    auto resetRotation = m_lastBounds.empty();
                    auto idealRotation = base::Angles(40, 30, 0);

                    m_lastBounds = m_mesh->bounds();
                    setupCameraAroundBounds(m_lastBounds, 1.0f, resetRotation ? &idealRotation : nullptr);
                }
            }
        }
    }

    void MeshPreviewPanel::previewSettings(const MeshPreviewPanelSettings& settings)
    {
        destroyPreviewElements();
        m_previewSettings = settings;
        createPreviewElements();
    }

    void MeshPreviewPanel::changePreviewSettings(const std::function<void(MeshPreviewPanelSettings&)>& func)
    {
        destroyPreviewElements();
        func(m_previewSettings);
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
                rendering::scene::DebugDrawer lines(frame.geometry.solid);
                lines.color(base::Color::YELLOW);
                lines.wireBox(m_mesh->bounds());
            }
        }
    }

    void MeshPreviewPanel::handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables)
    {
        base::Array<base::StringID> materialNames;

        if (m_mesh)
        {
            const auto& materials = m_mesh->materials();
            for (const auto& selectable : selectables)
            {
                if (selectable.objectID() == 42)
                {
                    if (selectable.subObjectID() < materials.size())
                    {
                        if (auto name = materials[selectable.subObjectID()].name)
                            materialNames.pushBackUnique(name);
                    }
                }
            }
        }

        call(EVENT_MATERIAL_CLICKED, materialNames);
    }

    void MeshPreviewPanel::handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
    {

    }

    void MeshPreviewPanel::destroyPreviewElements()
    {
        for (const auto& proxy : m_proxies)
            renderingScene()->dettachProxy(proxy);
        m_proxies.clear();
    }

    void MeshPreviewPanel::createPreviewElements()
    {
        m_proxies.clear();

        if (m_mesh)
        {
			rendering::scene::ObjectProxyMesh::Setup desc;
            desc.mesh = m_mesh;
            desc.forcedLodLevel = m_previewSettings.forceLod;
			
            bool drawSplit = (m_previewSettings.isolateMaterials || m_previewSettings.highlightMaterials) && !m_previewSettings.selectedMaterials.empty();
            if (drawSplit)
            {
                if (!m_previewSettings.isolateMaterials)
                {
                    for (auto materialName : m_previewSettings.selectedMaterials.keys())
                        desc.excludedMaterialMask.insert(materialName);

                    if (auto proxy = rendering::scene::ObjectProxyMesh::Compile(desc))
                    {
                        proxy->m_selectable = rendering::scene::Selectable(42, 0);
                        m_proxies.pushBack(proxy);
                    }
                }

				desc.excludedMaterialMask.clear();

				for (auto materialName : m_previewSettings.selectedMaterials.keys())
					desc.selectiveMaterialMask.insert(materialName);

				if (auto proxy = rendering::scene::ObjectProxyMesh::Compile(desc))
				{
					proxy->m_flags.configure(rendering::scene::ObjectProxyFlagBit::Selected, m_previewSettings.highlightMaterials);
                    proxy->m_selectable = rendering::scene::Selectable(42, 0);
					m_proxies.pushBack(proxy);
				}
            }
            else
            {
				if (auto proxy = rendering::scene::ObjectProxyMesh::Compile(desc))
				{
					proxy->m_selectable = rendering::scene::Selectable(42, 0);
					m_proxies.pushBack(proxy);
				}
            }
        }

        for (auto& proxy : m_proxies)
            renderingScene()->attachProxy(proxy);
    }

    //--
    
} // ed
