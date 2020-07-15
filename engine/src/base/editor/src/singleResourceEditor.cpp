/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"
#include "editorService.h"
#include "editorConfig.h"
#include "singleResourceEditor.h"
#include "assetBrowser.h"
#include "assetBrowserTabFiles.h"
#include "assetBrowserContextMenu.h"
#include "base/ui/include/uiWindow.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/editor/include/managedFileFormat.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/object/include/actionHistory.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/ui/include/uiDockLayout.h"

namespace ed
{
    
    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(SingleResourceEditor);
    RTTI_END_TYPE();

    SingleResourceEditor::SingleResourceEditor(ConfigGroup config, ManagedFile* file)
        : ResourceEditor(config, file->name())
        , m_file(file)
    {
        layoutVertical();

        // action history
        m_actionHistory = base::CreateSharedPtr<base::ActionHistory>();

        // general actions
        actions().bindCommand("Editor.Save"_id) = [this]() { cmdSave(); };
        actions().bindCommand("Editor.Undo"_id) = [this]() { cmdUndo(); };
        actions().bindCommand("Editor.Redo"_id) = [this]() { cmdRedo(); };
        actions().bindFilter("Editor.Save"_id) = [this]() { return canSave(); };
        actions().bindFilter("Editor.Undo"_id) = [this]() { return canUndo(); };
        actions().bindFilter("Editor.Redo"_id) = [this]() { return canRedo(); };
        actions().bindShortcut("Editor.Save"_id, "Ctrl+S");
        actions().bindShortcut("Editor.Undo"_id, "Ctrl+Z");
        actions().bindShortcut("Editor.Redo"_id, "Ctrl+Y");

        // load config
        {
            base::StringBuf fileHash = base::TempString("{}_{}", Hex(m_file->depotPath().cRC64()), m_file->name().stringBeforeFirst(".", true));
            m_fileConfig = config[fileHash];
        }

        // set title
        {
            base::StringBuf baseTitle = base::TempString("[img:file_edit] {}", file->name());
            title(baseTitle);
        }

        // menu bar
        {
            m_menubar = createChild<ui::MenuBar>();
            m_menubar->createMenu("File", [this]()
                {
                    auto menu = base::CreateSharedPtr<ui::MenuButtonContainer>();
                    fillFileMenu(menu);
                    return menu->convertToPopup();
                });
            m_menubar->createMenu("Edit", [this]()
                {
                    auto menu = base::CreateSharedPtr<ui::MenuButtonContainer>();
                    fillEditMenu(menu);
                    return menu->convertToPopup();
                });
            m_menubar->createMenu("View", [this]()
                {
                    auto menu = base::CreateSharedPtr<ui::MenuButtonContainer>();
                    fillViewMenu(menu);
                    return menu->convertToPopup();
                });
            m_menubar->createMenu("Tools", [this]()
                {
                    auto menu = base::CreateSharedPtr<ui::MenuButtonContainer>();
                    fillToolMenu(menu);
                    return menu->convertToPopup();
                });
        }

        // toolbar
        {
            m_toolbar = createChild<ui::ToolBar>();
            m_toolbar->createButton("Editor.Save"_id, ui::ToolbarButtonSetup().icon("save").caption("Save").tooltip("Save changed to all modified files"));
            m_toolbar->createSeparator();
            m_toolbar->createButton("Editor.Undo"_id, ui::ToolbarButtonSetup().icon("undo").caption("Undo").tooltip("Undo last action"));
            m_toolbar->createButton("Editor.Redo"_id, ui::ToolbarButtonSetup().icon("redo").caption("Redo").tooltip("Redo last action"));
        }

        m_dock = createChild<ui::DockContainer>();
        m_dock->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_dock->customVerticalAligment(ui::ElementVerticalLayout::Expand);

        //--

        bind("OnTabContextMenu"_id, this) = [](SingleResourceEditor* ed, ui::Position pos)
        {
            ed->showTabContextMenu(pos);
        };
    }

    SingleResourceEditor::~SingleResourceEditor()
    {
        destroyAspects();
    }

    void SingleResourceEditor::fillFileMenu(ui::MenuButtonContainer* menu)
    {
        menu->createAction("Editor.Save"_id, "Save", "[img:save]");
    }

    void SingleResourceEditor::fillEditMenu(ui::MenuButtonContainer* menu)
    {
        menu->createAction("Editor.Undo"_id, "Undo", "[img:undo]");
        menu->createAction("Editor.Redo"_id, "Redo", "[img:redo]");
    }

    void SingleResourceEditor::fillViewMenu(ui::MenuButtonContainer* menu)
    {
        m_dock->layout().fillViewMenu(menu);
    }

    void SingleResourceEditor::fillToolMenu(ui::MenuButtonContainer* menu)
    {
        // TODO
    }

    bool SingleResourceEditor::containsFile(const TFileSet& files) const
    {
        return files.contains(m_file);
    }

    void SingleResourceEditor::collectOpenedFiles(AssetItemList& outList) const
    {
        outList.collectFile(m_file);
    }

