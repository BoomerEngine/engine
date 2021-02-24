/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "materialGraphEditor.h"
#include "materialPreviewPanel.h"
#include "materialGraphEditorPanel.h"

#include "base/resource/include/resource.h"
#include "editor/common/include/managedFile.h"
#include "editor/common/include/managedFileFormat.h"
#include "editor/common/include/managedFileNativeResource.h"
#include "editor/common/include/assetBrowser.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiDockPanel.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiSplitter.h"
#include "base/ui/include/uiGraphEditorNodePalette.h"
#include "base/ui/include/uiMenuBar.h"

#include "rendering/material_graph/include/renderingMaterialGraph.h"
#include "rendering/material_graph/include/renderingMaterialGraphBlock.h"
#include "rendering/material/include/renderingMaterialTemplate.h"

BEGIN_BOOMER_NAMESPACE(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialGraphEditor);
RTTI_END_TYPE();

MaterialGraphEditor::MaterialGraphEditor(ManagedFileNativeResource* file)
    : ResourceEditorNativeFile(file, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::CopyPaste, ResourceEditorFeatureBit::UndoRedo })
{
    createInterface();
}

MaterialGraphEditor::~MaterialGraphEditor()
{}

void MaterialGraphEditor::handleContentModified()
{
    updatePreviewMaterial();
}

void MaterialGraphEditor::handleGeneralCopy()
{
    m_graphEditor->actionCopySelection();
}

void MaterialGraphEditor::handleGeneralCut()
{
    m_graphEditor->actionCutSelection();
}

void MaterialGraphEditor::handleGeneralPaste()
{
    m_graphEditor->actionPasteSelection();
}

void MaterialGraphEditor::handleGeneralDelete()
{
    m_graphEditor->actionDeleteSelection();
}

bool MaterialGraphEditor::checkGeneralCopy() const
{
    return m_graphEditor->hasSelection();
}

bool MaterialGraphEditor::checkGeneralCut() const
{
    return m_graphEditor->hasSelection();
}

bool MaterialGraphEditor::checkGeneralPaste() const
{
    return m_graphEditor->hasDataToPaste();
}

bool MaterialGraphEditor::checkGeneralDelete() const
{
    return m_graphEditor->hasSelection();
}

void MaterialGraphEditor::createInterface()
{
    {
        auto tab = base::RefNew<ui::DockPanel>("[img:schema_graph] Graph", "GraphPanel");
        tab->layoutVertical();

        m_graphEditor = tab->createChild<MaterialGraphEditorPanel>(actionHistory());
        m_graphEditor->expand();

        m_graphEditor->bind(EVENT_MATERIAL_BLOCK_SELECTION_CHANGED) = [this]()
        {
            handleChangedSelection();
        };

        dockLayout().attachPanel(tab);
    }

    {
        auto tab = base::RefNew<ui::DockPanel>("[img:shader] Preview", "PreviewPanel");
        tab->layoutVertical();

        m_previewPanel = tab->createChild<MaterialPreviewPanel>();
        m_previewPanel->expand();

        dockLayout().left(0.2f).attachPanel(tab);
    }

    {
        auto tab = base::RefNew<ui::DockPanel>("[img:properties] Properties", "PropertiesPanel");
        tab->layoutVertical();

        m_properties = tab->createChild<ui::DataInspector>();
        m_properties->bindActionHistory(actionHistory());
        m_properties->expand();

        dockLayout().left().bottom(0.5f).attachPanel(tab);
    }

    {
        auto tab = base::RefNew<ui::DockPanel>("[img:class] Palette", "PalettePanel");
        tab->layoutVertical();

        m_graphPalette = tab->createChild<ui::GraphBlockPalette>();
        m_graphPalette->expand();

        dockLayout().right(0.15f).attachPanel(tab);
    }        
}

bool MaterialGraphEditor::initialize()
{
    if (!TBaseClass::initialize())
        return false;

    m_graph = base::rtti_cast<rendering::MaterialGraph>(resource());
    if (!m_graph)
        return false;

    m_graphEditor->bindGraph(m_graph);
    m_graphPalette->setRootClasses(m_graph->graph());

    m_previewMaterial = base::RefNew<rendering::MaterialInstance>();
    m_previewPanel->bindMaterial(m_previewMaterial);

    updatePreviewMaterial();

    return true;
}

bool MaterialGraphEditor::save()
{
    if (!TBaseClass::save())
        return false;

    // load just saved material graph as a material template to force a reload
    LoadResource(res::ResourcePath(nativeFile()->depotPath()));
    return true;
}

//--

void MaterialGraphEditor::updatePreviewMaterial()
{
    auto previewTemplate = m_graph->createPreviewTemplate(TempString("{}.preview", nativeFile()->depotPath()));
    m_previewMaterial->baseMaterial(previewTemplate);
}

void MaterialGraphEditor::handleChangedSelection()
{
    // TODO: multiple block selection
    const auto& selectedBlocks = m_graphEditor->selectedBlocks();
    if (!selectedBlocks.empty())
    {
        m_properties->bindData(selectedBlocks.back()->createDataView());
    }
    else
    {
        // if nothing is selected edit the graph itself
        m_properties->bindData(m_previewMaterial->createDataView());
    }
}

//---

class MaterialGraphResourceEditorOpener : public IResourceEditorOpener
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphResourceEditorOpener, IResourceEditorOpener);

public:
    virtual bool canOpen(const ManagedFileFormat& format) const override
    {
        const auto graphClass = rendering::MaterialGraph::GetStaticClass();
        return (format.nativeResourceClass() == graphClass);
    }

    virtual base::RefPtr<ResourceEditor> createEditor(ManagedFile* file) const override
    {
        if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            return base::RefNew<MaterialGraphEditor>(nativeFile);

        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphResourceEditorOpener);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE(ed)