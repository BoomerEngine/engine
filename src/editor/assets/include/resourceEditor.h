/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

#include "engine/ui/include/uiWindow.h"
#include "engine/ui/include/uiDockContainer.h"
#include "engine/ui/include/uiDockPanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

class ResourceReimportPanel;

///---

/// editor "capabilities", controls the basic features
enum class ResourceEditorFeatureBit : uint32_t
{
    Native = FLAG(0), // this is a native resource editor
    Save = FLAG(1), // file can be saved
    UndoRedo = FLAG(2), // show and implement the "undo/redo" stuff, creates the action history
    CopyPaste = FLAG(3), // show the "copy/paste" stuff
};

typedef DirectFlags<ResourceEditorFeatureBit> ResourceEditorFeatureFlags;

///---

/// general resource editor window
class EDITOR_ASSETS_API ResourceEditor : public ui::DockPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceEditor, ui::DockPanel);

public:
    ResourceEditor(const ResourceInfo& info, ResourceEditorFeatureFlags flags, StringView defaultEditorTag = "Common");
    virtual ~ResourceEditor();

    //--

    // editor context information (what are we editing)
    INLINE const ResourceInfo& info() const { return m_info; }

    // window container tag for the resource editor window (ie. some resource type may want to open in separate windows)
    INLINE const StringBuf& containerTag() const { return m_containerTag; }

    // undo/redo action history
    INLINE const ActionHistoryPtr& actionHistory() const { return m_actionHistory; }

    // main dock container
    INLINE ui::DockContainer* dockContainer() const { return m_dock; }

    // main toolbar
    INLINE ui::ToolBar* toolbar() const { return m_toolbar; }

    // main menu bar
    INLINE ui::MenuBar* menubar() const { return m_menubar; }

    //--

    // root layout node for the editor panel
    ui::DockLayoutNode& dockLayout();

    //--

    // check if editor contains modified content
    virtual bool modified() const;

    // cleanup as much as possible, used when editor is closing
    virtual void cleanup();

    // save content edited in this editor
    virtual bool save();

    // tick - called every frame BEFORE ui, can be used to push/pull data around, update internal simulations, etc 
    virtual void update();

    // handle reimport of resource, usually we should apply the data to various parts of the editor
    virtual void reimported(ResourcePtr resource, ResourceMetadataPtr metadata);

    //--

    virtual void configLoad(const ui::ConfigBlock& block) override;
    virtual void configSave(const ui::ConfigBlock& block) const override;

    //--

    // file the standard menu with editor related options
    virtual void fillFileMenu(ui::MenuButtonContainer* menu);
    virtual void fillEditMenu(ui::MenuButtonContainer* menu);
    virtual void fillViewMenu(ui::MenuButtonContainer* menu);
    virtual void fillToolMenu(ui::MenuButtonContainer* menu);

    //--

    // create best editor for given stuff to edit
    static bool CreateEditor(ui::IElement* owner, StringView depotPath, ResourceEditorPtr& outEditor);

    //--

    virtual void handleReimportAction(ResourcePtr resource, ResourceMetadataPtr metadata);
    virtual void handleCloseRequest() override;
        
protected:
    virtual void handleGeneralUndo();
    virtual void handleGeneralRedo();
    virtual void handleGeneralSave();
    virtual void handleGeneralCopy();
    virtual void handleGeneralCut();
    virtual void handleGeneralPaste();
    virtual void handleGeneralDelete();
    virtual void handleGeneralDuplicate();

    virtual bool checkGeneralUndo() const;
    virtual bool checkGeneralRedo() const;
    virtual bool checkGeneralSave() const;
    virtual bool checkGeneralCopy() const;
    virtual bool checkGeneralCut() const;
    virtual bool checkGeneralPaste() const;
    virtual bool checkGeneralDelete() const;
    virtual bool checkGeneralDuplicate() const;

    virtual bool saveCustomFormat();

private:
    ResourceInfo m_info; // editor context information (what are we editing)

    ResourceEditorFeatureFlags m_features; // editor flags & features
 
    ActionHistoryPtr m_actionHistory; // undo/redo action history

    ui::DockContainerPtr m_dock; // master dock area for the editor
    ui::ToolBarPtr m_toolbar; // main toolbar
    ui::MenuBarPtr m_menubar; // main menu

    RefPtr<ResourceReimportPanel> m_reimportPanel;

    StringBuf m_containerTag;

    //--

    ui::Timer m_toolbarTimer;
    virtual void refreshToolbar();

    //--

    bool showTabContextMenu(const ui::Position& pos);

    virtual void close() override final;

    //--

    friend class Editor;
};

///---

/// file opener, provides way to edit file content, usually creates a ResourceEditor
/// NOTE: this is called only if no existing opened editor is found that can open the file
class EDITOR_ASSETS_API IResourceEditorOpener : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_MANAGED_DEPOT)
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IResourceEditorOpener);

public:
    virtual ~IResourceEditorOpener();

    // create editor for given file type, if owner is specified it's legal to display additional UI (otherwise it's silent mode)
    virtual bool createEditor(ui::IElement* owner, const ResourceInfo& context, ResourceEditorPtr& outEditor) const = 0;
};

///---

END_BOOMER_NAMESPACE_EX(ed)
