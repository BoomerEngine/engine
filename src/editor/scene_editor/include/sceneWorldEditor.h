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

/// editor for scenes
class EDITOR_SCENE_EDITOR_API SceneWorldEditor : public SceneCommonEditor
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneWorldEditor, SceneCommonEditor);

public:
    SceneWorldEditor(const ResourceInfo& info);
    virtual ~SceneWorldEditor();

protected:
    virtual bool checkGeneralSave() const override;
    virtual bool save() override;
    virtual void cleanup() override;

    virtual void recreateContent() override;

    void cmdBuildWorld();

    SceneContentWorldDirPtr m_rootLayersGroup;
    ScenePlayInEditorPanelPtr m_piePanel;
};

//--

END_BOOMER_NAMESPACE_EX(ed)

