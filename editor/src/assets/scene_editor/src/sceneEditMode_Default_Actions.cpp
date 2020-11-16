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
#include "sceneContentStructure.h"
#include "scenePreviewContainer.h"

#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiClassPickerBox.h"
#include "base/ui/include/uiRenderer.h"
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

        auto action = CreateSharedPtr<ActionDeleteNode>(std::move(nodesToDelete), this);
        actionHistory()->execute(action);
    }

    static StringView<char> NodeTypeName(SceneContentNodeType type)
    {
        switch (type)
        {
        case SceneContentNodeType::Component: return "component";
        case SceneContentNodeType::Entity: return "entity";
        case SceneContentNodeType::LayerFile: return "layer";
        case SceneContentNodeType::LayerDir: return "group";
        }

        return "Unknown";
    }

    void SceneEditMode_Default::processObjectCopy(const Array<SceneContentNodePtr>& selection)
    {
        if (!m_panel->renderer())
            return;

        if (selection.empty())
            return;

        const auto clipBoardData = BuildClipboardDataFromNodes(selection);
        if (clipBoardData)
        {
            ui::PostWindowMessage(m_panel, ui::MessageType::Info, "CopyPaste"_id,
                TempString("Copied {} {}{}", clipBoardData->data.size(), NodeTypeName(clipBoardData->type),
                    clipBoardData->data.size() == 1 ? "" : "s"));

            m_panel->renderer()->storeObjectToClipboard(clipBoardData);
        }
        else
        {
            ui::PostWindowMessage(m_panel, ui::MessageType::Error, "CopyPaste"_id, "Unable to copy selected objects to clipboard");
        }
    }

    void SceneEditMode_Default::processObjectPaste(const SceneContentNodePtr& context, const SceneContentClipboardDataPtr& data)
    {

    }

    void SceneEditMode_Default::processObjectDuplicate(const Array<SceneContentNodePtr>& selection)
    {

    }

    //---

    struct ShowHideNodeData
    {
        SceneContentNodePtr node;
        bool oldVisState = false;
        bool newVisState = true;
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
        if (!node->localVisibilityFlag())
        {
            if (node->type() == SceneContentNodeType::Entity || node->type() == SceneContentNodeType::Component)
            {
                outNodes.pushBack(AddRef(node));
            }
        }

        for (const auto& child : node->children())
            CollectHiddenNodes(child, outNodes);
    }

    static bool HasHiddenNodes(const SceneContentNode* node)
    {
        if (!node->localVisibilityFlag())
            return true;

        for (const auto& child : node->children())
            if (HasHiddenNodes(child))
                return true;

        return false;
    }

    static bool HasShownNodes(const SceneContentNode* node)
    {
        if (node->localVisibilityFlag())
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
            if (node->localVisibilityFlag())
            {
                auto& entry = data.emplaceBack();
                entry.node = node;
                entry.oldVisState = true;
                entry.newVisState = false;
            }
        }

        if (!data.empty())
        {
            auto action = CreateSharedPtr<ActionShowHideNodes>(std::move(data), this);
            actionHistory()->execute(action);
        }
    }

    void SceneEditMode_Default::processObjectShow(const Array<SceneContentNodePtr>& selection)
    {
        Array< ShowHideNodeData> data;
        data.reserve(selection.size());

        for (const auto& node : selection)
        {
            if (!node->localVisibilityFlag())
            {
                auto& entry = data.emplaceBack();
                entry.node = node;
                entry.oldVisState = false;
                entry.newVisState = true;
            }
        }

        if (!data.empty())
        {
            auto action = CreateSharedPtr<ActionShowHideNodes>(std::move(data), this);
            actionHistory()->execute(action);
        }
    }

    void SceneEditMode_Default::processObjectToggleVis(const Array<SceneContentNodePtr>& selection)
    {
        Array< ShowHideNodeData> data;
        data.reserve(selection.size());

        for (const auto& node : selection)
        {
            auto& entry = data.emplaceBack();
            entry.node = node;
            entry.oldVisState = node->localVisibilityFlag();
            entry.newVisState = !entry.oldVisState;
        }

        if (!data.empty())
        {
            auto action = CreateSharedPtr<ActionShowHideNodes>(std::move(data), this);
            actionHistory()->execute(action);
        }
    }

    //--

    void SceneEditMode_Default::processObjectCut(const Array<SceneContentNodePtr>& selection)
    {
        if (!m_panel->renderer())
            return;

        if (selection.empty())
            return;

        const auto clipBoardData = BuildClipboardDataFromNodes(selection);
        if (clipBoardData)
        {
            ui::PostWindowMessage(m_panel, ui::MessageType::Info, "CopyPaste"_id,
                TempString("Cut {} {}{}", clipBoardData->data.size(), NodeTypeName(clipBoardData->type),
                    clipBoardData->data.size() == 1 ? "" : "s"));

            m_panel->renderer()->storeObjectToClipboard(clipBoardData);
        }
        else
        {
            ui::PostWindowMessage(m_panel, ui::MessageType::Error, "CopyPaste"_id, "Unable to copy selected objects to clipboard");
            return;
        }

        processObjectDeletion(selection);
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
            if (node->canAttach(SceneContentNodeType::Entity))
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
            if (node->canAttach(SceneContentNodeType::Component))
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
                if (node->canAttach(SceneContentNodeType::Entity))
                {
                    canAddEntity = true;

                    if (node->type() == SceneContentNodeType::Entity)
                        canAddEmptyEntity = true;
                }
                
                if (node->canAttach(SceneContentNodeType::Component))
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
            bool canShow = false;
            bool canHide = false;

            for (const auto& node : selection)
            {
                if (node->localVisibilityFlag())
                    canHide = true;
                else
                    canShow = true;
            }

            menu->createCallback("Show", "[img:eye]", "", canShow) = [this, selection]()
            {
                processObjectShow(selection);
            };

            menu->createCallback("Hide", "[img:eye_cross]", "", canHide) = [this, selection]()
            {
                processObjectHide(selection);
            };

            menu->createSeparator();
        }

        //--
        {
            bool canDelete = !selection.empty();
            bool canCopy = !selection.empty();
            for (const auto& node : selection)
            {
                canDelete &= node->canDelete();
                canCopy &= node->canCopy();
            }

            // copy
            menu->createCallback("Copy", "[img:copy]", "Ctrl+C", canCopy) = [this, selection]()
            {
                processObjectCopy(selection);
            };

            // cut
            menu->createCallback("Cut", "[img:cut]", "Ctrl+X", canCopy && canDelete) = [this, selection]()
            {
                processObjectCut(selection);
            };

            // delete
            menu->createCallback("Delete", "[img:delete]", "Del", canDelete) = [this, selection]()
            {
                processObjectDeletion(selection);
            };

            // data to paste ?
            if (m_panel->renderer() && context)
            {
                SceneContentClipboardDataPtr data;
                if (m_panel->renderer()->loadObjectFromClipboard(data))
                {
                    if (!data->data.empty() && context->canAttach(data->type))
                    {
                        menu->createCallback(TempString("Paste ({} {}(s))", data->data.size(), NodeTypeName(data->type)), "[img:paste]", "Ctrl+V") = [this, context, data]()
                        {
                            processObjectPaste(context, data);
                        };
                    }
                }
            }
        }
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

        SceneContentClipboardDataPtr data;
        m_panel->renderer()->loadObjectFromClipboard(data);
        if (!data || data->data.empty())
            return;

        if (!target->canAttach(data->type))
            return;

        processObjectPaste(target, data);
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
        processObjectCopy(selection().keys());
    }

    void SceneEditMode_Default::handleGeneralCut()
    {
        processObjectCut(selection().keys());
    }

    void SceneEditMode_Default::handleGeneralPaste()
    {
        auto context = m_activeNode.lock();
        if (!context)
            return;

        SceneContentClipboardDataPtr data;
        m_panel->renderer()->loadObjectFromClipboard(data);
        if (!data || data->data.empty() || !context->canAttach(data->type))
            return;

        processObjectPaste(context, data);
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
        if (!m_panel->renderer())
            return false;

        const auto hasData = m_panel->renderer()->checkClipboardHasData(SceneContentClipboardData::GetStaticClass());
        return m_activeNode.lock() && hasData;
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

    bool SceneEditMode_Default::handleInternalKeyAction(input::KeyCode key, bool shift, bool alt, bool ctrl)
    {
        if (!shift && !alt && !ctrl)
        {
            if (key == input::KeyCode::KEY_F)
            {
                focusNodes(m_selection.keys());
                return true;
            }
            else if (key == input::KeyCode::KEY_W)
            {
                changeGizmo(SceneGizmoMode::Translation);
                return true;
            }
            else if (key == input::KeyCode::KEY_E)
            {
                changeGizmo(SceneGizmoMode::Rotation);
                return true;
            }
            else if (key == input::KeyCode::KEY_R)
            {
                changeGizmo(SceneGizmoMode::Scale);
                return true;
            }
            else if (key == input::KeyCode::KEY_SPACE)
            {
                changeGizmoNext();
                return true;
            }
            else if (key == input::KeyCode::KEY_LBRACKET)
            {
                changePositionGridSize(-1);
                return true;
            }
            else if (key == input::KeyCode::KEY_RBRACKET)
            {
                changePositionGridSize(1);
                return true;
            }
        }

        if (key == input::KeyCode::KEY_H && !alt)
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

} // ed

