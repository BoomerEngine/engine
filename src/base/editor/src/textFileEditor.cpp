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

    TextFileResourceEditor::TextFileResourceEditor(ManagedFileRawResource* file)
        : ResourceEditor(file, { ResourceEditorFeatureBit::CopyPaste, ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::UndoRedo })
        , m_textFile(file)
    {
        actions().bindCommand("TextEditor.Find"_id) = [this]() { cmdShowFindWindow(); };
        actions().bindCommand("TextEditor.NextFind"_id) = [this]() { cmdNextFind(); };
        actions().bindCommand("TextEditor.PrevFind"_id) = [this]() { cmdPrevFind(); };
        actions().bindShortcut("TextEditor.Find"_id, "Ctrl+F");
        actions().bindShortcut("TextEditor.NextFind"_id, "F3");
        actions().bindShortcut("TextEditor.PrevFind"_id, "Shift+F3");

        {
            auto panel = RefNew<ui::DockPanel>("[img:text] Text", "TextPanel");
            m_editor = panel->createChild<ui::ScintillaTextEditor>();
            dockLayout().attachPanel(panel);
        }
    }

    bool TextFileResourceEditor::modified() const
    {
        return m_editor->isModified();
    }

    bool TextFileResourceEditor::initialize()
    {
        if (!TBaseClass::initialize())
            return false;

        if (!m_editor)
            return false;

        if (auto content = m_textFile->loadContent())
        {
            auto str = StringBuf(content);
            m_editor->text(str);
        }

        return true;
    }

    void TextFileResourceEditor::cleanup()
    {
        TBaseClass::cleanup();

        if (m_editor)
        {
            detachChild(m_editor);
            m_editor.reset();
        }
    }

    bool TextFileResourceEditor::save() 
    {
        auto txt = m_editor->text();
        auto buffer = txt.view().toBuffer();

        if (m_textFile->storeContent(buffer))
        {
            m_editor->markAsSaved();
            return true;
        }

        return false;
    }

    void TextFileResourceEditor::fillEditMenu(ui::MenuButtonContainer* menu)
    {
        TBaseClass::fillEditMenu(menu);
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

        virtual RefPtr<ResourceEditor> createEditor(ManagedFile* file) const override
        {
            if (auto* rawFile = rtti_cast<ManagedFileRawResource>(file))
                return RefNew<TextFileResourceEditor>(rawFile);
            return nullptr;
        }        
    };

    RTTI_BEGIN_TYPE_CLASS(TextFileResourceEditorOpener);
    RTTI_END_TYPE();

    //---

} // editor
