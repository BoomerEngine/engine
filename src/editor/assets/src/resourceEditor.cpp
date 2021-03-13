/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"
#include "editor/common/include/service.h"

#include "resourceEditor.h"
#include "resourceReimport.h"

#include "engine/canvas/include/canvas.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiDockContainer.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiWindowPopup.h"
#include "engine/ui/include/uiDockNotebook.h"
#include "engine/ui/include/uiElementConfig.h"
#include "engine/ui/include/uiMessageBox.h"

#include "core/app/include/launcherPlatform.h"
#include "core/object/include/actionHistory.h"
#include "core/resource/include/depot.h"
#include "core/resource/include/metadata.h"
#include "core/containers/include/path.h"
#include "core/object/include/action.h"
#include "browserService.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceEditorOpener);
RTTI_END_TYPE();

IResourceEditorOpener::~IResourceEditorOpener()
{}

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ResourceEditor);
RTTI_END_TYPE();

ResourceEditor::ResourceEditor(const ResourceInfo& info, ResourceEditorFeatureFlags flags, StringView defaultEditorTag)
    : DockPanel("Editor")
    , m_features(flags)
    , m_containerTag(defaultEditorTag)
    , m_info(info)
    , m_toolbarTimer(this)
{
    layoutVertical();

    // action history
    m_actionHistory = RefNew<ActionHistory>();
       
    // set title and stuff for the tab
    tabCloseButton(true);
    tabIcon("file_edit");
    tabTitle(info.resourceDepotPath.view().fileStem());

    // menu bar
    {
        m_menubar = createChild<ui::MenuBar>();
        m_menubar->createMenu("File", [this]()
            {
                auto menu = RefNew<ui::MenuButtonContainer>();
                fillFileMenu(menu);
                return menu->convertToPopup();
            });
        m_menubar->createMenu("Edit", [this]()
            {
                auto menu = RefNew<ui::MenuButtonContainer>();
                fillEditMenu(menu);
                return menu->convertToPopup();
            });
        m_menubar->createMenu("View", [this]()
            {
                auto menu = RefNew<ui::MenuButtonContainer>();
                fillViewMenu(menu);
                return menu->convertToPopup();
            });
        m_menubar->createMenu("Tools", [this]()
            {
                auto menu = RefNew<ui::MenuButtonContainer>();
                fillToolMenu(menu);
                return menu->convertToPopup();
            });
    }

    // toolbar
    {
        m_toolbar = createChild<ui::ToolBar>();

        if (m_features.test(ResourceEditorFeatureBit::Save))
        {
            m_toolbar->createButton(ui::ToolBarButtonInfo("Save"_id).caption("Save", "save").tooltip("Save changed to all modified files"))
                = [this]() { handleGeneralSave(); };
            m_toolbar->createSeparator();

            bindShortcut("Ctrl+S") = [this]() { handleGeneralSave(); };
        }

        if (m_features.test(ResourceEditorFeatureBit::UndoRedo))
        {
            m_toolbar->createButton(ui::ToolBarButtonInfo("Undo"_id).caption("Undo", "undo").tooltip("Undo last action"))
                = [this]() { handleGeneralUndo(); };
            m_toolbar->createButton(ui::ToolBarButtonInfo("Redo"_id).caption("Redo", "redo").tooltip("Redo last action"))
                = [this]() { handleGeneralRedo(); };
            m_toolbar->createSeparator();

            bindShortcut("Ctrl+Z") = [this]() { handleGeneralUndo(); };
            bindShortcut("Ctrl+Y") = [this]() { handleGeneralRedo(); };
        }

        if (m_features.test(ResourceEditorFeatureBit::CopyPaste))
        {
            m_toolbar->createButton(ui::ToolBarButtonInfo().caption("Copy", "copy").tooltip("Copy selected objects to clipboard"))
                = [this]() { handleGeneralCopy(); };
            m_toolbar->createButton(ui::ToolBarButtonInfo().caption("Cut", "cut").tooltip("Cut selected objects to clipboard"))
                = [this]() { handleGeneralCut(); };
            m_toolbar->createButton(ui::ToolBarButtonInfo().caption("Paste", "paste").tooltip("Paste object from clipboard"))
                = [this]() { handleGeneralPaste(); };
            m_toolbar->createButton(ui::ToolBarButtonInfo().caption("Delete", "delete").tooltip("Delete selected objects"))
                = [this]() { handleGeneralDelete(); };
            m_toolbar->createSeparator();

            bindShortcut("Ctrl+C") = [this]() { handleGeneralCopy(); };
            bindShortcut("Ctrl+X") = [this]() { handleGeneralCut(); };
            bindShortcut("Ctrl+V") = [this]() { handleGeneralPaste(); };
            bindShortcut("Ctrl+D") = [this]() { handleGeneralDuplicate(); };
            bindShortcut("Delete") = [this]() { handleGeneralDelete(); };
        }
    }

    m_dock = createChild<ui::DockContainer>();
    m_dock->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_dock->customVerticalAligment(ui::ElementVerticalLayout::Expand);

    //--

    bind(ui::EVENT_TAB_CONTEXT_MENU) = [this](ui::Position pos)
    {
        showTabContextMenu(pos);
    };

    //--

    if (!m_info.metadata->importDependencies.empty())
    {
        const auto rootSourceAsset = m_info.metadata->importDependencies[0].importPath;
        if (rootSourceAsset)
        {
            m_reimportPanel = RefNew<ResourceReimportPanel>(this);
            m_reimportPanel->expand();
            dockLayout().right(0.2f).attachPanel(m_reimportPanel);
        }
    }

    //--

    m_toolbarTimer = [this]() { refreshToolbar(); };
    m_toolbarTimer.startRepeated(0.1f);

    //--
}

