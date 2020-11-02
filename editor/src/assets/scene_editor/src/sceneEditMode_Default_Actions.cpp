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
#include "base/object/include/actionHistory.h"
#include "base/object/include/action.h"


namespace ed
{

    //--

    struct AddedNodeData
    {
        SceneContentNodePtr m_parent;
        SceneContentNodePtr m_child;
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
                info.m_parent->attachChildNode(info.m_child);

            m_mode->changeSelection(newSelection);
            return true;
        }

        virtual bool undo() override
        {
            for (const auto& info : m_nodes)
                info.m_parent->detachChildNode(info.m_child);

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
                newSelection.remove(node.m_child);
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
                info.m_parent->detachChildNode(info.m_child);

            m_mode->changeSelection(m_newSelection);
            return true;
        }

        virtual bool undo() override
        {
            for (const auto& info : m_nodes)
                info.m_parent->attachChildNode(info.m_child);

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

    static const bool CanAddEntity(const SceneContentNode* node)
    {
        if (!node)
            return false;

        if (node->type() == SceneContentNodeType::PrefabRoot || node->type() == SceneContentNodeType::LayerFile)
            return true;

        if (node->type() == SceneContentNodeType::Entity)
        {
            /*if (auto entityNode = rtti_cast<SceneContentEntityNode>(node))
                return entityNode->originalContent(); // can't add a child node to a */
            return true;
        }

        return false;
    }

    static const bool CanDeleteNode(const SceneContentNode* node)
    {
        if (!node)
            return false;

        if (!node->parent())
            return false;

        if (node->type() == SceneContentNodeType::Entity)
        {
            if (auto entityNode = rtti_cast<SceneContentEntityNode>(node))
                return entityNode->originalContent(); // can only delete original content nodes
        }

        return false;
    }

    static const bool CanCopyNode(const SceneContentNode* node)
    {
        if (!node)
            return false;

        if (node->type() == SceneContentNodeType::Entity)
        {
            if (auto entityNode = rtti_cast<SceneContentEntityNode>(node))
                return entityNode->originalContent(); // can only delete original content nodes
        }

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
                node.m_parent = AddRef(info.node->parent());
                node.m_child = info.node;
            }
        }

        if (nodesToDelete.empty())
            return;

        auto action = CreateSharedPtr<ActionDeleteNode>(std::move(nodesToDelete), this);
        actionHistory()->execute(action);
    }

    //--

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
            Array<AddedNodeData> newNodes;
            for (const auto& node : selection)
            {
                if (CanAddEntity(node))
                {
                    auto& info = newNodes.emplaceBack();
                    info.m_parent = node;
                }
            }

            if (!newNodes.empty())
            {
                menu->createCallback("Add empty node", "[img:add]") = [this, newNodes]()
                {
                    const auto coreName = "node";

                    auto newNodesTemp = newNodes;
                    for (auto& info : newNodesTemp)
                    {
                        const auto safeName = info.m_parent->buildUniqueName(coreName);
                        info.m_child = CreateSharedPtr<SceneContentEntityNode>(safeName, Transform::IDENTITY(), Array<game::NodeTemplatePtr>());
                    }

                    auto action = CreateSharedPtr<ActionCreateNode>(std::move(newNodesTemp), this);
                    actionHistory()->execute(action);
                };
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

} // ed
