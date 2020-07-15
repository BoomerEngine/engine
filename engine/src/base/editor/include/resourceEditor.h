/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

#include "editorConfig.h"

#include "base/ui/include/uiWindow.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/ui/include/uiDockPanel.h"

namespace ed
{
    ///---

    struct AssetItemList;

    typedef base::HashSet<const ManagedFile*> TFileSet;

    /// generate resource editor window
    class BASE_EDITOR_API ResourceEditor : public ui::DockPanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ResourceEditor, ui::DockPanel);

    public:
        ResourceEditor(ConfigGroup config, base::StringView<char> title = "Panel");
        virtual ~ResourceEditor();

        INLINE ConfigGroup& config() { return m_config; }
        INLINE const ConfigGroup& config() const { return m_config; }

        virtual bool initialize() = 0;
        virtual void saveConfig() const = 0;
        virtual bool containsFile(const TFileSet& files) const = 0;
        virtual bool showFile(const TFileSet& files) = 0;
        virtual bool saveFile(const TFileSet& files) = 0;

        virtual void collectOpenedFiles(AssetItemList& outList) const = 0;
        virtual void collectModifiedFiles(AssetItemList& outList) const = 0;

    protected:
        ConfigGroup m_config;

        MainWindow* findMainWindow() const;

        virtual void handleCloseRequest() override final;
    };

    ///---

    /// file opener, provides way to edit file content, usually creates a ResourceEditor
    /// NOTE: this is called only if no existing opened editor is found that can open the file
    class BASE_EDITOR_API IResourceEditorOpener : public base::NoCopy
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IResourceEditorOpener);

    public:
        virtual ~IResourceEditorOpener();

        /// can we open given file FORMAT
        virtual bool canOpen(const ManagedFileFormat& format) const = 0;

        // create editor for given file type
        virtual base::RefPtr<ResourceEditor> createEditor(ConfigGroup config, ManagedFile* file) const = 0;
    };

    ///---

} // editor