ResourceEditor::~ResourceEditor()
{}

void ResourceEditor::configLoad(const ui::ConfigBlock& block)
{
    tabLocked(block.readOrDefault("TabLocked", false));
}

void ResourceEditor::configSave(const ui::ConfigBlock& block) const
{
    block.write("TabLocked", tabLocked());
}

ui::DockLayoutNode& ResourceEditor::dockLayout()
{
    return m_dock->layout();
}

void ResourceEditor::fillFileMenu(ui::MenuButtonContainer* menu)
{
    if (m_features.test(ResourceEditorFeatureBit::Save))
    {
        menu->createCallback("Save", "[img:save]")
            = [this]() { handleGeneralSave(); };
        menu->createSeparator();
    }
}

void ResourceEditor::fillEditMenu(ui::MenuButtonContainer* menu)
{
    if (m_features.test(ResourceEditorFeatureBit::UndoRedo))
    {
        menu->createCallback("Undo", "[img:undo]")
            = [this]() { handleGeneralUndo(); };
        menu->createCallback("Redo", "[img:redo]")
            = [this]() { handleGeneralRedo(); };
        menu->createSeparator();
    }

    if (m_features.test(ResourceEditorFeatureBit::CopyPaste))
    {
        menu->createCallback("Copy", "[img:copy]")
            = [this]() { handleGeneralCopy(); };
        menu->createCallback("Cut", "[img:cut]")
            = [this]() { handleGeneralCut(); };
        menu->createCallback("Paste", "[img:paste]")
            = [this]() { handleGeneralPaste(); };
        menu->createCallback("Delete", "[img:delete]")
            = [this]() { handleGeneralDelete(); };
        menu->createSeparator();
    }
}

void ResourceEditor::fillViewMenu(ui::MenuButtonContainer* menu)
{
       
}

void ResourceEditor::fillToolMenu(ui::MenuButtonContainer* menu)
{
    // the local tools
    m_dock->layout().fillViewMenu(menu);
    menu->createSeparator();

    // main window tools
    /*if (auto* mainWindow = findMainWindow())
    {
        mainWindow->layout().fillViewMenu(menu);
        menu->createSeparator();
    }*/
}

void ResourceEditor::cleanup()
{
}

void ResourceEditor::update()
{}

void ResourceEditor::handleReimportAction(ResourcePtr resource, ResourceMetadataPtr metadata)
{
    auto action = RefNew<ActionInplace>("Reimport", "Reimport"_id);

    auto currentResource = m_info.resource;
    auto currentMetadata = m_info.metadata;

    action->doFunc = [resource, metadata, this]()
    {
        reimported(resource, metadata);
        return true;
    };

    action->undoFunc = [currentResource, currentMetadata, this]()
    {
        reimported(currentResource, currentMetadata);
        return true;
    };

    actionHistory()->execute(action);
}

