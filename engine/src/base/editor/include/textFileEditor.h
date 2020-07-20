/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

#include "resourceEditor.h"
#include "managedFileRawResource.h"

namespace ed
{
    ///---

    /// a simple text file editor
    class BASE_EDITOR_API TextFileResourceEditor : public ResourceEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TextFileResourceEditor, ResourceEditor);
        
    public:
        TextFileResourceEditor(ConfigGroup config, ManagedFileRawResource* file);

        virtual bool modified() const override;
        virtual bool initialize() override;
        virtual void close() override;
        virtual bool save() override;

        virtual void fillEditMenu(ui::MenuButtonContainer* menu) override;

    private:
        RefPtr<ui::ScintillaTextEditor> m_editor;
        ManagedFileRawResource* m_textFile = nullptr;

        void cmdShowFindWindow();
        void cmdNextFind();
        void cmdPrevFind();
    };

    ///---

} // editor

