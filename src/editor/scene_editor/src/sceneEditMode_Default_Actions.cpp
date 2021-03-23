/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#include "build.h"
#include "sceneEditMode_Default.h"
#include "sceneEditMode_Default_UI.h"
#include "sceneEditMode_Default_Clipboard.h"

#include "sceneContentNodes.h"
#include "sceneContentNodesEntity.h"
#include "sceneContentStructure.h"
#include "scenePreviewContainer.h"
#include "scenePreviewPanel.h"

#include "editor/assets/include/browserDialogs.h"
#include "editor/common/include/utils.h"

#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiClassPickerBox.h"
#include "engine/ui/include/uiRenderer.h"
#include "engine/ui/include/uiInputBox.h"

#include "engine/world/include/worldEntity.h"
#include "engine/world/include/rawEntity.h"
#include "engine/world/include/rawPrefab.h"

#include "core/object/include/rttiResourceReferenceType.h"
#include "core/object/include/actionHistory.h"
#include "core/object/include/action.h"
#include "core/containers/include/path.h"
#include "core/resource/include/indirectTemplate.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

struct AddedNodeData
{
    SceneContentNodePtr parent;
    SceneContentNodePtr child;
};

struct ActionCreateNode : public IAction
{
public:
    ActionCreateNode(Array<AddedNodeData>&& nodes, SceneEditMode_Default* mode)
        : m_nodes(std::move(nodes))
        , m_oldSelection(mode->selection().keys())
        , m_mode(mode)
    {}

    virtual StringID id() const override
    {
        return "CreateNode"_id;
    }

    StringBuf description() const override
    {
        if (m_nodes.size() == 1)
            return TempString("Create node");
        else
            return TempString("Create {} nodes", m_nodes.size());
    }

    virtual bool execute() override
    {
        Array< SceneContentNodePtr> newSelection;

        for (const auto& info : m_nodes)
        {
            info.parent->attachChildNode(info.child);
            newSelection.pushBack(info.child);
        }

        m_mode->changeSelection(newSelection);
        return true;
    }

    virtual bool undo() override
    {
        for (const auto& info : m_nodes)
            info.parent->detachChildNode(info.child);

        m_mode->changeSelection(m_oldSelection);
        return true;
    }

private:
    Array<AddedNodeData> m_nodes;
    Array<SceneContentNodePtr> m_oldSelection;
    SceneEditMode_Default* m_mode = nullptr;
};

struct ActionDeleteNode : public IAction
{
public:
    ActionDeleteNode(Array<AddedNodeData>&& nodes, SceneEditMode_Default* mode)
        : m_nodes(std::move(nodes))
        , m_oldSelection(mode->selection().keys())
        , m_mode(mode)
    {
        auto newSelection = mode->selection();
        for (const auto& node : m_nodes)
            newSelection.remove(node.child);
    }

    virtual StringID id() const override
    {
        return "DeleteNode"_id;
    }

    StringBuf description() const override
    {
        if (m_nodes.size() == 1)
            return TempString("Delete node");
        else
            return TempString("Delete {} nodes", m_nodes.size());
    }

    virtual bool execute() override
    {
        for (const auto& info : m_nodes)
            info.parent->detachChildNode(info.child);

        m_mode->changeSelection(m_newSelection);
        return true;
    }

    virtual bool undo() override
    {
        for (const auto& info : m_nodes)
            info.parent->attachChildNode(info.child);

        m_mode->changeSelection(m_oldSelection);
        return true;
    }

private:
    Array<AddedNodeData> m_nodes;
    Array<SceneContentNodePtr> m_oldSelection;
    Array<SceneContentNodePtr> m_newSelection;
    SceneEditMode_Default* m_mode = nullptr;
};

//--

void SceneEditMode_Default::processObjectDeletion(const Array<SceneContentNodePtr>& selection)
{
    struct DeleteInfo
    {
        SceneContentNodePtr node;
        bool shouldDelete = true;
    };

    HashMap<SceneContentNode*, DeleteInfo> deleteInfo;
    deleteInfo.reserve(selection.size());
    for (const auto& node : selection)
    {
        if (node->canDelete())
        {
            DeleteInfo info;
            info.node = node;
            deleteInfo[node] = info;
        }
    }

    for (auto& info : deleteInfo.values())
    {
        auto parent = info.node->parent();
        while (parent)
        {
            if (auto* parentInfo = deleteInfo.find(parent)) // is parent deleted as well ?
                info.shouldDelete = false; // if parent is deleted we don't have to
            parent = parent->parent();
        }
    }

    Array<AddedNodeData> nodesToDelete;
    nodesToDelete.reserve(selection.size());

    for (const auto& info : deleteInfo.values())
    {
        if (info.shouldDelete)
        {
            auto& node = nodesToDelete.emplaceBack();
            node.parent = AddRef(info.node->parent());
            node.child = info.node;
        }
    }

    if (nodesToDelete.empty())
        return;

    auto action = RefNew<ActionDeleteNode>(std::move(nodesToDelete), this);
    actionHistory()->execute(action);
}

static StringView NodeTypeName(SceneContentNodeType type)
{
    switch (type)
    {
    case SceneContentNodeType::Entity: return "entity";
    case SceneContentNodeType::LayerFile: return "layer";
    case SceneContentNodeType::LayerDir: return "group";
    }

    return "Unknown";
}

