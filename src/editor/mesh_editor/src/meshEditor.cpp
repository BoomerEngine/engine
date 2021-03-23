/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "meshEditor.h"
#include "previewPanel.h"
#include "structureNodes.h"
#include "structurePanel.h"
#include "materialsPanel.h"

#include "engine/ui/include/uiDockPanel.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiSplitter.h"
#include "engine/ui/include/uiComboBox.h"

#include "engine/mesh/include/mesh.h"
#include "engine/ui/include/uiMenuBar.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MeshEditor);
RTTI_END_TYPE();

MeshEditor::MeshEditor(const ResourceInfo& info)
    : ResourceEditor(info, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::UndoRedo })
{
    createInterface();

    if (auto mesh = rtti_cast<Mesh>(info.resource))
    {
        m_structurePanel->bindResource(mesh);
        m_previewPanel->previewMesh(mesh);
        m_materialsPanel->bindResource(mesh);
    }
}

MeshEditor::~MeshEditor()
{}

void MeshEditor::createInterface()
{
    {
        auto tab = RefNew<ui::DockPanel>("[img:world] Preview", "PreviewPanel");
        tab->layoutVertical();

        m_previewPanel = tab->createChild<MeshPreviewPanel>();
        m_previewPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_previewPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

        m_previewPanel->bind(EVENT_MATERIAL_CLICKED, this) = [this](Array<StringID> materialNames)
        {
            m_materialsPanel->showMaterials(materialNames);
        };

        dockLayout().attachPanel(tab);
    }

    {
        auto tab = RefNew<ui::DockPanel>("[img:tree] Structure", "StructurePanel");
        tab->layoutVertical();

        m_structurePanel = tab->createChild<MeshStructurePanel>(actionHistory());
        m_structurePanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_structurePanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

        m_structurePanel->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
        {
            updatePreviewSettings();
        };

        dockLayout().right().attachPanel(tab);
    }

    {
        auto tab = RefNew<ui::DockPanel>("[img:color] Materials", "MaterialsPanel");
        tab->layoutVertical();

        m_materialsPanel = tab->createChild<MeshMaterialsPanel>(actionHistory());
        m_materialsPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_materialsPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);

        m_materialsPanel->bind(EVENT_MATERIAL_SELECTION_CHANGED, this) = [this]()
        {
            updateMaterialHighlights();
        };

        dockLayout().right().attachPanel(tab, false);
    }

    {
        toolbar()->createButton(ui::ToolBarButtonInfo("ShowBounds"_id).caption("Bounds", "cube")) = [this]() {
            m_previewSettings.showBounds = !m_previewSettings.showBounds;
            updateToolbar();
            updatePreviewSettings();
        };
    }

    {
        toolbar()->createButton(ui::ToolBarButtonInfo("ShowSkeleton"_id).caption("Skeleton", "skeleton")) = [this]() {
            m_previewSettings.showSkeleton = !m_previewSettings.showSkeleton;
            updateToolbar();
            updatePreviewSettings();
        };
    }

    {
        toolbar()->createButton(ui::ToolBarButtonInfo("ShowBoneAxes"_id).caption("Bone axes", "bone_axis")) = [this]() {
            m_previewSettings.showBoneAxes = !m_previewSettings.showBoneAxes;
            updateToolbar();
            updatePreviewSettings();
        };
    }

    {
        toolbar()->createButton(ui::ToolBarButtonInfo("ShowBoneNames"_id).caption("Bone names", "bone_name")) = [this]() {
            m_previewSettings.showBoneNames = !m_previewSettings.showBoneNames;
            updateToolbar();
            updatePreviewSettings();
        };
    }

    {
        toolbar()->createButton(ui::ToolBarButtonInfo("ShowCollision"_id).caption("Show collision", "physics")) = [this]() {
            m_previewSettings.showCollision = !m_previewSettings.showCollision;
            updateToolbar();
            updatePreviewSettings();
        };
    }

    {
        toolbar()->createButton(ui::ToolBarButtonInfo("ShowSnap"_id).caption("Snap", "snap")) = [this]() {
            m_previewSettings.showSnaps = !m_previewSettings.showSnaps;
            updateToolbar();
            updatePreviewSettings();
        };
    }

    toolbar()->createSeparator();

    {
        m_lodList = toolbar()->createChildWithType<ui::ComboBox>("ToolbarComboBox"_id);
        m_lodList->customMinSize(150, 10);
        m_lodList->addOption("Auto LOD");
        m_lodList->selectOption(0);

        m_lodList->bind(ui::EVENT_COMBO_SELECTED) = [this](int option)
        {
            m_previewSettings.forceLod = option - 1;
            updatePreviewSettings();
        };
    }

    updateToolbar();
    updateLODList();
}

