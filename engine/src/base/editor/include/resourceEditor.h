/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

#include "base/ui/include/uiWindow.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/ui/include/uiDockPanel.h"

namespace ed
{
    ///---

    /// editor "capabilities", controls the basic features
    enum class ResourceEditorFeatureBit : uint32_t
    {
        Native = FLAG(0), // this is a native resource editor
        Imported = FLAG(1), // file is imported and not user-created 
        Save = FLAG(2), // file can be saved
        UndoRedo = FLAG(3), // show and implement the "undo/redo" stuff, creates the action history
        CopyPaste = FLAG(4), // show the "copy/paste" stuff
    };

    typedef base::DirectFlags<ResourceEditorFeatureBit> ResourceEditorFeatureFlags;

    ///---

    /// generate resource editor window
    class BASE_EDITOR_API ResourceEditor : public ui::DockPanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ResourceEditor, ui::DockPanel);

    public:
        ResourceEditor(ManagedFile* file, ResourceEditorFeatureFlags flags);
        virtual ~ResourceEditor();

        INLINE ResourceEditorFeatureFlags features() const { return m_features; }

        INLINE ManagedFile* file() const { return m_file; }

        INLINE const Array<ResourceEditorAspectPtr>& aspects() const { return m_aspects; }

        INLINE const ActionHistoryPtr& actionHistory() const { return m_actionHistory; }

        INLINE ui::DockContainer* dockContainer() const { return m_dock; }
        INLINE ui::ToolBar* toolbar() const { return m_toolbar; }
        INLINE ui::MenuBar* menubar() const { return m_menubar; }

        //--

        // root layout node for the editor panel
        ui::DockLayoutNode& dockLayout();

        //--

        // check if editor contains modified content
        virtual bool modified() const;

        // initialize editor, usually loads the content of the file, can fail (editor will not be shown then)
        virtual bool initialize();

        // cleanup as much as possible, used when editor is closing
        virtual void cleanup();

        // save content edited in this editor
        virtual bool save() = 0;

        // tick - called every frame BEFORE ui, can be used to push/pull data around, update internal simulations, etc 
        virtual void update();

        //--

        virtual void configLoad(const ui::ConfigBlock& block) override;
        virtual void configSave(const ui::ConfigBlock& block) const override;

        //--

        // file the standard menu with editor related options
        virtual void fillFileMenu(ui::MenuButtonContainer* menu);
        virtual void fillEditMenu(ui::MenuButtonContainer* menu);
        virtual void fillViewMenu(ui::MenuButtonContainer* menu);
        virtual void fillToolMenu(ui::MenuButtonContainer* menu);
        
    protected:
        virtual void handleGeneralUndo();
        virtual void handleGeneralRedo();
        virtual void handleGeneralSave();
        virtual void handleGeneralCopy();
        virtual void handleGeneralCut();
        virtual void handleGeneralPaste();
        virtual void handleGeneralDelete();
        virtual void handleGeneralReimport();

        virtual bool checkGeneralUndo() const;
        virtual bool checkGeneralRedo() const;
        virtual bool checkGeneralSave() const;
        virtual bool checkGeneralCopy() const;
        virtual bool checkGeneralCut() const;
        virtual bool checkGeneralPaste() const;
        virtual bool checkGeneralDelete() const;

        void updateAspects();

    private:
        ManagedFile* m_file = nullptr; // file being edited

        ResourceEditorFeatureFlags m_features; // editor flags & features
 
        ActionHistoryPtr m_actionHistory; // undo/redo action history

        ui::DockContainerPtr m_dock; // master dock area for the editor
        ui::ToolBarPtr m_toolbar; // main toolbar
        ui::MenuBarPtr m_menubar; // main menu

        Array<ResourceEditorAspectPtr> m_aspects; // created and initialized editor aspects (plugins)

        //--

        void createActions();
        void createAspects();
        void destroyAspects();
        bool showTabContextMenu(const ui::Position& pos);

        MainWindow* findMainWindow() const;
        virtual void close() override final;

        //--

        friend class ManagedFile;
    };

    ///---

    /// resource editor "aspect" - simple plugins for resource editors
    class BASE_EDITOR_API IResourceEditorAspect : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IResourceEditorAspect, IObject);

    public:
        IResourceEditorAspect();
        virtual ~IResourceEditorAspect();

        //---

        // editor we are bound to
        INLINE ResourceEditor* editor() const { return rtti_cast<ResourceEditor>(parent()); }

        //---

        // initialize on given editor, should return true if we can operate or false if we cant
        // NOTE: this is usually called AFTER all main UI for the editor was created and the initial resource data was loaded
        virtual bool initialize(ResourceEditor* editor);

        // called when editor is closed
        virtual void close();

        // update aspect 
        virtual void update();
    };

    ///---

    /// file opener, provides way to edit file content, usually creates a ResourceEditor
    /// NOTE: this is called only if no existing opened editor is found that can open the file
    class BASE_EDITOR_API IResourceEditorOpener : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_MANAGED_DEPOT)
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IResourceEditorOpener);

    public:
        virtual ~IResourceEditorOpener();

        /// can we open given file FORMAT
        virtual bool canOpen(const ManagedFileFormat& format) const = 0;

        // create editor for given file type
        virtual RefPtr<ResourceEditor> createEditor(ManagedFile* file) const = 0;
    };

    ///---

} // editor