void SceneEditMode_Default::processObjectCopy(const Array<SceneContentNodePtr>& selection)
{
    if (!selection.empty())
    {
        if (const auto clipBoardData = BuildClipboardDataFromNodes(selection))
        {
            ui::PostWindowMessage(m_panel, ui::MessageType::Info, "CopyPaste"_id,
                TempString("Copied {} {}{}", clipBoardData->data.size(), NodeTypeName(clipBoardData->type),
                    clipBoardData->data.size() == 1 ? "" : "s"));

            m_panel->clipboard().storeObject(clipBoardData);
        }
        else
        {
            ui::PostWindowMessage(m_panel, ui::MessageType::Error, "CopyPaste"_id, "Unable to copy selected objects to clipboard");
        }
    }
}

struct ActionPastedNode
{
    SceneContentNodePtr parent;
    SceneContentNodePtr child;
};

struct ActionPasteNodes : public IAction
{
public:
    ActionPasteNodes(Array<ActionPastedNode>&& nodes, SceneEditMode_Default* mode)
        : m_nodes(std::move(nodes))
        , m_oldSelection(mode->selection().keys())
        , m_mode(mode)
    {
        auto newSelection = mode->selection();
        for (const auto& node : m_nodes)
            newSelection.remove(node.child);
    }

    virtual StringID id() const override
    {
        return "PasteNodes"_id;
    }

    StringBuf description() const override
    {
        if (m_nodes.size() == 1)
            return TempString("Paste node");
        else
            return TempString("Paste {} nodes", m_nodes.size());
    }

    virtual bool execute() override
    {
        Array< SceneContentNodePtr> newSelection;

        for (const auto& info : m_nodes)
        {
            info.parent->attachChildNode(info.child);
            newSelection.pushBack(info.child);
        }

        m_mode->changeSelection(newSelection);
        return true;
    }

    virtual bool undo() override
    {
        for (const auto& info : m_nodes)
            info.parent->detachChildNode(info.child);

        m_mode->changeSelection(m_oldSelection);
        return true;
    }

private:
    Array<ActionPastedNode> m_nodes;
    Array<SceneContentNodePtr> m_oldSelection;
    Array<SceneContentNodePtr> m_newSelection;
    SceneEditMode_Default* m_mode = nullptr;
};

void SceneEditMode_Default::processObjectPaste(const SceneContentNodePtr& context, const SceneContentClipboardDataPtr& data, SceneContentNodePasteMode mode, const Transform* worldPlacement)
{
    DEBUG_CHECK_RETURN_EX(context, "No context");
    DEBUG_CHECK_RETURN_EX(data, "No data to paset");

    if (!context->canAttach(data->type))
    {
        ui::PostWindowMessage(m_panel, ui::MessageType::Error, "CopyPaste"_id, "Unable to paste data at given target");
        return;
    }

    if (data->data.empty())
    {
        ui::PostWindowMessage(m_panel, ui::MessageType::Error, "CopyPaste"_id, "Nothing to paste");
        return;
    }

    Array<ActionPastedNode> pastedNodes;
    if (data->type == SceneContentNodeType::Entity)
    {
        const auto& baseWorldPlacement = data->data[0]->worldPlacement;

        Transform contextWorldPlacement; // we may be pasting into a layer, etc that has no transform
        if (auto contextPlacedNode = rtti_cast<SceneContentDataNode>(context))
            contextWorldPlacement = contextPlacedNode->cachedLocalToWorldTransform();

        auto loadedData = CloneObject(data);

        HashSet<StringBuf> additionalNames;
        for (const auto& entry : loadedData->data)
        {
            if (entry->name && entry->packedEntityData)
            {
                const auto safeName = context->buildUniqueName(entry->name, false, &additionalNames);
                auto node = RefNew<SceneContentEntityNode>(safeName, entry->packedEntityData);

                if (mode == SceneContentNodePasteMode::Absolute)
                {
                    if (worldPlacement)
                    {
                        auto relativeToBase = entry->worldPlacement / baseWorldPlacement;
                        auto newWorldPlacement = *worldPlacement * relativeToBase;
                        auto placement = (newWorldPlacement / contextWorldPlacement).toEulerTransform();
                        node->changeLocalPlacement(placement);
                    }
                    else
                    {
                        auto placement = (entry->worldPlacement / contextWorldPlacement).toEulerTransform();
                        node->changeLocalPlacement(placement);
                    }                            
                }

                auto& pasteEntry = pastedNodes.emplaceBack();
                pasteEntry.child = node;
                pasteEntry.parent = context;
            }
        }
    }

    if (pastedNodes.empty())
    {
        ui::PostWindowMessage(m_panel, ui::MessageType::Error, "CopyPaste"_id, "Nothing to paste");
        return;
    }

    auto action = RefNew<ActionPasteNodes>(std::move(pastedNodes), this);
    actionHistory()->execute(action);
}

void SceneEditMode_Default::processObjectDuplicate(const Array<SceneContentNodePtr>& selection)
{
    Array<ActionPastedNode> pastedNodes;
    HashMap<const SceneContentNode*, HashSet<StringBuf>> additionalUsedNames;
    pastedNodes.reserve(selection.size());
    additionalUsedNames.reserve(selection.size());

    for (const auto& node : selection)
    {
        auto parentNode = node->parent();
        if (!parentNode)
            continue;

        if (const auto entityNode = rtti_cast<SceneContentEntityNode>(node))
        {
            if (const auto data = entityNode->compiledForCopy())
            {
                auto& parentUsedNames = additionalUsedNames[parentNode];
                auto safeName = parentNode->buildUniqueName(node->name(), false, &parentUsedNames);
                parentUsedNames.insert(safeName);

                auto& entry = pastedNodes.emplaceBack();
                entry.parent = AddRef(parentNode);
                entry.child = RefNew<SceneContentEntityNode>(safeName, data);
            }
        }
    }

    if (!pastedNodes.empty())
    {
        auto action = RefNew<ActionPasteNodes>(std::move(pastedNodes), this);
        actionHistory()->execute(action);
    }
}

