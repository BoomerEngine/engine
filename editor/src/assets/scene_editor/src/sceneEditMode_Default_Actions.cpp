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
#include "sceneContentNodes.h"
#include "sceneContentStructure.h"
#include "scenePreviewContainer.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiClassPickerBox.h"
#include "base/object/include/actionHistory.h"
#include "base/object/include/action.h"
#include "base/world/include/worldEntityTemplate.h"
#include "base/world/include/worldComponentTemplate.h"


namespace ed
{

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

    static const bool CanAddEmptyEntity(const SceneContentNode* node)
    {
        if (!node)
            return false;

        if (node->type() == SceneContentNodeType::PrefabRoot)
            return true;

        if (node->type() == SceneContentNodeType::Entity)
            return true;

        return false;
    }

    static const bool CanAddEntity(const SceneContentNode* node)
    {
        if (!node)
            return false;

        if (node->type() == SceneContentNodeType::PrefabRoot || node->type() == SceneContentNodeType::LayerFile)
            return true;

        if (node->type() == SceneContentNodeType::Entity)
            return true;

        return false;
    }

    static const bool CanAddComponent(const SceneContentNode* node)
    {
        return (node && node->type() == SceneContentNodeType::Entity);
    }

    static const bool CanDeleteNode(const SceneContentNode* node)
    {
        if (!node)
            return false;

        if (!node->parent())
            return false;

        if (node->type() == SceneContentNodeType::Entity || node->type() == SceneContentNodeType::Component)
        {
            auto dataNode = rtti_cast<SceneContentDataNode>(node);
            return dataNode->baseData() == nullptr;
        }

        return false;
    }

    static const bool CanCopyNode(const SceneContentNode* node)
    {
        if (!node)
            return false;

        if (node->type() == SceneContentNodeType::Entity || node->type() == SceneContentNodeType::Component)
            return true;

        return false;
    }

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
            if (CanDeleteNode(node))
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