    void SingleResourceEditor::collectModifiedFiles(AssetItemList& outList) const
    {
        if (modifiedInternal())
            outList.collectFile(m_file);
    }

    bool SingleResourceEditor::showFile(const TFileSet& files)
    {
        return files.contains(m_file);
    }

    bool SingleResourceEditor::saveFile(const TFileSet& files)
    {
        if (files.contains(m_file))
            return saveInternal();
        return false;
    }

    ui::DockLayoutNode& SingleResourceEditor::dockLayout()
    {
        return m_dock->layout();
    }

    bool SingleResourceEditor::initialize()
    {
        createAspects();
        return true;
    }

    SingleResourceEditorAspect* SingleResourceEditor::findAspect(base::SpecificClassType<SingleResourceEditorAspect> cls) const
    {
        for (const auto& aspect : m_aspects)
            if (aspect->cls()->is(cls))
                return aspect;

        return nullptr;
    }

    bool SingleResourceEditor::createAspects()
    {
        base::InplaceArray<base::SpecificClassType<SingleResourceEditorAspect>, 30> aspectClasses;
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
        
        return true;
    }

    void SingleResourceEditor::destroyAspects()
    {
        for (int i = m_aspects.lastValidIndex(); i >= 0; --i)
            m_aspects[i]->shutdown();
        m_aspects.clear();
    }

    void SingleResourceEditor::saveConfig() const
    {

    }

    bool SingleResourceEditor::showTabContextMenu(const ui::Position& pos)
    {
        base::Array<ManagedItem*> depotItems;
        depotItems.pushBack(m_file);

        auto menu = base::CreateSharedPtr<ui::MenuButtonContainer>();
        BuildDepotContextMenu(*menu, nullptr, depotItems);

        auto ret = base::CreateSharedPtr<ui::PopupWindow>();
        ret->attachChild(menu);
        ret->show(this, ui::PopupWindowSetup().autoClose().relativeToCursor().bottomLeft());

        return true;
    }

    void SingleResourceEditor::cmdUndo()
    {
        if (!m_actionHistory->undo())
            ui::PostWindowMessage(this, ui::MessageType::Warning, "UndoRedo"_id, "Undo of last operation has failed");
    }

    void SingleResourceEditor::cmdRedo()
    {
        if (!m_actionHistory->redo())
            ui::PostWindowMessage(this, ui::MessageType::Warning, "UndoRedo"_id, "Redo of last operation has failed");
    }

    void SingleResourceEditor::cmdSave()
    {
        saveInternal();
    }

    bool SingleResourceEditor::canUndo() const
    {
        return m_actionHistory->hasUndo();
    }

    bool SingleResourceEditor::canRedo() const
    {
        return m_actionHistory->hasRedo();
    }

    bool SingleResourceEditor::canSave() const
    {
        AssetItemList tempList;
        collectModifiedFiles(tempList);
        return !tempList.empty();
    }

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SingleLoadedResourceEditor);
        RTTI_PROPERTY(m_resource);
    RTTI_END_TYPE();

    SingleLoadedResourceEditor::SingleLoadedResourceEditor(ConfigGroup config, ManagedFile* file)
        : SingleResourceEditor(config, file)
    {        
    }

    SingleLoadedResourceEditor::~SingleLoadedResourceEditor()
    {
        detachObserver();
    }

    void SingleLoadedResourceEditor::detachObserver()
    {
        if (m_observerToken && m_resource)
        {
            base::IObjectObserver::UnregisterObserver(m_observerToken, this);
            m_observerToken = 0;
        }
    }

    void SingleLoadedResourceEditor::attachObserver()
    {
        DEBUG_CHECK(!m_observerToken);

        if (m_resource)
            m_observerToken = base::IObjectObserver::RegisterObserver(m_resource, this);
    }

    void SingleLoadedResourceEditor::bindResource(const base::res::ResourcePtr& newResource)
    {
        if (m_resource != newResource)
        {
            detachObserver();
            m_resource = newResource;
            attachObserver();

            resourceChanged();

            for (const auto& aspect : m_aspects)
                aspect->resourceChanged();
        }
    }

    void SingleLoadedResourceEditor::resourceChanged()
    {
        // to be implemented in the derived editors
    }

    bool SingleLoadedResourceEditor::saveInternal()
    {
        return file()->storeContent(m_resource);
    }

    bool SingleLoadedResourceEditor::modifiedInternal() const
    {
        return m_resource->modified();
    }

    void SingleLoadedResourceEditor::onObjectChangedEvent(StringID eventID, const IObject* eventObject, StringView<char> eventPath, const rtti::DataHolder& eventData)
    {
        if (eventID == "ResourceModified"_id && eventObject == m_resource)
            file()->markAsModifed();
    }

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(SingleResourceEditorAspect);
    RTTI_END_TYPE();

    SingleResourceEditorAspect::SingleResourceEditorAspect()
    {}

    bool SingleResourceEditorAspect::initialize(SingleResourceEditor* editor)
    {
        m_editor = editor;
        return true;
    }

    void SingleResourceEditorAspect::resourceChanged()
    {}

    void SingleResourceEditorAspect::shutdown()
    {}

    //--

} // editor