//---

struct ShowHideNodeData
{
    SceneContentNodePtr node;
    SceneContentNodeLocalVisibilityState oldVisState = SceneContentNodeLocalVisibilityState::Default;
    SceneContentNodeLocalVisibilityState newVisState = SceneContentNodeLocalVisibilityState::Default;
};

struct ActionShowHideNodes : public IAction
{
public:
    ActionShowHideNodes(Array<ShowHideNodeData>&& nodes, SceneEditMode_Default* mode)
        : m_nodes(std::move(nodes))
        , m_mode(mode)
    {
    }

    virtual StringID id() const override
    {
        return "ShowHideNode"_id;
    }

    StringBuf description() const override
    {
        if (m_nodes.size() == 1)
            return TempString("Change node visiblity");
        else
            return TempString("Change visiblity of {} nodes", m_nodes.size());
    }

    virtual bool execute() override
    {
        for (const auto& info : m_nodes)
            info.node->visibility(info.newVisState);

        return true;
    }

    virtual bool undo() override
    {
        for (const auto& info : m_nodes)
            info.node->visibility(info.oldVisState);

        return true;
    }

private:
    Array<ShowHideNodeData> m_nodes;
    SceneEditMode_Default* m_mode = nullptr;
};

static void CollectHiddenNodes(const SceneContentNode* node, Array<SceneContentNodePtr>& outNodes)
{
    if (!node->visibilityFlagBool())
        if (node->type() == SceneContentNodeType::Entity)
            outNodes.pushBack(AddRef(node));

    for (const auto& child : node->children())
        CollectHiddenNodes(child, outNodes);
}

static bool HasHiddenNodes(const SceneContentNode* node)
{
    if (!node->visibilityFlagBool())
        return true;

    for (const auto& child : node->children())
        if (HasHiddenNodes(child))
            return true;

    return false;
}

static bool HasShownNodes(const SceneContentNode* node)
{
    if (node->visibilityFlagBool())
        return true;

    for (const auto& child : node->children())
        if (HasShownNodes(child))
            return true;

    return false;
}

void SceneEditMode_Default::processUnhideAll()
{
    if (container())
    {
        Array<SceneContentNodePtr> allNodes;
        CollectHiddenNodes(container()->content()->root(), allNodes);

        processObjectShow(allNodes);
    }
}

void SceneEditMode_Default::processObjectHide(const Array<SceneContentNodePtr>& selection)
{
    Array< ShowHideNodeData> data;
    data.reserve(selection.size());

    for (const auto& node : selection)
    {
        if (node->visibilityFlagBool())
        {
            auto& entry = data.emplaceBack();
            entry.node = node;
            entry.oldVisState = node->visibilityFlagRaw();
            entry.newVisState = SceneContentNodeLocalVisibilityState::Hidden;
        }
    }

    if (!data.empty())
    {
        auto action = RefNew<ActionShowHideNodes>(std::move(data), this);
        actionHistory()->execute(action);
    }
}

void SceneEditMode_Default::processObjectShow(const Array<SceneContentNodePtr>& selection)
{
    Array< ShowHideNodeData> data;
    data.reserve(selection.size());

    for (const auto& node : selection)
    {
        if (!node->visibilityFlagBool())
        {
            auto& entry = data.emplaceBack();
            entry.node = node;
            entry.oldVisState = node->visibilityFlagRaw();
            entry.newVisState = SceneContentNodeLocalVisibilityState::Visible;
        }
    }

    if (!data.empty())
    {
        auto action = RefNew<ActionShowHideNodes>(std::move(data), this);
        actionHistory()->execute(action);
    }
}

void SceneEditMode_Default::processObjectToggleVis(const Array<SceneContentNodePtr>& selection)
{
    Array<ShowHideNodeData> data;
    data.reserve(selection.size());

    for (const auto& node : selection)
    {
        auto& entry = data.emplaceBack();
        entry.node = node;
        entry.oldVisState = node->visibilityFlagRaw();
        if (entry.node->visibilityFlagBool())
            entry.newVisState = SceneContentNodeLocalVisibilityState::Hidden;
        else
            entry.newVisState = SceneContentNodeLocalVisibilityState::Visible;
    }

    if (!data.empty())
    {
        auto action = RefNew<ActionShowHideNodes>(std::move(data), this);
        actionHistory()->execute(action);
    }
}

