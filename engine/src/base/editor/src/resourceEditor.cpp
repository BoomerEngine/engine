/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"
#include "editorService.h"
#include "editorWindow.h"
#include "resourceEditor.h"

#include "managedFileNativeResource.h"
#include "managedDepotContextMenu.h"

#include "base/canvas/include/canvas.h"
#include "base/app/include/launcherPlatform.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiWindowPopup.h"
#include "base/ui/include/uiDockNotebook.h"
#include "base/ui/include/uiElementConfig.h"
#include "base/object/include/actionHistory.h"

namespace ed
{

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceEditorOpener);
    RTTI_END_TYPE();

    IResourceEditorOpener::~IResourceEditorOpener()
    {}

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ResourceEditor);
    RTTI_END_TYPE();

    ResourceEditor::ResourceEditor(ManagedFile* file, ResourceEditorFeatureFlags flags)
        : DockPanel("Editor")
        , m_features(flags)
        , m_file(file)
    {
        layoutVertical();
        createActions();

        // action history
        m_actionHistory = RefNew<ActionHistory>();
       
        // set title and stuff for the tab
        tabCloseButton(true);
        tabIcon("file_edit");
        tabTitle(file->name());

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
                m_toolbar->createButton("Editor.Save"_id, ui::ToolbarButtonSetup().icon("save").caption("Save").tooltip("Save changed to all modified files"));
                m_toolbar->createSeparator();
            }

            if (m_features.test(ResourceEditorFeatureBit::UndoRedo))
            {
                m_toolbar->createButton("Editor.Undo"_id, ui::ToolbarButtonSetup().icon("undo").caption("Undo").tooltip("Undo last action"));
                m_toolbar->createButton("Editor.Redo"_id, ui::ToolbarButtonSetup().icon("redo").caption("Redo").tooltip("Redo last action"));
                m_toolbar->createSeparator();
            }

            if (m_features.test(ResourceEditorFeatureBit::CopyPaste))
            {
                m_toolbar->createButton("Editor.Copy"_id, ui::ToolbarButtonSetup().icon("copy").caption("Copy").tooltip("Copy selected objects to clipboard"));
                m_toolbar->createButton("Editor.Cut"_id, ui::ToolbarButtonSetup().icon("cut").caption("Cut").tooltip("Cut selected objects to clipboard"));
                m_toolbar->createButton("Editor.Paste"_id, ui::ToolbarButtonSetup().icon("paste").caption("Paste").tooltip("Paste object from clipboard"));
                m_toolbar->createButton("Editor.Delete"_id, ui::ToolbarButtonSetup().icon("delete").caption("Delete").tooltip("Delete selected objects"));
                m_toolbar->createSeparator();
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

    void ResourceEditor::createActions()
    {
        // general save
        if (m_features.test(ResourceEditorFeatureBit::Save))
        {
            actions().bindCommand("Editor.Save"_id) = [this]() { handleGeneralSave(); };
            actions().bindFilter("Editor.Save"_id) = [this]() { return checkGeneralSave(); };
            actions().bindShortcut("Editor.Save"_id, "Ctrl+S");
        }

        // general "reimport"
        if (m_features.test(ResourceEditorFeatureBit::Imported))
        {
            actions().bindCommand("Editor.Reimport"_id) = [this]() { handleGeneralReimport(); };
            //actions().bindFilter("Editor.Reimport"_id) = [this]() { return checkGeneralSave(); };
            actions().bindShortcut("Editor.Reimport"_id, "Ctrl+Shift+R");
        }

        // general undo/redo
        if (m_features.test(ResourceEditorFeatureBit::UndoRedo))
        {
            actions().bindCommand("Editor.Undo"_id) = [this]() { handleGeneralUndo(); };
            actions().bindCommand("Editor.Redo"_id) = [this]() { handleGeneralRedo(); };
            actions().bindFilter("Editor.Undo"_id) = [this]() { return checkGeneralUndo(); };
            actions().bindFilter("Editor.Redo"_id) = [this]() { return checkGeneralRedo(); };
            actions().bindShortcut("Editor.Undo"_id, "Ctrl+Z");
            actions().bindShortcut("Editor.Redo"_id, "Ctrl+Y");
        }

        // general copy/paste
        if (m_features.test(ResourceEditorFeatureBit::CopyPaste))
        {
            actions().bindCommand("Editor.Copy"_id) = [this]() { handleGeneralCopy(); };
            actions().bindCommand("Editor.Cut"_id) = [this]() { handleGeneralCut(); };
            actions().bindCommand("Editor.Paste"_id) = [this]() { handleGeneralPaste(); };
            actions().bindCommand("Editor.Delete"_id) = [this]() { handleGeneralDelete(); };
            actions().bindFilter("Editor.Copy"_id) = [this]() { return checkGeneralCopy(); };
            actions().bindFilter("Editor.Cut"_id) = [this]() { return checkGeneralCut(); };
            actions().bindFilter("Editor.Paste"_id) = [this]() { return checkGeneralPaste(); };
            actions().bindFilter("Editor.Delete"_id) = [this]() { return checkGeneralDelete(); };
            actions().bindShortcut("Editor.Copy"_id, "Ctrl+C");
            actions().bindShortcut("Editor.Cut"_id, "Ctrl+X");
            actions().bindShortcut("Editor.Paste"_id, "Ctrl+V");
            actions().bindShortcut("Editor.Delete"_id, "Del");
        }
    }

    ui::DockLayoutNode& ResourceEditor::dockLayout()
    {
        return m_dock->layout();
    }

    void ResourceEditor::fillFileMenu(ui::MenuButtonContainer* menu)
    {
        if (m_features.test(ResourceEditorFeatureBit::Save))
        {
            menu->createAction("Editor.Save"_id, "Save", "[img:save]");
            menu->createSeparator();
        }

        if (m_features.test(ResourceEditorFeatureBit::Imported))
        {
            menu->createAction("Editor.Reimport"_id, "Reimport...", "[img:import]");
            menu->createSeparator();
        }
    }

    void ResourceEditor::fillEditMenu(ui::MenuButtonContainer* menu)
    {
        if (m_features.test(ResourceEditorFeatureBit::UndoRedo))
        {
            menu->createAction("Editor.Undo"_id, "Undo", "[img:undo]");
            menu->createAction("Editor.Redo"_id, "Redo", "[img:redo]");
            menu->createSeparator();
        }

        if (m_features.test(ResourceEditorFeatureBit::CopyPaste))
        {
            menu->createAction("Editor.Copy"_id, "Copy", "[img:copy]");
            menu->createAction("Editor.Cut"_id, "Cut", "[img:cut]");
            menu->createAction("Editor.Paste"_id, "Paste", "[img:paste]");
            menu->createAction("Editor.Delete"_id, "Delete", "[img:delete]");
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
        if (auto* mainWindow = findMainWindow())
        {
            mainWindow->layout().fillViewMenu(menu);
            menu->createSeparator();
        }
    }

    bool ResourceEditor::initialize()
    {
        return true;
    }

    void ResourceEditor::cleanup()
    {
        destroyAspects();
    }

    void ResourceEditor::update()
    {}

    bool ResourceEditor::modified() const
    {
        return m_file->isModified();
    }

    //--

    void ResourceEditor::updateAspects()
    {
        for (auto& aspect : m_aspects)
            aspect->update();
    }

    void ResourceEditor::createAspects()
    {
        static InplaceArray<SpecificClassType<IResourceEditorAspect>, 30> aspectClasses;
        if (aspectClasses.empty())
            RTTI::GetInstance().enumClasses(aspectClasses);

        for (const auto& aspectClass : aspectClasses)
        {
            if (auto aspect = aspectClass.create())
            {
                aspect->parent(this);

                if (aspect->initialize(this))
                    m_aspects.pushBack(aspect);
            }
        }
    }

    void ResourceEditor::destroyAspects()
    {
        for (int i = m_aspects.lastValidIndex(); i >= 0; --i)
            m_aspects[i]->close();
        m_aspects.clear();
    }

    MainWindow* ResourceEditor::findMainWindow() const
    {
        if (auto window = findParent<MainWindow>())
            return window;

/*        if (auto node = findParent<ui::DockNotebook>())
            if (auto container = node->container())
                return container->findParent<MainWindow>();*/

        return nullptr;
    }

    bool ResourceEditor::showTabContextMenu(const ui::Position& pos)
    {
        InplaceArray<ManagedItem*, 1> depotItems;
        depotItems.pushBack(m_file);

        auto menu = RefNew<ui::MenuButtonContainer>();
        {
            // editor crap
            menu->createCallback("Close", "[img:cross]") = [this]() { m_file->close(); };
            menu->createSeparator();

            // tab locking (prevents accidental close)
            if (tabLocked())
                menu->createCallback("Unlock", "[img:lock_open]") = [this]() { tabLocked(false); };
            else
                menu->createCallback("Lock", "[img:lock]") = [this]() { tabLocked(true); };
            menu->createSeparator();

            // general file crap
            DepotMenuContext context;
            BuildDepotContextMenu(this, *menu, context, depotItems);
        }

        auto ret = RefNew<ui::PopupWindow>();
        ret->attachChild(menu);
        ret->show(this, ui::PopupWindowSetup().autoClose().relativeToCursor().bottomLeft());

        return true;
    }

    void ResourceEditor::close()
    {
        m_file->close();
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
            ui::PostWindowMessage(this, ui::MessageType::Error, "Save"_id, TempString("Failed to save file '{}'", file()->depotPath()));
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

    void ResourceEditor::handleGeneralReimport()
    {
        if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file()))
        {
            InplaceArray<ManagedFileNativeResource*, 1> files;
            files.emplaceBack(nativeFile);

            base::GetService<Editor>()->mainWindow().addReimportFiles(files);
        }
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

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceEditorAspect);
    RTTI_END_TYPE();

    IResourceEditorAspect::IResourceEditorAspect()
    {}

    IResourceEditorAspect::~IResourceEditorAspect()
    {}

    bool IResourceEditorAspect::initialize(ResourceEditor* editor)
    {
        return true;
    }

    void IResourceEditorAspect::update()
    {

    }

    void IResourceEditorAspect::close()
    {
    }

    //--

} // editor

