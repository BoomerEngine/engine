/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"
#include "textFileEditor.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "assetBrowser.h"
#include "assetBrowserTabFiles.h"

#include "base/ui/include/uiScintillaTextEditor.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiDockLayout.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(TextFileResourceEditor);
    RTTI_END_TYPE();

    TextFileResourceEditor::TextFileResourceEditor(ConfigGroup config, ManagedFile* file)
        : SingleResourceEditor(config, file)
    {
        actions().bindCommand("TextEditor.Copy"_id) = [this]() { return m_editor->actions().run("Edit.Copy"_id); };
        actions().bindFilter("TextEditor.Copy"_id) = [this]() { return m_editor->hasSelection(); };
        actions().bindCommand("TextEditor.Cut"_id) = [this]() { return m_editor->actions().run("Edit.Cut"_id); };
        actions().bindFilter("TextEditor.Cut"_id) = [this]() { return m_editor->hasSelection(); };
        actions().bindCommand("TextEditor.Paste"_id) = [this]() { return m_editor->actions().run("Edit.Paste"_id); };
        actions().bindFilter("TextEditor.Paste"_id) = [this]() { return true; };
        actions().bindCommand("TextEditor.Delete"_id) = [this]() { return m_editor->actions().run("Edit.Delete"_id); };
        actions().bindFilter("TextEditor.Delete"_id) = [this]() { return m_editor->hasSelection(); };

        actions().bindCommand("TextEditor.Find"_id) = [this]() { cmdShowFindWindow(); };
        actions().bindCommand("TextEditor.NextFind"_id) = [this]() { cmdNextFind(); };
        actions().bindCommand("TextEditor.PrevFind"_id) = [this]() { cmdPrevFind(); };
        actions().bindShortcut("TextEditor.Find"_id, "Ctrl+F");
        actions().bindShortcut("TextEditor.NextFind"_id, "F3");
        actions().bindShortcut("TextEditor.PrevFind"_id, "Shift+F3");

        toolbar()->createButton("TextEditor.Copy"_id, "[img:copy]", "Copy selected text");
        toolbar()->createButton("TextEditor.Cut"_id, "[img:cut]", "Cut selected text");
        toolbar()->createButton("TextEditor.Paste"_id, "[img:paste]", "Paste text");
        toolbar()->createButton("TextEditor.Delete"_id, "[img:delete]", "Delete selected text");

        {
            auto panel = base::CreateSharedPtr<ui::DockPanel>("[img:text] Text", "TextPanel");
            m_editor = panel->createChild<ui::ScintillaTextEditor>();
            dockLayout().attachPanel(panel);
        }
    }

    bool TextFileResourceEditor::initialize()
    {
        TBaseClass::initialize();

        if (!m_editor)
            return false;

        if (auto content = file()->loadRawContent())
        {
            auto str = base::StringBuf(content);
            m_editor->text(str);
        }

        return true;
    }

    void TextFileResourceEditor::fillEditMenu(ui::MenuButtonContainer* menu)
    {
        TBaseClass::fillEditMenu(menu);

        menu->createSeparator();
        menu->createAction("TextEditor.Copy"_id, "Copy", "[img:copy]");
        menu->createAction("TextEditor.Cut"_id, "Cut", "[img:cut]");
        menu->createAction("TextEditor.Paste"_id, "Paste", "[img:paste]");
        menu->createAction("TextEditor.Delete"_id, "Delete", "[img:delete]");
    }

    void TextFileResourceEditor::collectModifiedFiles(AssetItemList& outList) const
    {
        if (m_editor->isModified())
            outList.collectFile(file());
    }

    bool TextFileResourceEditor::saveFile(ManagedFile* fileToSave)
    {
        if (fileToSave == file())
        {
            auto txt = m_editor->text();

            if (file()->storeRawContent(txt))
            {
                m_editor->markAsSaved();
                return true;
            }
        }

        return false;
    }

    void TextFileResourceEditor::cmdShowFindWindow()
    {        
    }

    void TextFileResourceEditor::cmdNextFind()
    {
    }

    void TextFileResourceEditor::cmdPrevFind()
    {
    }

    //---

    class TextFileResourceEditorOpener : public IResourceEditorOpener
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TextFileResourceEditorOpener, IResourceEditorOpener);

    public:
        virtual bool canOpen(const ManagedFileFormat& format) const override
        {
            return format.extension() == "txt" || format.extension() == "md";
        }

        virtual base::RefPtr<ResourceEditor> createEditor(ConfigGroup config, ManagedFile* file) const override
        {
            return base::CreateSharedPtr<TextFileResourceEditor>(config, file);
        }        
    };

    RTTI_BEGIN_TYPE_CLASS(TextFileResourceEditorOpener);
    RTTI_END_TYPE();

    //---

} // editor