void MeshEditor::fillViewMenu(ui::MenuButtonContainer* menu)
{
    TBaseClass::fillViewMenu(menu);

    // TODO: maybe put the "Show bounds" actions here
}

void MeshEditor::reimported(ResourcePtr resource, ResourceMetadataPtr metadata)
{
    TBaseClass::reimported(resource, metadata);

    if (auto mesh = rtti_cast<Mesh>(resource))
    {
        m_structurePanel->bindResource(mesh);
        m_previewPanel->previewMesh(mesh);
        m_materialsPanel->bindResource(mesh);

        updateLODList();
    }
}

void MeshEditor::updateMaterialHighlights()
{
    updatePreviewSettings();
}

void MeshEditor::updateLODList()
{
    uint32_t numLODs = 0;

    if (auto mesh = rtti_cast<Mesh>(info().resource))
        numLODs = mesh->detailLevels().size();

    m_lodList->clearOptions();
    m_lodList->addOption("Auto LOD");
    for (uint32_t i = 0; i < numLODs; ++i)
        m_lodList->addOption(TempString("LOD {}", i));

    if (m_previewSettings.forceLod > (int)numLODs)
    {
        m_previewSettings.forceLod = (int)numLODs - 1;
        updatePreviewSettings();
    }

    m_lodList->selectOption(m_previewSettings.forceLod + 1);
}

void MeshEditor::updateToolbar()
{
    toolbar()->toggleButton("ShowBounds"_id, m_previewSettings.showBounds);
    toolbar()->toggleButton("ShowSkeleton"_id, m_previewSettings.showSkeleton);
    toolbar()->toggleButton("ShowBoneAxes"_id, m_previewSettings.showBoneAxes);
    toolbar()->toggleButton("ShowBoneNames"_id, m_previewSettings.showBoneNames);
    toolbar()->toggleButton("ShowCollision"_id, m_previewSettings.showCollision);
    toolbar()->toggleButton("ShowSnaps"_id, m_previewSettings.showSnaps);
}

void MeshEditor::updatePreviewSettings()
{
    auto settings = m_previewSettings;

    {
        settings.highlightMaterials = m_materialsPanel->settings().highlight;
        settings.isolateMaterials = m_materialsPanel->settings().isolate;
        settings.selectedMaterials.clear();
        m_materialsPanel->collectSelectedMaterialNames(settings.selectedMaterials);
    }

    {
        InplaceArray<const IMeshStructureNode*, 10> nodes;
        m_structurePanel->collect(nodes);

        for (const auto& node : nodes)
            node->setup(settings, nodes, nodes.back());
    }

    m_previewPanel->previewSettings(settings);
}

//---

class MeshResourceEditorOpener : public IResourceEditorOpener
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshResourceEditorOpener, IResourceEditorOpener);

public:
    virtual bool createEditor(ui::IElement* owner, const ResourceInfo& context, ResourceEditorPtr& outEditor) const override final
    {
        if (auto texture = rtti_cast<Mesh>(context.resource))
        {
            outEditor = RefNew<MeshEditor>(context);
            return true;
        }

        return false;
    }
};

RTTI_BEGIN_TYPE_CLASS(MeshResourceEditorOpener);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(ed)
