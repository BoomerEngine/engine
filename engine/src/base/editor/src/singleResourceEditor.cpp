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
#include "base/resource_compiler/include/backgroundBakeService.h"
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
            m_toolbar->createButton("Editor.Save"_id, "[img:save]", "Save changed to all modified files");
            m_toolbar->createSeparator();
            m_toolbar->createButton("Editor.Undo"_id, "[img:undo]", "Undo last action");
            m_toolbar->createButton("Editor.Redo"_id, "[img:redo]", "Redo last action");
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

    bool SingleResourceEditor::containsFile(ManagedFile* file) const
    {
        return file == m_file;
    }

    void SingleResourceEditor::collectOpenedFiles(AssetItemList& outList) const
    {
        outList.collectFile(m_file);
    }

    bool SingleResourceEditor::showFile(ManagedFile* file)
    {
        if (m_file == file)
            return true;

        return false;
    }

    bool SingleResourceEditor::saveFile(ManagedFile* file)
    {
        if (m_file == file)
        {
            bool status = true;
            for (const auto& aspect : m_aspects)
                status &= aspect->saveFile(file);
            return status;
        }

        return false;
    }

    void SingleResourceEditor::collectModifiedFiles(AssetItemList& outList) const
    {
        for (const auto& aspect : m_aspects)
            aspect->collectModifiedFiles(outList);
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
        {

        }
    }

    void SingleResourceEditor::cmdRedo()
    {
        if (!m_actionHistory->redo())
        {

        }
    }

    void SingleResourceEditor::cmdSave()
    {
        if (saveFile(file()))
        {

        }
        else
        {

        }
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

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(SingleCookedResourceEditor);
    RTTI_PROPERTY(m_previewResource);
    RTTI_END_TYPE();

    SingleCookedResourceEditor::SingleCookedResourceEditor(ConfigGroup config, ManagedFile* file, base::SpecificClassType<base::res::IResource> mainResourceClass)
        : SingleResourceEditor(config, file)
        , m_mainResourceClass(mainResourceClass)
        , m_key(base::res::ResourcePath(file->depotPath()), mainResourceClass)
    {
        m_bakable = mainResourceClass->findMetadata<base::res::ResourceBakedOnlyMetadata>();

        if (m_bakable)
        {
            actions().bindCommand("Editor.Bake"_id) = [this]() { bakeResource(); };
            actions().bindShortcut("Editor.Bake"_id, "F5");

            toolbar()->createSeparator();
            toolbar()->createButton("Editor.Bake"_id, "[img:cog]", "Bake this resource");
        }

        auto selfRef = base::RefWeakPtr<SingleCookedResourceEditor>(this);
        base::LoadResourceAsync(m_key, [selfRef](const base::res::BaseReference& loadedRef)
        {
            RunSync("RefreshLoadedResource") << [selfRef, loadedRef](FIBER_FUNC)
            {
                if (auto editor = selfRef.lock())
                {
                    editor->m_previewResource = loadedRef.cast<base::res::IResource>();
                    editor->onPropertyChanged("previewResource");
                }
            };
        });
    }

    SingleCookedResourceEditor::~SingleCookedResourceEditor()
    {
    }

    void SingleCookedResourceEditor::previewResourceChanged()
    {
        for (auto& aspect : m_aspects)
            aspect->previewResourceChanged();
    }

    void SingleCookedResourceEditor::onPropertyChanged(StringView<char> path)
    {
        TBaseClass::onPropertyChanged(path);

        if (path == "previewResource")
            previewResourceChanged();
    }

    void SingleCookedResourceEditor::fillToolMenu(ui::MenuButtonContainer* menu)
    {
        menu->createAction("Editor.Bake"_id, "Bake", "[img:cog]");
    }

    void SingleCookedResourceEditor::bakeResource()
    {
        if (m_bakable)
        {
            if (m_currentBakingJob)
                m_currentBakingJob->cancel();

            bool contentSaved = true;
            for (const auto& aspect : m_aspects)
                contentSaved &= aspect->saveFile(file());
            
            if (contentSaved)
                m_currentBakingJob = base::GetService<base::res::BackgroundBaker>()->bake(key(), true, true);
        }
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

    void SingleResourceEditorAspect::previewResourceChanged()
    {}

    void SingleResourceEditorAspect::shutdown()
    {}

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(SingleResourceEditorManifestAspect);
    RTTI_END_TYPE();

    SingleResourceEditorManifestAspect::SingleResourceEditorManifestAspect(base::SpecificClassType<base::res::IResourceManifest> manifestClass)
        : m_manifestClass(manifestClass)
    {}

    void SingleResourceEditorManifestAspect::collectModifiedFiles(AssetItemList& outList) const
    {
        if (m_loadedManifest && m_loadedManifest->modified())
            outList.collectFile(editor()->file());
    }

    bool SingleResourceEditorManifestAspect::saveFile(ManagedFile* file)
    {
        if (file == editor()->file())
        {
            if (!file->storeManifest(m_loadedManifest))
                return false;

            m_loadedManifest->resetModifiedFlag();
        }

        return true;
    }

    bool SingleResourceEditorManifestAspect::modifiedFile(ManagedFile* file) const
    {
        if (file == editor()->file())
            return m_loadedManifest->modified();
        return false;
    }

    bool SingleResourceEditorManifestAspect::initialize(SingleResourceEditor* editor)
    {
        if (!TBaseClass::initialize(editor))
            return false;

        if (auto cookedEditor = base::rtti_cast<SingleCookedResourceEditor>(editor))
        {
            const auto& fileFormat = cookedEditor->file()->fileFormat();

            for (const auto& outputClass : fileFormat.cookableOutputs())
            {
                if (outputClass.resoureClass == cookedEditor->mainResourceClass())
                {
                    if (outputClass.manifestClasses.contains(m_manifestClass))
                    {
                        if (m_loadedManifest = editor->file()->loadManifest(m_manifestClass, true))
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    void SingleResourceEditorManifestAspect::shutdown()
    {
        // nothing
    }

    //--

} // editor

