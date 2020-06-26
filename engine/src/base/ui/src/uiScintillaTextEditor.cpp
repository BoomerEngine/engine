/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scintilla\scintilla #]
*
***/

#include "build.h"
#include "uiScintillaTextEditor.h"
#include "uiScintillaInnerWidget.h"
#include "uiScintillaPlatform.h"
#include "base/canvas/include/canvas.h"
#include "uiInputAction.h"
#include "uiScrollBar.h"

using namespace Scintilla;

namespace ui
{

    RTTI_BEGIN_TYPE_CLASS(ScintillaTextEditor);
        RTTI_METADATA(ElementClassNameMetadata).name("Scintilla");
    RTTI_END_TYPE();

    ScintillaTextEditor::ScintillaTextEditor()
    {
        layoutHorizontal();
        enableAutoExpand(true, true);

        // setup widget
        //bindCommands(commandBindings());

        // create scrollbar
        m_verticalScrollBar = base::CreateSharedPtr<ui::Scrollbar>();

        // create internal editor
        m_scintilla = base::CreateSharedPtr<Scintilla::ScintillaInnerWidget>(this, m_verticalScrollBar);
        attachChild(m_scintilla);
        attachChild(m_verticalScrollBar);

        // allow to take whole space
        //m_verticalScrollBar->customProportion(1.0f);
        m_verticalScrollBar->customVerticalAligment(ui::ElementVerticalLayout::Expand);
        m_scintilla->customVerticalAligment(ui::ElementVerticalLayout::Expand);
        m_scintilla->customHorizontalAligment(ui::ElementHorizontalLayout ::Expand);
        m_scintilla->customProportion(1.0f);
        customProportion(1.0f);

        // bind actions
        actions().bindShortcut("Edit.Undo"_id, "Ctrl+Z");
        actions().bindShortcut("Edit.Redo"_id, "Ctrl+Y");
        actions().bindShortcut("Edit.Paste"_id, "Ctrl+V");
        actions().bindShortcut("Edit.Copy"_id, "Ctrl+C");
        actions().bindShortcut("Edit.Cut"_id, "Ctrl+X");
        actions().bindShortcut("Edit.SelectAll"_id, "Ctrl+A");

        actions().bindFilter("Edit.Undo"_id) = [this]() -> bool { return canUndo(); };
        actions().bindFilter("Edit.Redo"_id) = [this]() -> bool { return canRedo(); };

        actions().bindFilter("Edit.Copy"_id) = [this]() -> bool { return hasSelection(); };
        actions().bindFilter("Edit.Cut"_id) = [this]() -> bool { return hasSelection(); };

        actions().bindCommand("Edit.Undo"_id) = [this]() { cmdUndo(); };
        actions().bindCommand("Edit.Redo"_id) = [this]() { cmdRedo(); };
        actions().bindCommand("Edit.Delete"_id) = [this]() { cmdDelete(); };
        actions().bindCommand("Edit.Copy"_id) = [this]() { cmdCopy(); };
        actions().bindCommand("Edit.Cut"_id) = [this]() { cmdCut(); };
        actions().bindCommand("Edit.Paste"_id) = [this]() { cmdPaste(); };
    }

    ScintillaTextEditor::~ScintillaTextEditor()
    {
    }

    //--

    void ScintillaTextEditor::text(const base::StringBuf& txt)
    {
        m_scintilla->WndProc(SCI_SETTEXT, 0, (sptr_t)txt.c_str());
        m_scintilla->WndProc(SCI_EMPTYUNDOBUFFER, 0, 0);
    }

    void ScintillaTextEditor::readOnly(bool isReadOnly)
    {
        m_scintilla->WndProc(SCI_SETREADONLY, isReadOnly, 0);
    }

    bool ScintillaTextEditor::isReadOnly() const
    {
        return m_scintilla->WndProc(SCI_GETREADONLY, 0, 0);
    }

    bool ScintillaTextEditor::isModified() const
    {
        return m_scintilla->WndProc(SCI_GETMODIFY, 0, 0);
    }

    void ScintillaTextEditor::markAsSaved()
    {
        m_scintilla->WndProc(SCI_SETSAVEPOINT, 0, 0);
    }

    base::StringBuf ScintillaTextEditor::text() const
    {
        // allocate buffer
        base::Array<char> buffer;
        buffer.resize(m_scintilla->WndProc(SCI_GETLENGTH, 0, 0) + 1);

        // get text
        m_scintilla->WndProc(SCI_GETTEXT, buffer.dataSize(), (sptr_t)buffer.data());
        return base::StringBuf(buffer.typedData());
    }

    bool ScintillaTextEditor::hasSelection() const
    {
        auto start = m_scintilla->WndProc(SCI_GETSELECTIONSTART, 0, 0);
        auto end = m_scintilla->WndProc(SCI_GETSELECTIONEND, 0, 0);
        return end > start;
    }

    base::StringBuf ScintillaTextEditor::selectedText() const
    {
        auto start = m_scintilla->WndProc(SCI_GETSELECTIONSTART, 0, 0);
        auto end = m_scintilla->WndProc(SCI_GETSELECTIONEND, 0, 0);
        if (end <= start)
            return base::StringBuf::EMPTY();

        // allocate buffer
        base::Array<char> buffer;
        buffer.resize(m_scintilla->WndProc(SCI_GETSELTEXT, 0, 0));

        // get text
        m_scintilla->WndProc(SCI_GETSELTEXT, 0, (sptr_t)buffer.data());
        return base::StringBuf(buffer.typedData());
    }

    void ScintillaTextEditor::gotoLine(int line)
    {
        if (line != -1)
            m_scintilla->WndProc(SCI_GOTOLINE, line, 0);
    }

    bool ScintillaTextEditor::canUndo() const
    {
        return m_scintilla->WndProc(SCI_CANUNDO, 0, 0);
    }

    bool ScintillaTextEditor::canRedo() const
    {
        return m_scintilla->WndProc(SCI_CANREDO, 0, 0);
    }

    void ScintillaTextEditor::undo()
    {
        m_scintilla->WndProc(SCI_UNDO, 0, 0);
    }

    void ScintillaTextEditor::redo()
    {
        m_scintilla->WndProc(SCI_REDO, 0, 0);
    }

    //--

    void ScintillaTextEditor::cmdUndo()
    {
        undo();
    }

    void ScintillaTextEditor::cmdRedo()
    {
        redo();
    }

    void ScintillaTextEditor::cmdCopy()
    {
        m_scintilla->WndProc(SCI_COPY, 0, 0);
    }

    void ScintillaTextEditor::cmdCut()
    {
        m_scintilla->WndProc(SCI_CUT, 0, 0);
    }

    void ScintillaTextEditor::cmdPaste()
    {
        m_scintilla->WndProc(SCI_PASTE, 0, 0);
    }

    void ScintillaTextEditor::cmdDelete()
    {
        //m_scintilla->WndProc(SCI_SELECTALL, 0, 0);
    }

    void ScintillaTextEditor::cmdSelectAll()
    {
        m_scintilla->WndProc(SCI_SELECTALL, 0, 0);
    }

    //--
    
} // ui