void SceneEditMode_Default::processSaveAsPrefab(const Array<SceneContentNodePtr>& selection)
{
    InplaceArray<SceneContentDataNodePtr, 10> roots;
    ExtractSelectionRoots(selection, roots);

    struct NodeToSave
    {
        RawEntityPtr data;
        Transform localToWorld;
    };

    HashSet<StringBuf> uniqueNames;
    InplaceArray<NodeToSave, 10> nodesToSave;
    nodesToSave.reserve(roots.size());
    for (const auto& rootNode : roots)
    {
        if (auto entity = rtti_cast<SceneContentEntityNode>(rootNode))
        {
            if (auto data = entity->compiledForCopy())
            {
                auto& entry = nodesToSave.emplaceBack();
                entry.data = data;
                entry.localToWorld = entity->cachedLocalToWorldTransform();

                const auto unqiueName = SceneContentNode::BuildUniqueName(rootNode->name(), true, uniqueNames);
                uniqueNames.insert(unqiueName);

                data->m_name = StringID(unqiueName.view());
                data->m_entityTemplate->placement(EulerTransform::IDENTITY());                    
            }
        }
    }

    if (nodesToSave.empty())
    {
        ui::PostWindowMessage(m_panel, ui::MessageType::Error, "Save"_id, "Nothing to save in selected nodes");
        return;
    }

    StringBuf depotPath;
    if (ShowSaveAsFileDialog(m_panel, nullptr, "Enter name of the prefab file:", "prefab", depotPath, ""))
    {
        auto prefab = RefNew<Prefab>();

        if (nodesToSave.size() == 1)
        {
            // save the only node we have directly as root
            auto singleNode = nodesToSave[0].data;
            singleNode->m_name = "default"_id;
            prefab->setup(singleNode);
        }
        else
        {
            // we have more than one entity to save, create a root node
            auto newRootNode = RefNew<RawEntity>();
            newRootNode->m_name = "default"_id;

            // whatever we save the root node has it's transform reset
            const auto referenceTransform = nodesToSave[0].localToWorld;

            // add all nodes
            for (auto i : nodesToSave.indexRange())
            {
                const auto& node = nodesToSave[i];

                // convert transform to local space
                if (i != 0)
                {
                    const auto placement = (node.localToWorld / referenceTransform).toEulerTransform();
                    node.data->m_entityTemplate->placement(placement);
                }

                node.data->parent(newRootNode);
                newRootNode->m_children.pushBack(node.data);
            }

            prefab->setup(newRootNode);
        }

        // save prefab
        auto id = ResourceID::Create();
        if (GetService<DepotService>()->createFile(depotPath, prefab, id))
        {
            ui::PostWindowMessage(m_panel, ui::MessageType::Info, "Save"_id, TempString("Prefab '{}' saved with {} node(s)", depotPath, nodesToSave.size()));
        }
        else
        {
            ui::PostWindowMessage(m_panel, ui::MessageType::Error, "Save"_id, "Failed to save prefab");
        }
    }
}

//--

struct ActionModifyPrefabsNodeData
{
    SceneContentEntityNodePtr node;
    SceneContentEntityInstancedContent oldInstancedContentet;
    SceneContentEntityInstancedContent newInstancedContentet;
};

struct EDITOR_SCENE_EDITOR_API ActionModifyPrefabs : public IAction
{
public:
    ActionModifyPrefabs(Array<ActionModifyPrefabsNodeData>&& nodes, Array<SceneContentNodePtr>&& oldSelection, SceneEditMode_Default* mode)
        : m_nodes(std::move(nodes))
        , m_oldSelection(std::move(oldSelection))
        , m_mode(mode)
    {
        m_newSelection.reserve(m_nodes.size());
        for (const auto& info : m_nodes)
            m_newSelection.pushBack(info.node);
    }

    virtual StringID id() const override
    {
        return "ChangeNodesPrefabs"_id;
    }

    StringBuf description() const override
    {
        return TempString("Change prefab(s)");
    }

    virtual bool execute() override
    {
        for (const auto& info : m_nodes)
            info.node->applyInstancedContent(info.newInstancedContentet);

        m_mode->changeSelection(m_newSelection);
        return true;
    }

    virtual bool undo() override
    {
        for (const auto& info : m_nodes)
            info.node->applyInstancedContent(info.oldInstancedContentet);

        m_mode->changeSelection(m_oldSelection);
        return true;
    }

private:
    Array<SceneContentNodePtr> m_oldSelection;
    Array<SceneContentNodePtr> m_newSelection;
    Array<ActionModifyPrefabsNodeData> m_nodes;
    SceneEditMode_Default* m_mode = nullptr;
};

void SceneEditMode_Default::processUnwrapPrefab(const Array<SceneContentNodePtr>& selection, bool explode)
{
    processGenericPrefabAction(selection, [explode](RawEntity* node)->RawEntityPtr
        {
            return UnpackTopLevelPrefabs(node);
        });
}

void SceneEditMode_Default::processReplaceWithClipboard(const Array<SceneContentNodePtr>& selection)
{

}

//--

void SceneEditMode_Default::processObjectCut(const Array<SceneContentNodePtr>& selection)
{
    if (!selection.empty())
    {
        if (const auto clipBoardData = BuildClipboardDataFromNodes(selection))
        {
            ui::PostWindowMessage(m_panel, ui::MessageType::Info, "CopyPaste"_id,
                TempString("Cut {} {}{}", clipBoardData->data.size(), NodeTypeName(clipBoardData->type),
                    clipBoardData->data.size() == 1 ? "" : "s"));

            m_panel->clipboard().storeObject(clipBoardData);

            processObjectDeletion(selection);
        }
        else
        {
            ui::PostWindowMessage(m_panel, ui::MessageType::Error, "CopyPaste"_id, "Unable to copy selected objects to clipboard");
        }
    }
}

//--

void BindResourceToObjectTemplate(ObjectIndirectTemplate* ptr, StringView depotPath)
{
    DEBUG_CHECK_RETURN_EX(depotPath, "Empty resource path");
    DEBUG_CHECK_RETURN_EX(ValidateDepotFilePath(depotPath), "Invalid resource path");

    if (ptr->templateClass())
    {
        for (const auto& temp : ptr->templateClass()->allTemplateProperties())
        {
            if (temp.editorData.m_primaryResource && temp.type.metaType() == MetaType::ResourceAsyncRef)
            {
                const auto* asyncRefType = static_cast<const IResourceReferenceType*>(temp.type.ptr());
                const auto asyncRefResourceClass = asyncRefType->referenceResourceClass().cast<IResource>();

                if (CanLoadAsClass(depotPath, asyncRefResourceClass))
                {
                    const auto asyncRef = BuildAsyncResourceRef(depotPath, asyncRefResourceClass);
                    ptr->writeProperty(temp.name, &asyncRef, asyncRefType);
                }
            }
        }
    }
}

