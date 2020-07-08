/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

#include "resourceEditor.h"

#include "base/ui/include/uiWindow.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/ui/include/uiDockPanel.h"

namespace ed
{
    ///---

    class SingleResourceEditorAspect;

    /// single resource editor window, auto docked in the center dock area
    class BASE_EDITOR_API SingleResourceEditor : public ResourceEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SingleResourceEditor, ResourceEditor);
        
    public:
        SingleResourceEditor(ConfigGroup config, ManagedFile* file);
        virtual ~SingleResourceEditor();

        INLINE ManagedFile* file() const { return m_file; }
        INLINE ConfigGroup& fileConfig() { return m_fileConfig; }
        INLINE const ConfigGroup& fileConfig() const { return m_fileConfig; }
        INLINE const base::Array<base::RefPtr<SingleResourceEditorAspect>>& aspects() const { return m_aspects; }

        INLINE const base::ActionHistoryPtr& actionHistory() const { return m_actionHistory; }

        INLINE ui::DockContainer* dockContainer() const { return m_dock; }
        INLINE ui::ToolBar* toolbar() const { return m_toolbar; }
        INLINE ui::MenuBar* menubar() const { return m_menubar; }

        ui::DockLayoutNode& dockLayout();

        virtual bool initialize() override;
        virtual bool uniqueWindow() { return false; }
        virtual bool containsFile(ManagedFile* file) const override;
        virtual bool showFile(ManagedFile* file) override;
        virtual bool saveFile(ManagedFile* file) override;

        virtual void saveConfig() const override;
        virtual void collectOpenedFiles(AssetItemList& outList) const override;
        virtual void collectModifiedFiles(AssetItemList& outList) const override;

        virtual void fillFileMenu(ui::MenuButtonContainer* menu);
        virtual void fillEditMenu(ui::MenuButtonContainer* menu);
        virtual void fillViewMenu(ui::MenuButtonContainer* menu);
        virtual void fillToolMenu(ui::MenuButtonContainer* menu);

    private:
        ManagedFile* m_file; // root file being edited, we may have more (manifests)
        ConfigGroup m_fileConfig; // configuration for file related stuff
        base::ActionHistoryPtr m_actionHistory;

        ui::DockContainerPtr m_dock; // master dock area for the editor
        ui::ToolBarPtr m_toolbar; // main toolbar
        ui::MenuBarPtr m_menubar; // main menu

    protected:
        virtual bool createAspects();
        virtual void destroyAspects();

        void cmdUndo();
        void cmdRedo();
        void cmdSave();

        bool canUndo() const;
        bool canRedo() const;
        bool canSave() const;

        bool showTabContextMenu(const ui::Position& pos);

        base::Array<base::RefPtr<SingleResourceEditorAspect>> m_aspects;

        SingleResourceEditorAspect* findAspect(base::SpecificClassType<SingleResourceEditorAspect> cls) const;
    };

    ///---

    /// editor for class specific resource
    class BASE_EDITOR_API SingleCookedResourceEditor : public SingleResourceEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SingleCookedResourceEditor, SingleResourceEditor);
        
    public:
        SingleCookedResourceEditor(ConfigGroup config, ManagedFile* file, base::SpecificClassType<base::res::IResource> mainResourceClass);
        virtual ~SingleCookedResourceEditor();

        INLINE base::SpecificClassType<base::res::IResource> mainResourceClass() const { return m_mainResourceClass; }
        INLINE const base::res::ResourceKey& key() const { return m_key; }

        INLINE const base::res::Ref<base::res::IResource>& previewResource() const { return m_previewResource; }

        INLINE bool bakable() const { return m_bakable; }

        void bakeResource();

        virtual void previewResourceChanged();

    private:
        base::res::ResourceKey m_key;
        base::SpecificClassType<base::res::IResource> m_mainResourceClass;

        base::res::Ref<base::res::IResource> m_previewResource;
        bool m_bakable = false;

        base::res::BakingJobPtr m_currentBakingJob;

        virtual void fillToolMenu(ui::MenuButtonContainer* menu) override;
        virtual void onPropertyChanged(StringView<char> path) override;
    };

    ///---
    
    /// file aspect editor
    class BASE_EDITOR_API SingleResourceEditorAspect : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SingleResourceEditorAspect, base::IObject);

    public:
        SingleResourceEditorAspect();

        INLINE SingleResourceEditor* editor() const { return m_editor; }

        virtual void collectModifiedFiles(AssetItemList& outList) const = 0;
        virtual bool saveFile(ManagedFile* file) = 0;
        virtual bool modifiedFile(ManagedFile* file) const = 0;

        virtual bool initialize(SingleResourceEditor* editor);
        virtual void shutdown(); // called when editor is closed

        virtual void previewResourceChanged();

    private:
        SingleResourceEditor* m_editor;
    };   

    ///---

    /// manifest file aspect editor
    class BASE_EDITOR_API SingleResourceEditorManifestAspect : public SingleResourceEditorAspect
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SingleResourceEditorManifestAspect, SingleResourceEditorAspect);

    public:
        SingleResourceEditorManifestAspect(base::SpecificClassType<base::res::IResourceManifest> manifestClass);

        INLINE const res::ResourceCookingManifestPtr& manifest() const { return m_loadedManifest; }

        virtual void collectModifiedFiles(AssetItemList& outList) const override;
        virtual bool saveFile(ManagedFile* file) override;
        virtual bool modifiedFile(ManagedFile* file) const override;

        virtual bool initialize(SingleResourceEditor* editor);
        virtual void shutdown(); // called when editor is closed

    private:
        base::SpecificClassType<base::res::IResourceManifest> m_manifestClass;
        res::ResourceCookingManifestPtr m_loadedManifest;
        bool m_manifestChanged = false;
    };

} // editor

