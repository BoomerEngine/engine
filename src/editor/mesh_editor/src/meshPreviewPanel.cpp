/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "meshPreviewPanel.h"

#include "engine/rendering/include/renderingScene.h"
#include "engine/mesh/include/renderingMesh.h"
#include "engine/rendering/include/renderingSceneObject.h"
#include "engine/rendering/include/renderingSceneObject_Mesh.h"
#include "engine/rendering/include/renderingFrameParams.h"
#include "engine/rendering/include/renderingFrameDebug.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--
     
RTTI_BEGIN_TYPE_CLASS(MeshPreviewPanel);
RTTI_END_TYPE();

MeshPreviewPanel::MeshPreviewPanel()
{
    m_previewSettings.showBounds = true;
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

void MeshPreviewPanel::previewMesh(const MeshPtr& ptr)
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
                auto idealRotation = Angles(40, 30, 0);

                m_lastBounds = m_mesh->bounds();
                focusOnBounds(m_lastBounds, 1.0f, resetRotation ? &idealRotation : nullptr);
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

void MeshPreviewPanel::previewMaterial(StringID name, MaterialPtr data)
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

void MeshPreviewPanel::handleRender(rendering::FrameParams& frame)
{
    TBaseClass::handleRender(frame);

    if (m_mesh)
    {
        if (m_previewSettings.showBounds)
        {
            rendering::DebugDrawer lines(frame.geometry.solid);
            lines.color(Color::YELLOW);
            lines.wireBox(m_mesh->bounds());
        }
    }
}

void MeshPreviewPanel::handlePointSelection(bool ctrl, bool shift, const Point& clientPosition, const Array<Selectable>& selectables)
{
    Array<StringID> materialNames;

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

void MeshPreviewPanel::handleAreaSelection(bool ctrl, bool shift, const Rect& clientRect, const Array<Selectable>& selectables)
{

}

void MeshPreviewPanel::destroyPreviewElements()
{
    for (const auto& proxy : m_proxies)
        scene()->manager<rendering::ObjectManagerMesh>()->detachProxy(proxy);
    m_proxies.clear();
}

void MeshPreviewPanel::createPreviewElements()
{
    m_proxies.clear();

    if (m_mesh)
    {
		rendering::ObjectProxyMesh::Setup desc;
        desc.mesh = m_mesh;
        desc.forcedLodLevel = m_previewSettings.forceLod;
			
        bool drawSplit = (m_previewSettings.isolateMaterials || m_previewSettings.highlightMaterials) && !m_previewSettings.selectedMaterials.empty();
        if (drawSplit)
        {
            if (!m_previewSettings.isolateMaterials)
            {
                for (auto materialName : m_previewSettings.selectedMaterials.keys())
                    desc.excludedMaterialMask.insert(materialName);

                if (auto proxy = rendering::ObjectProxyMesh::Compile(desc))
                {
                    proxy->m_selectable = Selectable(42, 0);
                    m_proxies.pushBack(proxy);
                }
            }

			desc.excludedMaterialMask.clear();

			for (auto materialName : m_previewSettings.selectedMaterials.keys())
				desc.selectiveMaterialMask.insert(materialName);

			if (auto proxy = rendering::ObjectProxyMesh::Compile(desc))
			{
				proxy->m_flags.configure(rendering::ObjectProxyFlagBit::Selected, m_previewSettings.highlightMaterials);
                proxy->m_selectable = Selectable(42, 0);
				m_proxies.pushBack(proxy);
			}
        }
        else
        {
			if (auto proxy = rendering::ObjectProxyMesh::Compile(desc))
			{
				proxy->m_selectable = Selectable(42, 0);
				m_proxies.pushBack(proxy);
			}
        }
    }

    for (auto& proxy : m_proxies)
        scene()->manager<rendering::ObjectManagerMesh>()->attachProxy(proxy);
}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