void SceneEditMode_Default::createEntityAtNodes(const Array<SceneContentNodePtr>& selection, ClassType entityClass, const Transform* initialPlacement, StringView path)
{
    if (!entityClass)
        entityClass = Entity::GetStaticClass();

    DEBUG_CHECK_RETURN(entityClass);
    DEBUG_CHECK_RETURN(!entityClass->isAbstract());

    const auto coreName = path 
        ? StringBuf(path.fileStem()).toLower()
        : StringBuf(entityClass->name().view());
        
    Array<AddedNodeData> createdNodes;
    for (const auto& node : selection)
    {
        if (node->canAttach(SceneContentNodeType::Entity))
        {
            const auto safeName = node->buildUniqueName(coreName);

            auto sourceNode = RefNew<RawEntity>();
            sourceNode->m_entityTemplate = RefNew<ObjectIndirectTemplate>();
            sourceNode->m_entityTemplate->parent(sourceNode);
            sourceNode->m_entityTemplate->templateClass(entityClass);

            if (path)
                BindResourceToObjectTemplate(sourceNode->m_entityTemplate, path);

            if (initialPlacement)
            {
                Transform parentTransform;
                if (auto parentDataNode = rtti_cast<SceneContentDataNode>(node))
                    parentTransform = parentDataNode->cachedLocalToWorldTransform();

                auto placement = (*initialPlacement / parentTransform).toEulerTransform();
                sourceNode->m_entityTemplate->placement(placement);
            }

            auto& info = createdNodes.emplaceBack();
            info.parent = node;
            info.child = RefNew<SceneContentEntityNode>(safeName, sourceNode);
        }
    }

    auto action = RefNew<ActionCreateNode>(std::move(createdNodes), this);
    actionHistory()->execute(action);
}

void SceneEditMode_Default::createPrefabAtNodes(const Array<SceneContentNodePtr>& selection, StringView prefabDepotPath, const Transform* initialPlacement)
{
    DEBUG_CHECK_RETURN_EX(ValidateDepotFilePath(prefabDepotPath), "Invalid prefab");

    const auto prefabData = LoadResourceRef<Prefab>(prefabDepotPath);
    if (!prefabData)
    {
        ui::PostWindowMessage(container(), ui::MessageType::Error, "LoadResource"_id, TempString("Unable to load prefab '{}'", prefabDepotPath));
        return;
    }

    auto coreName = StringBuf(prefabDepotPath.fileStem()).toLower();

    Array<AddedNodeData> createdNodes;
    for (const auto& node : selection)
    {
        if (node->canAttach(SceneContentNodeType::Entity))
        {
            const auto safeName = node->buildUniqueName(coreName);

            auto sourceNode = RefNew<RawEntity>();
            sourceNode->m_entityTemplate = RefNew<ObjectIndirectTemplate>();
            sourceNode->m_entityTemplate->parent(sourceNode);

            if (initialPlacement)
            {
                Transform parentTransform;
                if (auto parentDataNode = rtti_cast<SceneContentDataNode>(node))
                    parentTransform = parentDataNode->cachedLocalToWorldTransform();

                auto placement = (*initialPlacement / parentTransform).toEulerTransform();
                sourceNode->m_entityTemplate->placement(placement);
            }

            auto& prefabInfo = sourceNode->m_prefabAssets.emplaceBack();
            prefabInfo.enabled = true;
            prefabInfo.prefab = prefabData;

            auto& info = createdNodes.emplaceBack();
            info.parent = node;
            info.child = RefNew<SceneContentEntityNode>(safeName, sourceNode);
        }
    }

    auto action = RefNew<ActionCreateNode>(std::move(createdNodes), this);
    actionHistory()->execute(action);
}

/*void SceneEditMode_Default::createEntityWithComponentAtNodes(const Array<SceneContentNodePtr>& selection, ClassType componentClass, const Transform* initialPlacement, const ManagedFile* resourceFile)
{
    DEBUG_CHECK_RETURN(componentClass);
    DEBUG_CHECK_RETURN(!componentClass->isAbstract());

    const auto coreName = ExtractCoreName(componentClass, resourceFile);

    Array<AddedNodeData> createdNodes;
    for (const auto& node : selection)
    {
        if (node->canAttach(SceneContentNodeType::Entity))
        {
            const auto safeName = node->buildUniqueName(coreName);

            //--

            auto sourceNode = RefNew<RawEntity>();
            sourceNode->m_entityTemplate = RefNew<ObjectIndirectTemplate>();
            sourceNode->m_entityTemplate->parent(sourceNode);

            if (initialPlacement)
            {
                Transform parentTransform;
                if (auto parentDataNode = rtti_cast<SceneContentDataNode>(node))
                    parentTransform = parentDataNode->cachedLocalToWorldTransform();

                auto placement = (*initialPlacement / parentTransform).toEulerTransform();
                sourceNode->m_entityTemplate->placement(placement);
            }

            const auto lowerCaseClassName = StringBuf(componentClass->shortName().view().beforeFirstOrFull("Component")).toLower();

            auto& sourceComp = sourceNode->m_behaviorTemplates.emplaceBack();
            sourceComp.name = StringID(lowerCaseClassName);
            sourceComp.data = RefNew<ObjectIndirectTemplate>();
            sourceComp.data->templateClass(componentClass);

            BindResourceToObjectTemplate(sourceComp.data, resourceFile);

            //---

            auto& info = createdNodes.emplaceBack();
            info.parent = node;
            info.child = RefNew<SceneContentEntityNode>(safeName, sourceNode);
        }
    }

    auto action = RefNew<ActionCreateNode>(std::move(createdNodes), this);
    actionHistory()->execute(action);
}

void SceneEditMode_Default::createComponentAtNodes(const Array<SceneContentNodePtr>& selection, ClassType componentClass, const Transform* initialPlacement, const ManagedFile* resourceFile)
{
    DEBUG_CHECK_RETURN(componentClass);
    DEBUG_CHECK_RETURN(!componentClass->isAbstract());

    const auto coreName = ExtractCoreName(componentClass, resourceFile);

    Array<AddedNodeData> createdNodes;
    for (const auto& node : selection)
    {
        if (node->canAttach(SceneContentNodeType::Component))
        {
            const auto safeName = node->buildUniqueName(coreName);

            auto componentData = RefNew<ObjectIndirectTemplate>();
            componentData->templateClass(componentClass);
            BindResourceToObjectTemplate(componentData, resourceFile);

            if (initialPlacement)
            {
                Transform parentTransform;
                if (auto parentDataNode = rtti_cast<SceneContentDataNode>(node))
                    parentTransform = parentDataNode->cachedLocalToWorldTransform();

                auto placement = (*initialPlacement / parentTransform).toEulerTransform();
                componentData->placement(placement);
            }

            auto& info = createdNodes.emplaceBack();
            info.parent = node;
            info.child = RefNew<SceneContentComponentNode>(safeName, componentData);
        }
    }

    auto action = RefNew<ActionCreateNode>(std::move(createdNodes), this);
    actionHistory()->execute(action);
}*/

