/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "sceneCommonEditor.h"

namespace ed
{

    //--

    /// editor for scenes
    class ASSETS_SCENE_EDITOR_API SceneWorldEditor : public SceneCommonEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneWorldEditor, SceneCommonEditor);

    public:
        SceneWorldEditor(ManagedFileNativeResource* file);
        virtual ~SceneWorldEditor();

    protected:
        virtual bool checkGeneralSave() const override;
        virtual bool save() override;

        virtual void recreateContent() override;

        SceneContentWorldDirPtr m_rootLayersGroup;
    };

    //--

} // ed
