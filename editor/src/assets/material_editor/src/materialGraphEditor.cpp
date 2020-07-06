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

#include "base/resources/include/resource.h"
#include "base/editor/include/managedFile.h"
#include "base/editor/include/managedFileFormat.h"
#include "base/editor/include/assetBrowser.h"
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

namespace ed
{

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialGraphEditor);
    RTTI_END_TYPE();

    MaterialGraphEditor::MaterialGraphEditor(ConfigGroup config, ManagedFile* file)
        : SingleResourceEditor(config, file)
    {
        actions().bindCommand("MaterialGraphEditor.Copy"_id) = [this]() { m_graphEditor->actionCopySelection(); };
        actions().bindCommand("MaterialGraphEditor.Cut"_id) = [this]() { m_graphEditor->actionCutSelection(); };
        actions().bindCommand("MaterialGraphEditor.Paste"_id) = [this]() { m_graphEditor->actionPasteSelection(); };
        actions().bindCommand("MaterialGraphEditor.Delete"_id) = [this]() { m_graphEditor->actionDeleteSelection(); };

        actions().bindFilter("MaterialGraphEditor.Copy"_id) = [this]() { return m_graphEditor->hasSelection(); };
        actions().bindFilter("MaterialGraphEditor.Cut"_id) = [this]() { return m_graphEditor->hasSelection(); };
        actions().bindFilter("MaterialGraphEditor.Paste"_id) = [this]() { return m_graphEditor->hasDataToPaste(); };
        actions().bindFilter("MaterialGraphEditor.Delete"_id) = [this]() { return m_graphEditor->hasSelection(); };

        createInterface();
    }

    MaterialGraphEditor::~MaterialGraphEditor()
    {}

    void MaterialGraphEditor::createInterface()
    {
        {
            toolbar()->createButton("MaterialGraphEditor.Copy"_id, "[img:copy]", "Copy selected blocks");
            toolbar()->createButton("MaterialGraphEditor.Cut"_id, "[img:cut]", "Cut selected blocks");
            toolbar()->createButton("MaterialGraphEditor.Paste"_id, "[img:paste]", "Paste blocks");
            toolbar()->createButton("MaterialGraphEditor.Delete"_id, "[img:delete]", "Delete blocks");
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:schema_graph] Graph", "GraphPanel");
            tab->layoutVertical();

            m_graphEditor = tab->createChild<MaterialGraphEditorPanel>(actionHistory());
            m_graphEditor->expand();

            m_graphEditor->bind("OnSelectionChanged"_id) = [this]()
            {
                handleChangedSelection();
            };

            dockLayout().attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:shader] Preview", "PreviewPanel");
            tab->layoutVertical();

            m_previewPanel = tab->createChild<MaterialPreviewPanelWithToolbar>();
            m_previewPanel->expand();

            dockLayout().left(0.2f).attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:properties] Properties", "PropertiesPanel");
            tab->layoutVertical();

            m_properties = tab->createChild<ui::DataInspector>();
            m_properties->bindActionHistory(actionHistory());
            m_properties->expand();

            dockLayout().left().bottom(0.5f).attachPanel(tab);
        }

        {
            auto tab = base::CreateSharedPtr<ui::DockPanel>("[img:class] Palette", "PalettePanel");
            tab->layoutVertical();

            m_graphPalette = tab->createChild<ui::GraphBlockPalette>();
            m_graphPalette->expand();

            dockLayout().right(0.15f).attachPanel(tab);
        }

        
    }

    void MaterialGraphEditor::fillEditMenu(ui::MenuButtonContainer* menu)
    {
        TBaseClass::fillEditMenu(menu);

        menu->createSeparator();
        menu->createAction("MaterialGraphEditor.Copy"_id, "Copy", "[img:copy]");
        menu->createAction("MaterialGraphEditor.Cut"_id, "Cut", "[img:cut]");
        menu->createAction("MaterialGraphEditor.Paste"_id, "Paste", "[img:paste]");
        menu->createAction("MaterialGraphEditor.Delete"_id, "Delete", "[img:delete]");
    }

    bool MaterialGraphEditor::initialize()
    {
        if (!TBaseClass::initialize())
            return false;

        m_graph = file()->loadContent<rendering::MaterialGraph>();
        if (!m_graph)
            return false;

        m_graph->resetModifiedFlag();
        m_graphEditor->bindGraph(m_graph);
        m_graphPalette->setRootClasses(m_graph->graph());

        m_previewInstance = base::CreateSharedPtr<rendering::MaterialInstance>();

        if (auto existingMaterial = base::LoadResource<rendering::MaterialTemplate>(base::res::ResourcePath(file()->depotPath())))
            m_previewInstance->baseMaterial(existingMaterial);

        m_previewPanel->bindMaterial(m_previewInstance);
        return true;
    }

    bool MaterialGraphEditor::saveFile(ManagedFile* fileToSave)
    {
        if (fileToSave == file())
        {
            if (file()->storeContent(m_graph))
            {
                m_graph->resetModifiedFlag();
                return true;
            }
        }

        return false;
    }

    void MaterialGraphEditor::collectModifiedFiles(AssetItemList& outList) const
    {
        if (m_graph->modified())
            outList.collectFile(file());
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
            m_properties->bindData(m_previewInstance->createDataView());
        }
    }

    //---

    class  MaterialGraphResourceEditorOpener : public IResourceEditorOpener
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphResourceEditorOpener, IResourceEditorOpener);

    public:
        virtual bool canOpen(const ManagedFileFormat& format) const override
        {
            const auto graphClass = rendering::MaterialGraph::GetStaticClass();
            return (format.nativeResourceClass() == graphClass);
        }

        virtual base::RefPtr<ResourceEditor> createEditor(ConfigGroup config, ManagedFile* file) const override
        {
            return base::CreateSharedPtr<MaterialGraphEditor>(config, file);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphResourceEditorOpener);
    RTTI_END_TYPE();

    //---

} // ed