        auto action = CreateSharedPtr<ActionDeleteNode>(std::move(nodesToDelete), this);
        actionHistory()->execute(action);
    }

    //--

    void SceneEditMode_Default::createEntityAtNodes(const Array<SceneContentNodePtr>& selection, ClassType entityClass)
    {
        if (!entityClass)
            entityClass = base::world::EntityTemplate::GetStaticClass();

        DEBUG_CHECK_RETURN(entityClass);
        DEBUG_CHECK_RETURN(!entityClass->isAbstract());

        auto coreName = entityClass->name().view();
        coreName = coreName.beforeFirstNoCaseOrFull("Template");
        coreName = coreName.afterLastOrFull("::");
        
        Array<AddedNodeData> createdNodes;
        for (const auto& node : selection)
        {
            if (CanAddEntity(node))
            {
                const auto safeName = node->buildUniqueName(coreName);
                const auto entityData = entityClass->create<base::world::EntityTemplate>();

                Array<RefPtr<SceneContentEntityNodePrefabSource>> prefabs;

                const AbsoluteTransform* placement = &AbsoluteTransform::ROOT();
                if (auto parentData = rtti_cast<SceneContentDataNode>(node))
                    placement = &parentData->localToWorldTransform();

                auto& info = createdNodes.emplaceBack();
                info.parent = node;
                info.child = CreateSharedPtr<SceneContentEntityNode>(safeName, std::move(prefabs), *placement, entityData, nullptr);
            }
        }

        auto action = CreateSharedPtr<ActionCreateNode>(std::move(createdNodes), this);
        actionHistory()->execute(action);
    }

    void SceneEditMode_Default::createComponentAtNodes(const Array<SceneContentNodePtr>& selection, ClassType componentClass)
    {
        DEBUG_CHECK_RETURN(componentClass);
        DEBUG_CHECK_RETURN(!componentClass->isAbstract());

        auto coreName = componentClass->name().view();
        coreName = coreName.beforeFirstNoCaseOrFull("Template");
        coreName = coreName.afterLastOrFull("::");

        Array<AddedNodeData> createdNodes;
        for (const auto& node : selection)
        {
            if (CanAddComponent(node))
            {
                const auto safeName = node->buildUniqueName(coreName);
                const auto entityData = componentClass->create<base::world::ComponentTemplate>();

                const AbsoluteTransform* placement = &AbsoluteTransform::ROOT();
                if (auto parentData = rtti_cast<SceneContentDataNode>(node))
                    placement = &parentData->localToWorldTransform();

                auto& info = createdNodes.emplaceBack();
                info.parent = node;
                info.child = CreateSharedPtr<SceneContentComponentNode>(safeName, *placement, entityData, nullptr);
            }
        }

        auto action = CreateSharedPtr<ActionCreateNode>(std::move(createdNodes), this);
        actionHistory()->execute(action);
    }

    void SceneEditMode_Default::handleTreeContextMenu(ui::MenuButtonContainer* menu, const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection)
    {
        // set active
        if (selection.size() == 1)
        {
            const auto singleNode = selection[0];
            const auto alreadyActive = (m_activeNode == selection[0]);
            menu->createCallback("Make active", alreadyActive ? "[img:star_gray]" : "[img:star]", "", !alreadyActive) = [this, singleNode]()
            {
                activeNode(singleNode);
            };
            menu->createSeparator();
        }

        // "add" 
        {
            bool canAddEntity = false;
            bool canAddEmptyEntity = false;
            bool canAddComponent = false;
            for (const auto& node : selection)
            {
                if (CanAddEntity(node))
                {
                    canAddEntity = true;

                    if (CanAddEmptyEntity(node))
                        canAddEmptyEntity = true;
                }
                
                if (CanAddComponent(node))
                    canAddComponent = true;
            }

            if (canAddEmptyEntity)
            {
                menu->createCallback("Add child", "[img:add]") = [this, selection]()
                {
                    createEntityAtNodes(selection, nullptr);
                };
            }

            if (canAddEntity)
            {
                menu->createSubMenu(m_entityClassSelector, "Add entity", "[img:add]");
            }

            if (canAddComponent)
            {
                menu->createSubMenu(m_componentClassSelector, "Add component", "[img:add]");
            }

            menu->createSeparator();
        }

        //--
        {
            bool canDelete = false;
            bool canCopy = false;
            for (const auto& node : selection)
            {
                if (CanDeleteNode(node))
                {
                    canDelete = true;
                    break;
                }

                if (CanCopyNode(node))
                {
                    canCopy = true;
                    break;
                }
            }

            // copy
            {

            }

            // delete
            menu->createCallback("Delete", "[img:delete]", "Del", canDelete) = [this, selection]()
            {
                processObjectDeletion(selection);
            };
        }
    }

    void SceneEditMode_Default::handleTreeDeleteNodes(const Array<SceneContentNodePtr>& selection)
    {
        processObjectDeletion(selection);
    }

    void SceneEditMode_Default::handleTreeCutNodes(const Array<SceneContentNodePtr>& selection)
    {

    }

    void SceneEditMode_Default::handleTreeCopyNodes(const Array<SceneContentNodePtr>& selection)
    {

    }

    void SceneEditMode_Default::handleTreePasteNodes(const SceneContentNodePtr& target, SceneContentNodePasteMode mode)
    {

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
        auto action = base::CreateSharedPtr<ActionSelectNodes>(selection, this);
        actionHistory()->execute(action);
    }

    //--

    void SceneEditMode_Default::handleGeneralCopy()
    {

    }

    void SceneEditMode_Default::handleGeneralCut()
    {

    }

    void SceneEditMode_Default::handleGeneralPaste()
    {

    }

    void SceneEditMode_Default::handleGeneralDelete()
    {
        processObjectDeletion(selection().keys());
    }

    bool SceneEditMode_Default::checkGeneralCopy() const
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
    struct ASSETS_SCENE_EDITOR_API ActionMoveSceneNodes : public IAction
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
                info.node->changePlacement(info.newTransform);

            if (m_fullContentRefresh)
                refreshEntityContent();

            m_mode->handleTransformsChanged();
            return true;
        }

        virtual bool undo() override
        {
            for (const auto& info : m_nodes)
                info.node->changePlacement(info.oldTransform);

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
        return CreateSharedPtr<ActionMoveSceneNodes>(std::move(nodes), mode, fullRefresh);
    }

    //--

} // ed

