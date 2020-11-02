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
    {
        if (auto root = m_structure->root())
        {
            m_rootProxy = MemNew(EditorNodeProxy, root, nullptr);
            m_proxiesMap[root.get()] = m_rootProxy;
        }

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

        m_contentEvents.bind(m_structure->eventKey(), EVENT_CONTENT_STRUCTURE_NODE_RENAMED) = [this](SceneContentNodePtr node)
        {
            handleNodeNameChanged(node);
        };

    }

    SceneContentTreeModel::~SceneContentTreeModel()
    {
        for (auto* proxy : m_proxiesMap.values())
            MemDelete(proxy);

        m_proxiesMap.clear();
        m_rootProxy = nullptr;
    }

    uint32_t SceneContentTreeModel::rowCount(const ui::ModelIndex& parent/*= ui::ModelIndex()*/) const
    {
        if (parent)
        {
            if (auto* proxy = parent.unsafe<EditorNodeProxy>())
            {
                return proxy->m_children.size();
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 1;
        }
    }

    bool SceneContentTreeModel::hasChildren(const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        if (parent)
        {
            if (auto* proxy = parent.unsafe<EditorNodeProxy>())
                return !proxy->m_children.empty();

            return false;
        }
        else
        {
            return true;
        }
    }

    bool SceneContentTreeModel::hasIndex(int row, int col, const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        if (auto* proxy = parent.unsafe<EditorNodeProxy>())
            return row <= proxy->m_children.lastValidIndex();
        return false;
    }

    ui::ModelIndex SceneContentTreeModel::parent(const ui::ModelIndex& item /*= ui::ModelIndex()*/) const
    {
        if (auto* proxy = item.unsafe<EditorNodeProxy>())
            if (proxy->m_parent)
                return proxy->m_parent->index(this);

        return ui::ModelIndex();
    }

    ui::ModelIndex SceneContentTreeModel::index(int row, int column, const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        if (parent)
        {
            if (auto* proxy = parent.unsafe<EditorNodeProxy>())
                if (row <= proxy->m_children.lastValidIndex())
                    return proxy->m_children[row]->index(this);

            return ui::ModelIndex();
        }
        else
        {
            return m_rootProxy->index(this);
        }
    }

    bool SceneContentTreeModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex /*= 0*/) const
    {
        auto* firstProxy = first.unsafe<EditorNodeProxy>();
        auto* secondProxy = first.unsafe<EditorNodeProxy>();
        return firstProxy < secondProxy;
    }

    bool SceneContentTreeModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
    {
        return true;
    }

    base::StringBuf SceneContentTreeModel::displayContent(const ui::ModelIndex& id, int colIndex /*= 0*/) const
    {
        if (auto* proxy = id.unsafe<EditorNodeProxy>())
        {
            if (auto node = proxy->m_node.unsafe())
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
                    return node->visible() ? "[img:eye]" : "[color:#888][img:eye][/color]";
                }
            }
        }

        return base::StringBuf::EMPTY();
    }

    ui::ModelIndex SceneContentTreeModel::indexForNode(const SceneContentNode* node) const
    {
        if (auto* proxy = m_proxiesMap.findSafe(node, nullptr))
            return proxy->index(this);

        return ui::ModelIndex();
    }

    SceneContentNodePtr SceneContentTreeModel::nodeForIndex(const ui::ModelIndex& id) const
    {
        if (auto* proxy = id.unsafe<EditorNodeProxy>())
            return proxy->m_node.lock();
        return nullptr;
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
        if (child)
        {
            if (auto parent = child->parent())
            {
                if (auto* parentProxy = m_proxiesMap.findSafe(parent, nullptr))
                {
                    if (auto* addedChild = parentProxy->addChild(this, child))
                    {
                        m_proxiesMap[child] = addedChild;
                    }
                }
            }
        }
    }

    void SceneContentTreeModel::handleChildNodeDetached(SceneContentNode* child)
    {
        if (auto* proxy = m_proxiesMap.findSafe(child, nullptr))
        {
            if (proxy->m_parent)
            {
                if (auto* removedChild = proxy->m_parent->removeChild(this, child))
                {
                    DEBUG_CHECK(m_proxiesMap[child] == removedChild);
                    m_proxiesMap.remove(child);
                }
            }
        }
    }

    void SceneContentTreeModel::handleNodeVisibilityChanged(SceneContentNode* child)
    {
        if (auto* proxy = m_proxiesMap.findSafe(child, nullptr))
        {
            auto index = proxy->index(this);
            requestItemUpdate(index);
        }
    }

    void SceneContentTreeModel::handleNodeVisualFlagsChanged(SceneContentNode* child)
    {
        if (auto* proxy = m_proxiesMap.findSafe(child, nullptr))
        {
            auto index = proxy->index(this);
            requestItemUpdate(index);
        }
    }

    void SceneContentTreeModel::handleNodeNameChanged(SceneContentNode* child)
    {
        if (auto* proxy = m_proxiesMap.findSafe(child, nullptr))
        {
            auto index = proxy->index(this);
            requestItemUpdate(index);
        }
    }

    //--

    SceneContentTreeModel::EditorNodeProxy::EditorNodeProxy(SceneContentNode* node, EditorNodeProxy* parent)
        : m_node(node)
        , m_parent(parent)
    {

    }

    SceneContentTreeModel::EditorNodeProxy::~EditorNodeProxy()
    {
        for (auto* child : m_children)
            child->m_parent = nullptr;
    }

    int SceneContentTreeModel::EditorNodeProxy::indexInParent() const
    {
        DEBUG_CHECK_RETURN_V(m_parent, 0);
        auto index = m_parent->m_children.find(this);
        DEBUG_CHECK(index != -1);
        return index;
    }

    ui::ModelIndex SceneContentTreeModel::EditorNodeProxy::index(const SceneContentTreeModel* model) const
    {
        if (m_parent)
        {
            auto row = indexInParent();
            return ui::ModelIndex(model, row, 0, this);
        }
        else
        {
            return ui::ModelIndex(model, 0, 0, this);
        }
    }

    SceneContentTreeModel::EditorNodeProxy* SceneContentTreeModel::EditorNodeProxy::addChild(SceneContentTreeModel* model, SceneContentNode* node)
    {
        // TODO: make faster :)

        for (auto* child : m_children)
            if (child->m_node == node)
                return nullptr; // no child created

        auto child = MemNew(EditorNodeProxy, node, this).ptr;

        auto childIndex = m_children.size();
        model->beingInsertRows(index(model), childIndex, 1);
        m_children.pushBack(child);
        model->endInsertRows();

        return child;
    }

    SceneContentTreeModel::EditorNodeProxy* SceneContentTreeModel::EditorNodeProxy::removeChild(SceneContentTreeModel* model, SceneContentNode* node)
    {
        for (auto childIndex : m_children.indexRange())
        {
            auto child = m_children[childIndex];
            if (child->m_node == node)
            {
                model->beingRemoveRows(index(model), childIndex, 1);
                m_children.erase(childIndex);
                model->endRemoveRows();

                return child;
            }
        }

        return nullptr;
    }

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneStructurePanel);
    RTTI_END_TYPE();

    SceneStructurePanel::SceneStructurePanel()
    {
        layoutVertical();

        auto columns = createChild<ui::ColumnHeaderBar>();
        columns->addColumn("Name", 450, false, false, true);
        columns->addColumn("[img:eye]", 40, false, false, false);
        columns->addColumn("[img:save]", 40, false, false, false);

        m_tree = createChild<ui::TreeView>();
        m_tree->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_tree->customVerticalAligment(ui::ElementVerticalLayout::Expand);
        m_tree->columnCount(3);

        m_tree->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
        {
            treeSelectionChanged();
        };

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
    }

    SceneStructurePanel::~SceneStructurePanel()
    {}

    void SceneStructurePanel::bindScene(SceneContentStructure* scene, ScenePreviewContainer* preview)
    {
        m_preview = AddRef(preview);

        if (m_treeModel)
        {
            m_tree->model(nullptr);
            m_treeModel.reset();
        }

        m_scene = AddRef(scene);

        if (m_scene)
        {
            m_treeModel = CreateSharedPtr<SceneContentTreeModel>(scene, preview);
            m_tree->model(m_treeModel);
        }
    }

    void SceneStructurePanel::syncExternalSelection(const Array<SceneContentNodePtr>& nodes)
    {
        if (m_treeModel)
        {
            Array<ui::ModelIndex> indices;
            indices.reserve(nodes.size());

            for (const auto& node : nodes)
                if (auto index = m_treeModel->indexForNode(node))
                    indices.pushBack(index);

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
