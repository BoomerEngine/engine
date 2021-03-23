/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"

#include "scenePrefabEditor.h"
#include "sceneContentNodes.h"
#include "sceneContentNodesEntity.h"
#include "sceneContentStructure.h"
#include "sceneEditMode_Default.h"

#include "engine/world/include/rawPrefab.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePrefabEditor);
RTTI_END_TYPE();

ScenePrefabEditor::ScenePrefabEditor(const ResourceInfo& info)
    : SceneCommonEditor(info, SceneContentNodeType::PrefabRoot, "Prefabs")
{
    recreateContent();
    refreshEditMode();
}

ScenePrefabEditor::~ScenePrefabEditor()
{}

void ScenePrefabEditor::recreateContent()
{
    if (auto rootNode = rtti_cast<SceneContentPrefabRoot>(m_content->root()))
    {
        rootNode->detachAllChildren();

        SceneContentNodePtr editableNode;
        if (const auto prefabData = rtti_cast<Prefab>(info().resource))
        {
            if (auto root = prefabData->root())
            {
                editableNode = RefNew<SceneContentEntityNode>(StringBuf(root->m_name.view()), root);
                rootNode->attachChildNode(editableNode);
            }
        }

        rootNode->resetModifiedStatus();

        if (editableNode)
            m_defaultEditMode->activeNode(editableNode);
        else
            m_defaultEditMode->activeNode(rootNode);
    }
}

bool ScenePrefabEditor::checkGeneralSave() const
{
    if (const auto& root = m_content->root())
        return root->modified();

    return false;
}

bool ScenePrefabEditor::save()
{
    if (const auto& root = m_content->root())
    {
        if (const auto prefabData = rtti_cast<Prefab>(info().resource))
        {
            RawEntityPtr rootNode;

            if (!root->entities().empty())
            {
                auto content = root->entities()[0];
                rootNode = content->compileDifferentialData();
            }

            prefabData->setup(rootNode);
        }

        if (TBaseClass::save())
        {
            root->resetModifiedStatus();
            return true;
        }
    }

    return false;
}

//---

class ScenePrefabResourceEditorOpener : public IResourceEditorOpener
{
    RTTI_DECLARE_VIRTUAL_CLASS(ScenePrefabResourceEditorOpener, IResourceEditorOpener);

public:
    virtual bool createEditor(ui::IElement* owner, const ResourceInfo& context, ResourceEditorPtr& outEditor) const override final
    {
        if (auto texture = rtti_cast<Prefab>(context.resource))
        {
            outEditor = RefNew<ScenePrefabEditor>(context);
            return true;
        }

        return false;
    }
};

RTTI_BEGIN_TYPE_CLASS(ScenePrefabResourceEditorOpener);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(ed)