void ResourceEditor::reimported(ResourcePtr resource, ResourceMetadataPtr metadata)
{
    DEBUG_CHECK_RETURN_EX(resource, "Invalid reimported resource");
    DEBUG_CHECK_RETURN_EX(metadata, "Invalid reimported metadata");

    // apply resource
    m_info.resource = resource;
    m_info.resource->markModified();

    // update metadata
    metadata->ids = m_info.metadata->ids;
    metadata->loadExtension = m_info.metadata->loadExtension;
    metadata->internalRevision = m_info.metadata->internalRevision + 1;

    // apply new metadata 
    m_info.metadata = metadata;
    m_info.metadata->markModified();

    // show new config
    if (m_reimportPanel)
        m_reimportPanel->updateMetadata(metadata);
}

bool ResourceEditor::modified() const
{
    return m_info.resource->modified();
}

bool ResourceEditor::saveCustomFormat()
{
    return false;
}

bool ResourceEditor::save()
{
    auto* depot = GetService<DepotService>();

    if (!depot->saveFileFromXMLObject(m_info.metadataDepotPath, m_info.metadata))
        return false;

    //m_info.metadata->resetModifiedFlag();

    if (m_info.customFormat)
    {
        if (!saveCustomFormat())
            return false;
    }
    else
    {
        if (!depot->saveFileFromResource(m_info.resourceDepotPath, m_info.resource))
            return false;
    }

    m_info.resource->resetModifiedFlag();

    LoadResource(info().resourceDepotPath);

    //actionHistory()->clear();

    return true;
}

//--

void ResourceEditor::refreshToolbar()
{
    m_toolbar->enableButton("Save"_id, modified());
    m_toolbar->enableButton("Undo"_id, m_actionHistory->hasUndo());
    m_toolbar->enableButton("Redo"_id, m_actionHistory->hasRedo());
}

bool ResourceEditor::showTabContextMenu(const ui::Position& pos)
{
    auto menu = RefNew<ui::MenuButtonContainer>();

    {
        // editor crap
        menu->createCallback("Close", "[img:cross]") = [this]() { handleCloseRequest(); };
        menu->createSeparator();

        // tab locking (prevents accidental close)
        if (tabLocked())
            menu->createCallback("Unlock", "[img:lock_open]") = [this]() { tabLocked(false); };
        else
            menu->createCallback("Lock", "[img:lock]") = [this]() { tabLocked(true); };
        menu->createSeparator();

        // general file crap
        menu->createCallback("Show in depot", "[img:zoom]") = [this]() { GetService<AssetBrowserService>()->showFile(info().resourceDepotPath); };
        menu->createSeparator();
    }

    menu->show(this);
    return true;
}

void ResourceEditor::close()
{
    /*if (auto* container = findParent<IBaseResourceContainerWindow>())
        container->detachEditor(this);*/

    cleanup();

    TBaseClass::close();
}

void ResourceEditor::handleCloseRequest()
{
    if (modified())
    {
        StringBuilder txt;
        txt.appendf("File '{}' is [b][color:#F00]modified[/color][/b].\n \nDo you want to save it or discard the changes?", info().resourceDepotPath);

        ui::MessageBoxSetup setup;
        setup.title("Confirm closing editor");
        setup.message(txt.toString());
        setup.yes().no().cancel().defaultYes().warn();
        setup.caption(ui::MessageButton::Yes, "[img:save] Save");
        setup.caption(ui::MessageButton::No, "[img:delete_black] Discard");
        setup.m_constructiveButton = ui::MessageButton::Yes;
        setup.m_destructiveButton = ui::MessageButton::No;

        const auto ret = ui::ShowMessageBox(this, setup);
        if (ret == ui::MessageButton::Yes)
        {
            if (!save())
            {
                ui::PostWindowMessage(this, ui::MessageType::Error, "FileSave"_id, TempString("Error saving file '{}'", info().resourceDepotPath));
                return;
            }
        }
        else if (ret == ui::MessageButton::Cancel)
        {
            return;
        }
    }

    TBaseClass::handleCloseRequest();
}

//---

void ResourceEditor::handleGeneralUndo()
{
    if (!m_actionHistory->undo())
        ui::PostWindowMessage(this, ui::MessageType::Warning, "UndoRedo"_id, "Undo of last operation has failed");
}

void ResourceEditor::handleGeneralRedo()
{
    if (!m_actionHistory->redo())
        ui::PostWindowMessage(this, ui::MessageType::Warning, "UndoRedo"_id, "Redo of last operation has failed");
}

