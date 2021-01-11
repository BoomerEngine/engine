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

    /// editor for prefabs
    class ASSETS_SCENE_EDITOR_API ScenePrefabEditor : public SceneCommonEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScenePrefabEditor, SceneCommonEditor);

    public:
        ScenePrefabEditor(ManagedFileNativeResource* file);
        virtual ~ScenePrefabEditor();

    protected:
        virtual bool checkGeneralSave() const override;
        virtual bool save() override;

        virtual void recreateContent() override;
    };

    //--

} // ed
