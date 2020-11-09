/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#include "build.h"
#include "sceneStructurePanel.h"
#include "sceneContentStructure.h"
#include "sceneContentNodes.h"
#include "scenePreviewContainer.h"
#include "sceneEditMode.h"

#include "base/ui/include/uiTreeView.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiColumnHeaderBar.h"

namespace ed
{
    //--

    SceneContentTreeModel::SceneContentTreeModel(SceneContentStructure* structure, ScenePreviewContainer* preview)
        : m_structure(structure)
        , m_preview(preview)
        , m_contentEvents(this)
        , m_root(m_structure->root())
    {
        m_contentEvents.bind(m_structure->eventKey(), EVENT_CONTENT_STRUCTURE_NODE_ADDED) = [this](SceneContentNodePtr node)
        {
            handleChildNodeAttached(node);
        };

        m_contentEvents.bind(m_structure->eventKey(), EVENT_CONTENT_STRUCTURE_NODE_REMOVED) = [this](SceneContentNodePtr node)
        {
            handleChildNodeDetached(node);
        };

        m_contentEvents.bind(m_structure->eventKey(), EVENT_CONTENT_STRUCTURE_NODE_VISUAL_FLAG_CHANGED) = [this](SceneContentNodePtr node)
        {
            handleNodeVisualFlagsChanged(node);
        };

        m_contentEvents.bind(m_structure->eventKey(), EVENT_CONTENT_STRUCTURE_NODE_VISIBILITY_CHANGED) = [this](SceneContentNodePtr node)
        {
            handleNodeVisibilityChanged(node);
        };

        m_contentEvents.bind(m_structure->eventKey(), EVENT_CONTENT_STRUCTURE_NODE_MODIFIED_FLAG_CHANGED) = [this](SceneContentNodePtr node)
        {
            handleNodeModifiedFlagsChanged(node);
        };

        m_contentEvents.bind(m_structure->eventKey(), EVENT_CONTENT_STRUCTURE_NODE_RENAMED) = [this](SceneContentNodePtr node)
        {
            handleNodeNameChanged(node);
        };

    }

    SceneContentTreeModel::~SceneContentTreeModel()
    {
        m_root.reset();
    }

    bool SceneContentTreeModel::hasChildren(const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        if (!parent)
            return m_root;

        if (parent.model() == this)
            if (auto* proxy = parent.unsafe<SceneContentNode>())
                return !proxy->children().empty();

        return false;
    }

    void SceneContentTreeModel::children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const
    {
        if (!parent)
        {
            if (m_root)
            {
                auto rootIndex = ui::ModelIndex(this, m_root, m_root->uniqueModelIndex());
                outChildrenIndices.pushBack(rootIndex);
            }
        }
        else if (parent.model() == this)
        {
            if (auto* proxy = parent.unsafe<SceneContentNode>())
            {
                outChildrenIndices.reserve(proxy->children().size());

                for (const auto& child : proxy->children())
                {
                    auto childIndex = ui::ModelIndex(this, child, child->uniqueModelIndex());
                    outChildrenIndices.pushBack(childIndex);
                }
            }
        }
    }

    ui::ModelIndex SceneContentTreeModel::parent(const ui::ModelIndex& item /*= ui::ModelIndex()*/) const
    {
        if (item.model() == this)
            if (auto* proxy = item.unsafe<SceneContentNode>())
                if (auto* parentNode = proxy->parent())
                    return ui::ModelIndex(this, parentNode, parentNode->uniqueModelIndex());

        return ui::ModelIndex();
    }