void SceneEditMode_Default::handleTreeContextMenu(ui::MenuButtonContainer* menu, const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection)
{
    ContextMenuSetup setup;
    setup.contextTreeItem = context;
    setup.selection = selection;
        
    buildContextMenu(menu, setup);
}

void SceneEditMode_Default::handleTreeDeleteNodes(const Array<SceneContentNodePtr>& selection)
{
    processObjectDeletion(selection);
}

void SceneEditMode_Default::handleTreeCutNodes(const Array<SceneContentNodePtr>& selection)
{
    processObjectCut(selection);
}

void SceneEditMode_Default::handleTreeCopyNodes(const Array<SceneContentNodePtr>& selection)
{
    processObjectCopy(selection);
}

void SceneEditMode_Default::handleTreePasteNodes(const SceneContentNodePtr& target, SceneContentNodePasteMode mode)
{
    if (!m_panel->renderer() || !target)
        return;

    if (auto data = m_panel->clipboard().loadObject<SceneContentClipboardData>())
        processObjectPaste(target, data, SceneContentNodePasteMode::Relative);
}

bool SceneEditMode_Default::handleTreeResourceDrop(const SceneContentNodePtr& target, StringView depotFilePath)
{
    return false;
}

struct ActionMoveNodeData
{
    SceneContentNodePtr oldParent;
    SceneContentNodePtr newParent;
    SceneContentNodePtr child;
};

struct EDITOR_SCENE_EDITOR_API ActionMoveNodes : public IAction
{
public:
    ActionMoveNodes(Array<ActionMoveNodeData>&& nodes, Array<SceneContentNodePtr>&& oldSelection, SceneEditMode_Default* mode)
        : m_nodes(std::move(nodes))
        , m_oldSelection(std::move(oldSelection))
        , m_mode(mode)
    {
        m_newSelection.reserve(m_nodes.size());
        for (const auto& info : m_nodes)
            m_newSelection.pushBack(info.child);
    }

    virtual StringID id() const override
    {
        return "MoveNodes"_id;
    }

    StringBuf description() const override
    {
        return TempString("Move nodes(s)");
    }

    virtual bool execute() override
    {
        for (const auto& info : m_nodes)
        {
            info.oldParent->detachChildNode(info.child);
            info.newParent->attachChildNode(info.child);
        }

        m_mode->changeSelection(m_newSelection);
        return true;
    }

    virtual bool undo() override
    {
        for (const auto& info : m_nodes)
        {
            info.newParent->detachChildNode(info.child);
            info.oldParent->attachChildNode(info.child);
        }

        m_mode->changeSelection(m_oldSelection);
        return true;
    }

private:
    Array<SceneContentNodePtr> m_oldSelection;
    Array<SceneContentNodePtr> m_newSelection;
    Array<ActionMoveNodeData> m_nodes;
    SceneEditMode_Default* m_mode = nullptr;
};

bool SceneEditMode_Default::handleTreeNodeDrop(const SceneContentNodePtr& target, const SceneContentNodePtr& source)
{
    Array<ActionMoveNodeData> nodes;

    if (target && source && !source->contains(target))
    {
        if (source->canDelete())
        {
            auto& entry = nodes.emplaceBack();
            entry.child = source;
            entry.oldParent = AddRef(source->parent());
            entry.newParent = target;
        }
    }

    if (nodes.empty())
        return false;

    auto oldSelection = m_selection.keys();
    auto action = RefNew<ActionMoveNodes>(std::move(nodes), std::move(oldSelection), this);
    actionHistory()->execute(action);

    return true;
}

//--

struct ActionSelectNodes : public IAction
{
public:
    ActionSelectNodes(const Array<SceneContentNodePtr>& nodes, SceneEditMode_Default* mode)
        : m_newSelection(nodes)
        , m_oldSelection(mode->selection().keys())
        , m_mode(mode)
    {
    }

    virtual StringID id() const override
    {
        return "SelectNode"_id;
    }

    StringBuf description() const override
    {
        if (m_newSelection.empty())
            return TempString("Clear selection");
        else
            return TempString("Change selection");
    }

