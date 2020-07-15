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
        virtual bool containsFile(const TFileSet& files) const override;
        virtual bool showFile(const TFileSet& files) override;
        virtual bool saveFile(const TFileSet& files) override;

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

        virtual bool saveInternal() = 0;
        virtual bool modifiedInternal() const = 0;

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

    class BASE_EDITOR_API SingleLoadedResourceEditor : public SingleResourceEditor, public base::IObjectObserver 
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SingleLoadedResourceEditor, SingleResourceEditor);

    public:
        SingleLoadedResourceEditor(ConfigGroup config, ManagedFile* file);
        virtual ~SingleLoadedResourceEditor();

        // resource we are editing
        INLINE const base::res::ResourcePtr& resource() const { return m_resource; }

        // show content of this resource in the editor
        void bindResource(const base::res::ResourcePtr& newResource);

        // resource we are editing hash been changed (usually due to reload/reimport)
        virtual void resourceChanged();

    protected:
        base::res::ResourcePtr m_resource; // resource being edited
        uint32_t m_observerToken = 0;

        virtual bool saveInternal() override;
        virtual bool modifiedInternal() const override;

        void detachObserver();
        void attachObserver();

        virtual void onObjectChangedEvent(StringID eventID, const IObject* eventObject, StringView<char> eventPath, const rtti::DataHolder& eventData) override;
    };
    
    ///---
    
    /// file aspect editor
    class BASE_EDITOR_API SingleResourceEditorAspect : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SingleResourceEditorAspect, base::IObject);

    public:
        SingleResourceEditorAspect();

        INLINE SingleResourceEditor* editor() const { return m_editor; }

        virtual bool initialize(SingleResourceEditor* editor);
        virtual void shutdown(); // called when editor is closed
        virtual void resourceChanged();

    private:
        SingleResourceEditor* m_editor;
    };   

    ///---
    
} // editor

