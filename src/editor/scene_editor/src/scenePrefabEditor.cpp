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
#include "sceneContentStructure.h"
#include "sceneEditMode_Default.h"

#include "engine/world/include/worldPrefab.h"
#include "editor/common/include/managedFileFormat.h"
#include "editor/common/include/managedFileNativeResource.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePrefabEditor);
RTTI_END_TYPE();

ScenePrefabEditor::ScenePrefabEditor(ManagedFileNativeResource* file)
    : SceneCommonEditor(file, SceneContentNodeType::PrefabRoot, "Prefabs")
{
}

ScenePrefabEditor::~ScenePrefabEditor()
{}

void ScenePrefabEditor::recreateContent()
{
    if (auto rootNode = rtti_cast<SceneContentPrefabRoot>(m_content->root()))
    {
        rootNode->detachAllChildren();

        SceneContentNodePtr editableNode;
        if (const auto prefabData = rtti_cast<Prefab>(resource()))
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
        if (const auto prefabData = rtti_cast<Prefab>(resource()))
        {
            NodeTemplatePtr rootNode;

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
    virtual bool canOpen(const ManagedFileFormat& format) const override
    {
        return format.nativeResourceClass() == Prefab::GetStaticClass();
    }

    virtual RefPtr<ResourceEditor> createEditor(ManagedFile* file) const override
    {
        if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            return RefNew<ScenePrefabEditor>(nativeFile);

        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(ScenePrefabResourceEditorOpener);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(ed)