void ResourceEditor::handleGeneralSave()
{
    if (!save())
        ui::PostWindowMessage(this, ui::MessageType::Error, "Save"_id, TempString("Failed to save file '{}'", info().resourceDepotPath));
}

void ResourceEditor::handleGeneralCopy()
{

}

void ResourceEditor::handleGeneralCut()
{

}

void ResourceEditor::handleGeneralPaste()
{

}

void ResourceEditor::handleGeneralDelete()
{

}

void ResourceEditor::handleGeneralDuplicate()
{

}

bool ResourceEditor::checkGeneralUndo() const
{
    return m_actionHistory->hasUndo();
}

bool ResourceEditor::checkGeneralRedo() const
{
    return m_actionHistory->hasRedo();
}

bool ResourceEditor::checkGeneralSave() const
{
    return modified();
}

bool ResourceEditor::checkGeneralCopy() const
{
    return false;
}

bool ResourceEditor::checkGeneralCut() const
{
    return false;
}

bool ResourceEditor::checkGeneralPaste() const
{
    return false;
}

bool ResourceEditor::checkGeneralDelete() const
{
    return false;
}

bool ResourceEditor::checkGeneralDuplicate() const
{
    return false;
}

//--

bool ResourceEditor::CreateEditor(ui::IElement* owner, StringView depotPath, ResourceEditorPtr& outEditor)
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid depot path", false);

    // load metadata of the asset, it should always be there
    const auto metadataPath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);
    ResourceMetadataPtr metadata;
    if (!GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(metadataPath, metadata))
    {
        if (!owner)
        {
            ui::ShowMessageBox(owner, ui::MessageBoxSetup().title("Open editor").message("Unable to load metadata for the resource, it may be corrupted.").error());
            // TODO: options to "edit in notepad"
            // TODO: options to "recreate metadata" etc
        }

        return false;
    }

    // metadata does not specifiy the resource class ?
    if (!metadata->resourceClassType)
    {
        if (!owner)
        {
            ui::ShowMessageBox(owner, ui::MessageBoxSetup().title("Open editor").message("Unknown asset format.").error());
            // TODO: try to load from asset file
            // TODO: option to manually specify
        }

        return false;
    }

    // check that the source file exists
    const auto assetExtension = metadata->loadExtension ? metadata->loadExtension : IResource::FILE_EXTENSION;
    const auto assetPath = ReplaceExtension(depotPath, assetExtension);
    if (!GetService<DepotService>()->fileExists(assetPath))
    {
        if (!owner)
        {
            ui::ShowMessageBox(owner, ui::MessageBoxSetup().title("Open editor").message("Asset file do not exist.").error());
            // TODO: options to "create empty (for non-imported ones)"
            // TODO: options to "delete"
        }

        return false;
    }

    // TODO: check for reimport before opening ?
    
    // load existing asset
    ResourcePtr resourcePtr;
    if (!GetService<DepotService>()->loadFileToResource(assetPath, resourcePtr, metadata->resourceClassType, true))
    {
        if (!owner)
        {
            ui::ShowMessageBox(owner, ui::MessageBoxSetup().title("Open editor").message("Failed to load asset.").error());
            // TODO: options to "create empty (for non-imported ones)"
            // TODO: options to "delete"
        }

        return false;
    }

    //--

    ResourceInfo info;
    info.metadata = metadata;
    info.resource = resourcePtr;
    info.customFormat = assetExtension != IResource::FILE_EXTENSION;
    info.resourceExtension = StringBuf(assetExtension);
    info.resourceDepotPath = assetPath;
    info.metadataDepotPath = metadataPath;

    InplaceArray<SpecificClassType<IResourceEditorOpener>, 32> resourceOpeners;
    RTTI::GetInstance().enumClasses(resourceOpeners);

    for (const auto openerClass : resourceOpeners)
    {
        auto opener = openerClass->create<IResourceEditorOpener>();
        if (opener->createEditor(owner, info, outEditor))
            return true;
    }

    if (!owner)
    {
        ui::ShowMessageBox(owner, ui::MessageBoxSetup().title("Open editor").message("No editor found that can open selected file.").error());
        // TODO: options to "create empty (for non-imported ones)"
        // TODO: options to "delete"
    }

    return false;
}

//--

END_BOOMER_NAMESPACE_EX(ed)