    virtual bool execute() override
    {
        m_mode->changeSelection(m_newSelection);
        return true;
    }

    virtual bool undo() override
    {
        m_mode->changeSelection(m_oldSelection);
        return true;
    }

private:
    Array<SceneContentNodePtr> m_oldSelection;
    Array<SceneContentNodePtr> m_newSelection;
    SceneEditMode_Default* m_mode = nullptr;
};

void SceneEditMode_Default::actionChangeSelection(const Array<SceneContentNodePtr>& selection)
{
    auto action = RefNew<ActionSelectNodes>(selection, this);
    actionHistory()->execute(action);
}

//--

void SceneEditMode_Default::handleGeneralDuplicate()
{
    processObjectDuplicate(selection().keys());
}

void SceneEditMode_Default::handleGeneralCopy()
{
    processObjectCopy(selection().keys());
}

void SceneEditMode_Default::handleGeneralCut()
{
    processObjectCut(selection().keys());
}

void SceneEditMode_Default::handleGeneralPaste()
{
    if (auto context = m_activeNode.lock())
    {
        if (auto data = m_panel->clipboard().loadObject<SceneContentClipboardData>())
            if (!data->data.empty() && context->canAttach(data->type))
                processObjectPaste(context, data, SceneContentNodePasteMode::Absolute);
    }
}

void SceneEditMode_Default::handleGeneralDelete()
{
    processObjectDeletion(selection().keys());
}

bool SceneEditMode_Default::checkGeneralCopy() const
{
    return m_canCopySelection;
}

bool SceneEditMode_Default::checkGeneralDuplicate() const
{
    return m_canCopySelection;
}

bool SceneEditMode_Default::checkGeneralCut() const
{
    return m_canCutSelection;
}

bool SceneEditMode_Default::checkGeneralPaste() const
{
    return m_activeNode.lock();
}

bool SceneEditMode_Default::checkGeneralDelete() const
{
    return !selection().empty();
}

//--

// transform store/restore action
struct EDITOR_SCENE_EDITOR_API ActionMoveSceneNodes : public IAction
{
public:
    ActionMoveSceneNodes(Array<ActionMoveSceneNodeData>&& nodes, SceneEditMode_Default* mode, bool fullContentRefresh)
        : m_nodes(std::move(nodes))
        , m_mode(mode)
        , m_fullContentRefresh(fullContentRefresh)
    {
    }

    virtual StringID id() const override
    {
        return "TransformNodes"_id;
    }

    StringBuf description() const override
    {
        if (m_nodes.size() == 1)
            return TempString("Move node");
        else
            return TempString("Move {} nodes", m_nodes.size());
    }

    virtual bool execute() override
    {
        for (const auto& info : m_nodes)
            info.node->changeLocalPlacement(info.newTransform);

        if (m_fullContentRefresh)
            refreshEntityContent();

        m_mode->handleTransformsChanged();
        return true;
    }

    virtual bool undo() override
    {
        for (const auto& info : m_nodes)
            info.node->changeLocalPlacement(info.oldTransform);

        if (m_fullContentRefresh)
            refreshEntityContent();

        m_mode->handleTransformsChanged();
        return true;
    }

    void refreshEntityContent()
    {
        for (const auto& info : m_nodes)
            if (auto* entity = rtti_cast<SceneContentEntityNode>(info.node.get()))
                entity->invalidateData();
    }

private:
    Array<ActionMoveSceneNodeData> m_nodes;
    SceneEditMode_Default* m_mode = nullptr;
    bool m_fullContentRefresh;
};

ActionPtr CreateSceneNodeTransformAction(Array<ActionMoveSceneNodeData>&& nodes, SceneEditMode_Default* mode, bool fullRefresh)
{
    DEBUG_CHECK_RETURN_V(mode, nullptr);
    DEBUG_CHECK_RETURN_V(!nodes.empty(), nullptr);
    return RefNew<ActionMoveSceneNodes>(std::move(nodes), mode, fullRefresh);
}

//--

bool SceneEditMode_Default::handleInternalKeyAction(InputKey key, bool shift, bool alt, bool ctrl)
{
    if (!shift && !alt && !ctrl)
    {
        if (key == InputKey::KEY_F)
        {
            focusNodes(m_selection.keys());
            return true;
        }
        else if (key == InputKey::KEY_W)
        {
            changeGizmo(SceneGizmoMode::Translation);
            return true;
        }
        else if (key == InputKey::KEY_E)
        {
            changeGizmo(SceneGizmoMode::Rotation);
            return true;
        }
        else if (key == InputKey::KEY_R)
        {
            changeGizmo(SceneGizmoMode::Scale);
            return true;
        }
        else if (key == InputKey::KEY_SPACE)
        {
            changeGizmoNext();
            return true;
        }
        else if (key == InputKey::KEY_LBRACKET)
        {
            changePositionGridSize(-1);
            return true;
        }
        else if (key == InputKey::KEY_RBRACKET)
        {
            changePositionGridSize(1);
            return true;
        }
    }

    if (key == InputKey::KEY_H && !alt)
    {
        if (shift && !ctrl)
        {
            processUnhideAll();
            return true;
        }
        else if (ctrl && !shift)
        {
            processObjectShow(m_selection.keys());
            return true;
        }
        else if (!ctrl && !shift)
        {
            processObjectHide(m_selection.keys());
            return true;
        }
    }

    return false;
}

//--

