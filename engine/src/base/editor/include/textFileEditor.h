/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

#include "singleResourceEditor.h"

namespace ed
{
    ///---

    /// a simple text file editor
    class BASE_EDITOR_API TextFileResourceEditor : public SingleResourceEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TextFileResourceEditor, SingleResourceEditor);
        
    public:
        TextFileResourceEditor(ConfigGroup config, ManagedFile* file);

        virtual bool initialize() override;
        virtual void fillEditMenu(ui::MenuButtonContainer* menu) override;

    private:
        ui::ScintillaTextEditor* m_editor;

        virtual bool saveInternal() override;
        virtual bool modifiedInternal() const override;

        void cmdShowFindWindow();
        void cmdNextFind();
        void cmdPrevFind();
    };

    ///---

} // editor

