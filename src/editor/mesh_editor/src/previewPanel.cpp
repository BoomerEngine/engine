/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "previewPanel.h"

#include "engine/rendering/include/scene.h"
#include "engine/mesh/include/mesh.h"
#include "engine/rendering/include/object.h"
#include "engine/rendering/include/objectMesh.h"
#include "engine/rendering/include/params.h"
#include "engine/rendering/include/debugGeometry.h"
#include "engine/rendering/include/debugGeometryBuilder.h"
#include "engine/material/include/materialInstance.h"
#include "engine/material/include/materialTemplate.h"
#include "engine/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--
     
RTTI_BEGIN_TYPE_CLASS(MeshPreviewPanel);
RTTI_END_TYPE();

MeshPreviewPanel::MeshPreviewPanel()
{
    m_previewSettings.showBounds = true;

    auto statsArea = centerArea()->createChildWithType<ui::IElement>("StatsPanel"_id);
    statsArea->customHorizontalAligment(ui::ElementHorizontalLayout::Right);
    statsArea->customVerticalAligment(ui::ElementVerticalLayout::Top);
    statsArea->overlay(true);

    m_statsText = statsArea->createChild<ui::TextLabel>();
    m_statsText->customMinSize(250, 20);
    m_statsText->customHorizontalAligment(ui::ElementHorizontalLayout::Left);
    m_statsText->customVerticalAligment(ui::ElementVerticalLayout::Top);
    m_statsText->text("Mesh stats");
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

void MeshPreviewPanel::buildStatsString(IFormatStream& f) const
{
    if (m_mesh)
    {
        const float distance = cameraSettings().position.distance(Vector3::ZERO());
        f.appendf("Distance: [color:#AAF]{}[/color] m[br]", Prec(distance, 2));

        const auto size = m_mesh->bounds().size();
        f.appendf("Bounds: {}x{}x{} m[br]", Prec(size.x, 2), Prec(size.y, 2), Prec(size.z, 2));

        uint32_t lodMask = 0;
        if (m_previewSettings.forceLod < 0)
            lodMask = m_mesh->calculateActiveDetailLevels(distance);
        else
            lodMask = 1U << m_previewSettings.forceLod;

        HashSet<uint32_t> activeMaterials;
        HashMap<int, uint32_t> activeVertexFormatsTris;
        HashMap<int, uint32_t> activeVertexFormatsChunks;
        HashMap<StringBuf, uint32_t> activeShadersTris;
        HashMap<StringBuf, uint32_t> activeShadersChunks;
        uint32_t numTriangles = 0;
        uint32_t numChunks = 0;
        if (lodMask)
        {
            for (uint32_t i = 0; i < 32; ++i)
                if (lodMask & (1U << i))
                    f.appendf("LOD{} ", i);

            for (const auto& chunk : m_mesh->chunks())
            {
                if (chunk.detailMask & lodMask)
                {
                    numTriangles += chunk.indexCount / 3;
                    activeMaterials.insert(chunk.materialIndex);
                    activeVertexFormatsTris[(int)chunk.vertexFormat] += chunk.indexCount / 3;
                    activeVertexFormatsChunks[(int)chunk.vertexFormat] += 1;

                    const auto& material = m_mesh->materials()[chunk.materialIndex];
                    if (material.material)
                    {
                        if (const auto base = material.material->resolveTemplate())
                        {
                            const auto loadPath = base->loadPath();
                            if (!loadPath.empty())
                            {
                                activeShadersTris[loadPath] += chunk.indexCount / 3;
                                activeShadersChunks[loadPath] += 1;
                            }
                        }
                    }
                }
            }
            f.append("[br]");

            f.appendf("Total LOD Triangles: [b]{}[/b][br]", numTriangles);
            f.appendf("Total LOD Chunks: [b]{}[/b][br]", numChunks);
            f.appendf("Total LOD Materials: [b]{}[/b][br]", activeMaterials.size());
            f.appendf("[br]");

            for (const auto& key : activeVertexFormatsTris.keys())
                f.appendf("VF {}: {} chunks, {} tris[br]", (MeshVertexFormat)key, activeVertexFormatsChunks[key], activeVertexFormatsTris[key]);
            f.append("[br]");

            for (const auto& key : activeShadersTris.keys())
            {
                const auto shortName = key.view().fileStem();
                f.appendf("Shader {}: {} chunks, {} tris[br]", shortName, activeShadersChunks[key], activeShadersTris[key]);
            }
            f.append("[br]");
        }
        else
        {
            f.append("[b][color:#F88]NoLOD[/color][b][br]");
        }
    }

    f.append(" [br]");
    m_frameStats.mainView.print("Rendering.", f);
}

void MeshPreviewPanel::handlePostRenderContent()
{
    // update stats
    {
        StringBuilder s;
        s << "[b][size:++]Rendering stats[/size][/b][br]";
        s << " [br]";

        buildStatsString(s);
        m_statsText->text(s.view());
    }
}

void MeshPreviewPanel::handleFrame(FrameParams& frame, DebugGeometryCollector& debug)
{
    TBaseClass::handleFrame(frame, debug);

    if (m_mesh)
    {
        if (m_previewSettings.showBounds)
        {
            DebugGeometryBuilder dd;
            dd.color(Color::YELLOW);
            dd.wireBox(m_mesh->bounds());
            debug.push(dd);
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
        scene()->manager<RenderingMeshManager>()->commandDetachProxy(proxy);
    m_proxies.clear();
}

void MeshPreviewPanel::createPreviewElements()
{
    m_proxies.clear();

    if (m_mesh)
    {
		RenderingMesh::Setup desc;
        desc.mesh = m_mesh;
        desc.forcedLodLevel = m_previewSettings.forceLod;
        desc.focedSingleChunk = m_previewSettings.forceChunk;
			
        bool drawSplit = (m_previewSettings.isolateMaterials || m_previewSettings.highlightMaterials) && !m_previewSettings.selectedMaterials.empty();
        if (drawSplit)
        {
            if (!m_previewSettings.isolateMaterials)
            {
                for (auto materialName : m_previewSettings.selectedMaterials.keys())
                    desc.excludedMaterialMask.insert(materialName);

                if (auto proxy = RenderingMesh::Compile(desc))
                {
                    proxy->m_selectable = Selectable(42, 0);
                    m_proxies.pushBack(proxy);
                }
            }

			desc.excludedMaterialMask.clear();

			for (auto materialName : m_previewSettings.selectedMaterials.keys())
				desc.selectiveMaterialMask.insert(materialName);

			if (auto proxy = RenderingMesh::Compile(desc))
			{
				proxy->m_flags.configure(RenderingObjectFlagBit::Selected, m_previewSettings.highlightMaterials);
                proxy->m_selectable = Selectable(42, 0);
				m_proxies.pushBack(proxy);
			}
        }
        else
        {
			if (auto proxy = RenderingMesh::Compile(desc))
			{
				proxy->m_selectable = Selectable(42, 0);
				m_proxies.pushBack(proxy);
			}
        }
    }

    for (auto& proxy : m_proxies)
        scene()->manager<RenderingMeshManager>()->commandAttachProxy(proxy);
}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