    bool SceneContentTreeModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex /*= 0*/) const
    {
        if (first.model() == this && second.model() == this)
        {
            const auto* firstProxy = first.unsafe<SceneContentNode>();
            const auto* secondProxy = second.unsafe<SceneContentNode>();
            if (firstProxy && secondProxy)
                return firstProxy->name() < secondProxy->name();
        }

        return first < second;
    }

    bool SceneContentTreeModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
    {
        if (id.model() == this)
            if (auto* proxy = id.unsafe<SceneContentNode>())
                return filter.testString(proxy->name());

        return true;
    }

    base::StringBuf SceneContentTreeModel::displayContent(const ui::ModelIndex& id, int colIndex /*= 0*/) const
    {
        if (id.model() == this)
        {
            if (auto* node = id.unsafe<SceneContentNode>())
            {
                switch (colIndex)
                {
                    case 0:
                    {
                        StringBuilder txt;
                        node->displayText(txt);
                        return txt.toString();
                    }

                    case 1:
                    {
                        return node->visible() ? "[img:eye]" : "[color:#888][img:eye][/color]";
                    }

                    case 2:
                    {
                        if (node->type() == SceneContentNodeType::LayerFile)
                            return node->modified() ? "[img:save]" : "";
                    }
                }
            }
        }

        return base::StringBuf::EMPTY();
    }

    ui::ModelIndex SceneContentTreeModel::indexForNode(const SceneContentNode* node) const
    {
        if (node)
            return ui::ModelIndex(this, node, node->uniqueModelIndex());

        return ui::ModelIndex();
    }

    SceneContentNodePtr SceneContentTreeModel::nodeForIndex(const ui::ModelIndex& id) const
    {
        return id.lock<SceneContentNode>();
    }

    ui::PopupPtr SceneContentTreeModel::contextMenu(ui::AbstractItemView* view, const base::Array<ui::ModelIndex>& indices) const
    {
        if (m_preview)
        {
            if (auto mode = m_preview->mode())
            {
                // resolve current item (under cursor)
                const auto currentNode = nodeForIndex(view->current());

                // gather selected indices
                InplaceArray<SceneContentNodePtr, 10> selectedNodes;
                for (const auto& id : indices)
                    if (auto node = nodeForIndex(id))
                        selectedNodes.pushBack(node);

                auto menu = CreateSharedPtr<ui::MenuButtonContainer>();
                mode->handleTreeContextMenu(menu, currentNode, selectedNodes);
                return menu->convertToPopup();
            }
        }

        return nullptr;
    }

    ui::ElementPtr SceneContentTreeModel::tooltip(ui::AbstractItemView* view, ui::ModelIndex id) const
    {
        return nullptr;
    }

    ui::DragDropDataPtr SceneContentTreeModel::queryDragDropData(const base::input::BaseKeyFlags& keys, const ui::ModelIndex& item)
    {
        return nullptr;
    }

    ui::DragDropHandlerPtr SceneContentTreeModel::handleDragDropData(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data, const ui::Position& pos)
    {
        return nullptr;
    }

    bool SceneContentTreeModel::handleDragDropCompletion(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data)
    {
        return false;
    }

    void SceneContentTreeModel::visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const
    {
        TBaseClass::visualize(item, columnCount, content);
    }

    //--

    void SceneContentTreeModel::handleChildNodeAttached(SceneContentNode* child)
    {
        if (const auto childIndex = indexForNode(child))
            if (const auto parentIndex = indexForNode(child->parent()))
                notifyItemAdded(parentIndex, childIndex);
    }

    void SceneContentTreeModel::handleChildNodeDetached(SceneContentNode* child)
    {
        if (const auto childIndex = indexForNode(child))
            if (const auto parentIndex = indexForNode(child->parent()))
                notifyItemRemoved(parentIndex, childIndex);
    }

    void SceneContentTreeModel::handleNodeVisibilityChanged(SceneContentNode* child)
    {
        if (auto index = indexForNode(child))
            requestItemUpdate(index);
    }

    void SceneContentTreeModel::handleNodeVisualFlagsChanged(SceneContentNode* child)
    {
        if (auto index = indexForNode(child))
            requestItemUpdate(index);
    }

    void SceneContentTreeModel::handleNodeModifiedFlagsChanged(SceneContentNode* child)
    {
        if (auto index = indexForNode(child))
            requestItemUpdate(index);
    }

    void SceneContentTreeModel::handleNodeNameChanged(SceneContentNode* child)
    {
        if (auto index = indexForNode(child))
            requestItemUpdate(index);
    }    

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneStructurePanel);
    RTTI_END_TYPE();

    SceneStructurePanel::SceneStructurePanel(SceneContentStructure* scene, ScenePreviewContainer* preview)
        : m_preview(AddRef(preview))
        , m_scene(AddRef(scene))
    {
        layoutVertical();

        //--

        actions().bindCommand("Tree.Copy"_id) = [this]() { handleTreeObjectCopy(); };
        actions().bindCommand("Tree.Cut"_id) = [this]() { handleTreeObjectCut(); };
        actions().bindCommand("Tree.Delete"_id) = [this]() { handleTreeObjectDeletion(); };
        actions().bindCommand("Tree.Paste"_id) = [this]() { handleTreeObjectPaste(false); };
        actions().bindCommand("Tree.PasteRelative"_id) = [this]() { handleTreeObjectPaste(true); };

        actions().bindShortcut("Tree.Copy"_id, "Ctrl+C");
        actions().bindShortcut("Tree.Cut"_id, "Ctrl+X");
        actions().bindShortcut("Tree.Delete"_id, "Delete");
        actions().bindShortcut("Tree.Paste"_id, "Ctrl+V");
        actions().bindShortcut("Tree.PasteRelative"_id, "Ctrl+Shift+V");

        //--

        auto columns = createChild<ui::ColumnHeaderBar>();
        columns->addColumn("Name", 450, false, false, true);
        columns->addColumn("[img:eye]", 30, true, false, false);
        columns->addColumn("[img:save]", 30, true, false, false);

        m_tree = createChild<ui::TreeView>();
        m_tree->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_tree->customVerticalAligment(ui::ElementVerticalLayout::Expand);
        m_tree->columnCount(3);

        m_tree->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
        {
            treeSelectionChanged();
        };

        //--

        m_treeModel = CreateSharedPtr<SceneContentTreeModel>(scene, preview);
        m_tree->model(m_treeModel);

        //--
    }

    SceneStructurePanel::~SceneStructurePanel()
    {}

    void SceneStructurePanel::syncExternalSelection(const Array<SceneContentNodePtr>& nodes)
    {
        if (m_treeModel)
        {
            Array<ui::ModelIndex> indices;
            indices.reserve(nodes.size());

            for (const auto& node : nodes)
            {
                if (auto index = m_treeModel->indexForNode(node))
                {
                    m_tree->expandItem(index.parent());
                    indices.pushBack(index);
                }
            }

            if (!indices.empty())
                m_tree->ensureVisible(indices.back());

            m_tree->select(indices, ui::ItemSelectionModeBit::Default, false);
        }
    }

    void SceneStructurePanel::treeSelectionChanged()
    {
        if (m_treeModel)
        {
            if (auto mode = m_preview ? m_preview->mode() : nullptr)
            {
                Array<SceneContentNodePtr> selection;
                for (const auto& id : m_tree->selection())
                    if (auto node = m_treeModel->nodeForIndex(id))
                        selection.pushBack(node);

                auto current = m_treeModel->nodeForIndex(m_tree->current());
                mode->handleTreeSelectionChange(current, selection);
            }
        }
    }

    void SceneStructurePanel::handleTreeObjectDeletion()
    {
        if (m_treeModel)
        {
            if (auto mode = m_preview ? m_preview->mode() : nullptr)
            {
                Array<SceneContentNodePtr> selection;
                for (const auto& id : m_tree->selection())
                    if (auto node = m_treeModel->nodeForIndex(id))
                        selection.pushBack(node);

                mode->handleTreeDeleteNodes(selection);
            }
        }
    }

    void SceneStructurePanel::handleTreeObjectCopy()
    {
        if (m_treeModel)
        {
            if (auto mode = m_preview ? m_preview->mode() : nullptr)
            {
                Array<SceneContentNodePtr> selection;
                for (const auto& id : m_tree->selection())
                    if (auto node = m_treeModel->nodeForIndex(id))
                        selection.pushBack(node);

                mode->handleTreeCopyNodes(selection);
            }
        }
    }

    void SceneStructurePanel::handleTreeObjectCut()
    {
        if (m_treeModel)
        {
            if (auto mode = m_preview ? m_preview->mode() : nullptr)
            {
                Array<SceneContentNodePtr> selection;
                for (const auto& id : m_tree->selection())
                    if (auto node = m_treeModel->nodeForIndex(id))
                        selection.pushBack(node);

                mode->handleTreeCutNodes(selection);
            }
        }
    }

    void SceneStructurePanel::handleTreeObjectPaste(bool relative)
    {
        if (m_treeModel)
        {
            if (auto mode = m_preview ? m_preview->mode() : nullptr)
            {
                auto current = m_treeModel->nodeForIndex(m_tree->current());
                mode->handleTreePasteNodes(current, relative ? SceneContentNodePasteMode::Relative : SceneContentNodePasteMode::Absolute);
            }
        }
    }

    //--
    
} // ed
