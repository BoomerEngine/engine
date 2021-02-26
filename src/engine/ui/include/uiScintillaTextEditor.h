/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scintilla\scintilla #]
*
***/

#pragma once

#include "uiElement.h"
#include "uiDragDrop.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

class ScintillaInnerWidget;

/// generic graph editor, requires the "graph data model" instance
class ENGINE_UI_API ScintillaTextEditor : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ScintillaTextEditor, IElement);

public:
    ScintillaTextEditor();
    virtual ~ScintillaTextEditor();

    ///---

    // set text, forces regeneration of the glyph buffer (UTF8)
    void text(const StringBuf& txt);

    // set read only mode
    void readOnly(bool isReadOnly);

    // get text for the whole buffer (UTF8)
    StringBuf text() const;

    // are we in the read only mode ?
    bool isReadOnly() const;

    // is the text modified since last save ?
    bool isModified() const;

    //---

    // get selected text (UTF8)
    StringBuf selectedText() const;

    // do we have selection
    bool hasSelection() const;

    // go to line and character
    void gotoLine(int line);

    // mark as saved
    void markAsSaved();

    //--

    // can we undo ?
    bool canUndo() const;

    // can we redo ?
    bool canRedo() const;

    // undo last operation
    void undo();

    // redo last operation
    void redo();

    //--

private:
    RefPtr<ScintillaInnerWidget> m_scintilla;
    RefPtr<Scrollbar> m_verticalScrollBar;

    void cmdUndo();
    void cmdRedo();
    void cmdCopy();
    void cmdCut();
    void cmdPaste();
    void cmdDelete();
    void cmdSelectAll();
};

END_BOOMER_NAMESPACE_EX(ui)