void SceneEditMode_Default::processGenericPrefabAction(const Array<SceneContentNodePtr>& inputNodes, const std::function<RawEntityPtr(RawEntity* currentData)>& func)
{
    auto oldSelection = inputNodes;

    Array<SceneContentDataNodePtr> inputRootNodes;
    ExtractSelectionRoots(inputNodes, inputRootNodes);

    Array<ActionModifyPrefabsNodeData> nodes;
    nodes.reserve(inputRootNodes.size());
    for (const auto& inputNode : inputRootNodes)
    {
        if (auto entity = rtti_cast<SceneContentEntityNode>(inputNode))
        {
            auto& entry = nodes.emplaceBack();

            entry.node = entity;

            auto content = entity->compileDifferentialData();

            entity->extractCurrentInstancedContent(entry.oldInstancedContentet);

            content = func(content);

            entity->createInstancedContent(content, entry.newInstancedContentet);
        }
    }

    if (!nodes.empty())
    {
        auto action = RefNew< ActionModifyPrefabs>(std::move(nodes), std::move(oldSelection), this);
        actionHistory()->execute(action);
    }
}

void SceneEditMode_Default::cmdAddPrefabFile(const Array<SceneContentNodePtr>& inputNodes, StringView depotFilePath)
{
    const auto prefabData = LoadResourceRef<Prefab>(depotFilePath);
    if (!prefabData)
    {
        ui::PostWindowMessage(m_panel, ui::MessageType::Error, "LoadResource"_id, TempString("Unable to load prefab '{}'", depotFilePath));
        return;
    }

    processGenericPrefabAction(inputNodes, [prefabData](RawEntity* node) -> RawEntityPtr
        {
            bool contains = false;
            for (const auto& prefab : node->m_prefabAssets)
            {
                if (prefab.prefab == prefabData)
                {
                    contains = true;
                    break;
                }
            }

            if (!contains)
            {
                auto& entry = node->m_prefabAssets.emplaceBack();
                entry.enabled = true;
                entry.prefab = prefabData;
            }

            return AddRef(node);
        });
}

void SceneEditMode_Default::cmdRemovePrefabFile(const Array<SceneContentNodePtr>& inputNodes, const Array<ResourceID>& ids)
{
    processGenericPrefabAction(inputNodes, [&ids](RawEntity* node) -> RawEntityPtr
        {
            for (auto i : node->m_prefabAssets.indexRange().reversed())
            {
                const auto id = node->m_prefabAssets[i].prefab.id();
                if (ids.contains(id))
                    node->m_prefabAssets.erase(i);
            }

            return AddRef(node);
        });
}

void SceneEditMode_Default::cmdMovePrefabFileUp(const Array<SceneContentNodePtr>& inputNodes, StringView file)
{

}

void SceneEditMode_Default::cmdMovePrefabFileDown(const Array<SceneContentNodePtr>& inputNodes, StringView file)
{

}

void SceneEditMode_Default::cmdEnablePrefabFile(const Array<SceneContentNodePtr>& inputNodes, const Array<ResourceID>& files)
{

}

void SceneEditMode_Default::cmdDisablePrefabFile(const Array<SceneContentNodePtr>& inputNodes, const Array<ResourceID>& files)
{

}

//--

void SceneEditMode_Default::handleContextMenu(ScenePreviewPanel* panel, bool ctrl, bool shift, const ui::Position& absolutePosition, const Point& clientPosition, const Selectable& objectUnderCursor, const ExactPosition* positionUnderCursor)
{
    ContextMenuSetup setup;
    setup.viewportBased = true;
    setup.selection = selection().keys();
    setup.contextClickedItem = container()->resolveSelectable(objectUnderCursor);

    if (positionUnderCursor)
    {
        auto pos = *positionUnderCursor;

        if (container()->gridSettings().positionGridEnabled)
            pos.snap(container()->gridSettings().positionGridSize);

        setup.contextWorldPositionValid = true;
        setup.contextWorldPosition = pos;
    }

    auto menu = RefNew<ui::MenuButtonContainer>();
    buildContextMenu(menu, setup);

    if (auto popup = menu->convertToPopup())
        popup->show(panel, ui::PopupWindowSetup().relativeToCursor().bottomLeft().interactive().autoClose());
}

//--

void SceneEditMode_Default::processCreateLayer(const Array<SceneContentNodePtr>& selection)
{
    static StringBuf name = "layer";
    if (ui::ShowInputBox(m_panel, ui::InputBoxSetup().title("New layer").message("Enter name of new layer:").fileNameValidation(), name))
    {
        Array<AddedNodeData> createdNodes;
        for (const auto& node : selection)
        {
            if (node->canAttach(SceneContentNodeType::LayerFile))
            {
                const auto safeName = node->buildUniqueName(name, true);

                auto& info = createdNodes.emplaceBack();
                info.parent = node;
                info.child = RefNew<SceneContentWorldLayer>(safeName);
            }
        }

        auto action = RefNew<ActionCreateNode>(std::move(createdNodes), this);
        actionHistory()->execute(action);
    }
}

void SceneEditMode_Default::processCreateDirectory(const Array<SceneContentNodePtr>& selection)
{
    static StringBuf name = "dir";
    if (ui::ShowInputBox(m_panel, ui::InputBoxSetup().title("New directory").message("Enter name of new layer directory:").fileNameValidation(), name))
    {
        Array<AddedNodeData> createdNodes;
        for (const auto& node : selection)
        {
            if (node->canAttach(SceneContentNodeType::LayerDir))
            {
                const auto safeName = node->buildUniqueName(name, true);

                auto& info = createdNodes.emplaceBack();
                info.parent = node;
                info.child = RefNew<SceneContentWorldDir>(safeName, false);
            }
        }

        auto action = RefNew<ActionCreateNode>(std::move(createdNodes), this);
        actionHistory()->execute(action);
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)
