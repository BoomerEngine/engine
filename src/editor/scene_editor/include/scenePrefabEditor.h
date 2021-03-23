/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "sceneCommonEditor.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

/// editor for prefabs
class EDITOR_SCENE_EDITOR_API ScenePrefabEditor : public SceneCommonEditor
{
    RTTI_DECLARE_VIRTUAL_CLASS(ScenePrefabEditor, SceneCommonEditor);

public:
    ScenePrefabEditor(const ResourceInfo& info);
    virtual ~ScenePrefabEditor();

protected:
    virtual bool checkGeneralSave() const override;
    virtual bool save() override;

    virtual void recreateContent() override;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
