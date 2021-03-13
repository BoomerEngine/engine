/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "graphEditor.h"
#include "previewPanel.h"
#include "parametersPanel.h"
#include "graphEditorPanel.h"

#include "core/resource/include/resource.h"
#include "core/resource/include/loader.h"
#include "editor/assets/include/browserService.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDockPanel.h"
#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiSplitter.h"
#include "engine/ui/include/uiGraphEditorNodePalette.h"
#include "engine/ui/include/uiMenuBar.h"

#include "engine/material_graph/include/graph.h"
#include "engine/material_graph/include/graphBlock.h"
#include "engine/material/include/materialTemplate.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialGraphEditor);
RTTI_END_TYPE();

MaterialGraphEditor::MaterialGraphEditor(const ResourceInfo& info)
    : ResourceEditor(info, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::CopyPaste, ResourceEditorFeatureBit::UndoRedo })
    , m_graph(rtti_cast<MaterialGraph>(info.resource))
    , m_events(this)
{
    createInterface();

    m_graphEditor->bindGraph(m_graph);
    m_graphPalette->setRootClasses(m_graph->graph());

    m_previewMaterial = RefNew<MaterialInstance>();
    m_previewPanel->bindMaterial(m_previewMaterial);
    m_parametersPanel->bindGraph(m_graph);

    m_events.bind(info.resource->eventKey(), EVENT_RESOURCE_MODIFIED) = [this]
    {
        updatePreviewMaterial();
    };

    updatePreviewMaterial();
}

MaterialGraphEditor::~MaterialGraphEditor()
{}

/*void MaterialGraphEditor::handleContentModified()
{
    updatePreviewMaterial();
}*/

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
        auto tab = RefNew<ui::DockPanel>("[img:schema_graph] Graph", "GraphPanel");
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
        auto tab = RefNew<ui::DockPanel>("[img:shader] Preview", "PreviewPanel");
        tab->layoutVertical();

        m_previewPanel = tab->createChild<MaterialPreviewPanel>();
        m_previewPanel->expand();

        dockLayout().left(0.2f).attachPanel(tab);
    }

    {
        auto tab = RefNew<ui::DockPanel>("[img:properties] Properties", "PropertiesPanel");
        tab->layoutVertical();

        m_properties = tab->createChild<ui::DataInspector>();
        m_properties->bindActionHistory(actionHistory());
        m_properties->expand();

        dockLayout().left().bottom(0.5f).attachPanel(tab);
    }

    {
        auto tab = RefNew<ui::DockPanel>("[img:table] Parameters", "ParametersPanel");
        tab->layoutVertical();

        m_parametersPanel = tab->createChild<MaterialParametersPanel>(actionHistory());
        m_parametersPanel->expand();

        dockLayout().left().bottom(0.5f).attachPanel(tab, false);
        
    }

    {
        auto tab = RefNew<ui::DockPanel>("[img:class] Palette", "PalettePanel");
        tab->layoutVertical();

        m_graphPalette = tab->createChild<ui::GraphBlockPalette>();
        m_graphPalette->expand();

        dockLayout().right(0.15f).attachPanel(tab);
    }        
}

//--

void MaterialGraphEditor::updatePreviewMaterial()
{
    auto previewTemplate = m_graph->createPreviewTemplate(TempString("{}.preview", info().resourceDepotPath));
    m_previewMaterial->baseMaterial(MaterialRef(ResourceID(), previewTemplate));
}

void MaterialGraphEditor::handleChangedSelection()
{
    const auto& selectedBlocks = m_graphEditor->selectedBlocks();
    if (!selectedBlocks.empty())
        m_properties->bindObjects(selectedBlocks);
    else
        m_properties->bindObject(m_previewMaterial);
}

//---

class MaterialGraphResourceEditorOpener : public IResourceEditorOpener
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphResourceEditorOpener, IResourceEditorOpener);

public:
    virtual bool createEditor(ui::IElement* owner, const ResourceInfo& context, ResourceEditorPtr& outEditor) const override final
    {
        if (auto texture = rtti_cast<MaterialGraph>(context.resource))
        {
            outEditor = RefNew<MaterialGraphEditor>(context);
            return true;
        }

        return false;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphResourceEditorOpener);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(ed)
