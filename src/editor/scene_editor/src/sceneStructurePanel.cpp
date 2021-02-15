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

#include "base/world/include/worldPrefab.h"
#include "base/editor/include/assetBrowser.h"
#include "base/editor/include/managedFile.h"
#include "base/editor/include/managedFileFormat.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiSearchBar.h"
#include "base/ui/include/uiComboBox.h"
#include "base/ui/include/uiElementConfig.h"
#include "base/ui/include/uiMessageBox.h"
#include "base/ui/include/uiInputBox.h"
#include "base/ui/include/uiTreeView.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiColumnHeaderBar.h"
#include "base/object/include/object.h"

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
                        if (node->visible())
                        {
                            if (node->visibilityFlagRaw() == SceneContentNodeLocalVisibilityState::Default)
                                return node->defaultVisibilityFlag() ? "[color:#aaa][img:eye][/color]" : "[color:#aaa][img:eye_cross][/color]";
                            else if (node->visibilityFlagRaw() == SceneContentNodeLocalVisibilityState::Visible)
                                return "[img:eye]";
                            else
                                return "[img:eye_cross]";
                        }
                        else
                        {
                            return node->visibilityFlagBool() ? "[color:#555][img:eye][/color]" : "[color:#555][img:eye_cross][/color]";
                        }
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

                auto menu = RefNew<ui::MenuButtonContainer>();
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

    //--

    /// drag&drop data with scene node
    class SceneNodeDragDropData : public ui::IDragDropData
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneNodeDragDropData, ui::IDragDropData);

    public:
        SceneNodeDragDropData(const SceneContentNode* node)
            : m_node(AddRef(node))
        {}

        INLINE const SceneContentNodePtr& data() const { return m_node; }

    private:
        virtual ui::ElementPtr createPreview() const
        {
            StringBuilder caption;
            caption << SceneContentNode::IconTextForType(m_node->type());
            caption << " ";
            caption << m_node->buildHierarchicalName();

            return RefNew<ui::TextLabel>(caption.view());
        }

        SceneContentNodePtr m_node;
    };

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneNodeDragDropData);
    RTTI_END_TYPE();

    //--

    ui::DragDropDataPtr SceneContentTreeModel::queryDragDropData(const base::input::BaseKeyFlags& keys, const ui::ModelIndex& id)
    {
        if (id.model() == this)
            if (auto* node = id.unsafe<SceneContentNode>())
                if (node->type() == SceneContentNodeType::Entity)
                    return RefNew<SceneNodeDragDropData>(node);

        return nullptr;
    }

    ui::DragDropHandlerPtr SceneContentTreeModel::handleDragDropData(ui::AbstractItemView* view, const ui::ModelIndex& id, const ui::DragDropDataPtr& data, const ui::Position& pos)
    {
        if (id.model() == this)
        {
            if (auto* node = id.unsafe<SceneContentNode>())
            {
                if (auto fileData = base::rtti_cast<AssetBrowserFileDragDrop>(data))
                {
                    if (auto file = fileData->file())
                        if (node->canAttach(SceneContentNodeType::Entity))
                            return base::RefNew<ui::DragDropHandlerGeneric>(data, view, pos);
                }
                else if (auto nodeData = base::rtti_cast<SceneNodeDragDropData>(data))
                {
                    if (node->canAttach(nodeData->data()->type()))
                        return base::RefNew<ui::DragDropHandlerGeneric>(data, view, pos);
                }
            }
        }

        return nullptr;
    }

    bool SceneContentTreeModel::handleDragDropCompletion(ui::AbstractItemView* view, const ui::ModelIndex& id, const ui::DragDropDataPtr& data)
    {
        if (id.model() == this)
        {
            if (auto* node = id.unsafe<SceneContentNode>())
            {
                if (m_preview)
                {
                    if (auto mode = m_preview->mode())
                    {
                        if (auto fileData = base::rtti_cast<AssetBrowserFileDragDrop>(data))
                        {
                            if (auto file = fileData->file())
                                return mode->handleTreeResourceDrop(AddRef(node), file);
                        }
                        else if (auto nodeData = base::rtti_cast<SceneNodeDragDropData>(data))
                        {
                            return mode->handleTreeNodeDrop(AddRef(node), nodeData->data());
                        }
                    }
                }
            }
        }

        return false;
    }

    bool SceneContentTreeModel::handleIconClick(const ui::ModelIndex& id, int columnIndex) const
    {
        if (columnIndex == 1)
        {
            if (id.model() == this)
            {
                if (auto* node = id.unsafe<SceneContentNode>())
                {
                    if (node->visibilityFlagBool())
                        node->visibility(SceneContentNodeLocalVisibilityState::Hidden);
                    else
                        node->visibility(SceneContentNodeLocalVisibilityState::Visible);

                    return true;
                }
            }
        }

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

        if (scene->root()->type() == SceneContentNodeType::WorldRoot)
        {
            auto toolbar = createChild<ui::IElement>();
            toolbar->layoutHorizontal();
            toolbar->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            toolbar->customMargins(4, 4, 4, 4);

            {
                auto btn = toolbar->createChild<ui::Button>("[img:save] Save");
                btn->styleType("BackgroundButton"_id);
                btn->tooltip("Save current preset");
                btn->customVerticalAligment(ui::ElementVerticalLayout::Middle);
                btn->customMargins(ui::Offsets(0, 0, 2, 0));
                btn->bind(ui::EVENT_CLICKED) = [this]() { presetSave(); };
            }

            {
                m_presetList = toolbar->createChild<ui::ComboBox>();
                m_presetList->expand();
                m_presetList->bind(ui::EVENT_COMBO_SELECTED) = [this]() { presetSelect(); };
            }

            {
                auto btn = toolbar->createChild<ui::Button>("[img:table_add]");
                btn->styleType("BackgroundButton"_id);
                btn->tooltip("Add new preset using current state of scene");
                btn->customVerticalAligment(ui::ElementVerticalLayout::Middle);
                btn->customMargins(ui::Offsets(2, 0, 2, 0));
                btn->bind(ui::EVENT_CLICKED) = [this]() { presetAddNew(); };
            }

            {
                auto btn = toolbar->createChild<ui::Button>("[img:table_delete]");
                btn->styleType("BackgroundButton"_id);
                btn->customVerticalAligment(ui::ElementVerticalLayout::Middle);
                btn->tooltip("Remove current preset");
                btn->customMargins(ui::Offsets(0, 0, 0, 0));
                btn->bind(ui::EVENT_CLICKED) = [this]() { presetRemove(); };
            }
        }

        //--

        m_searchBar = createChild<ui::SearchBar>();
        m_searchBar->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

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

        m_treeModel = RefNew<SceneContentTreeModel>(scene, preview);
        m_tree->model(m_treeModel);

        //--
    }

    SceneStructurePanel::~SceneStructurePanel()
    {}

    void SceneStructurePanel::presetSave(const ui::ConfigBlock& block) const
    {
        if (m_presetList)
        {
            // save list of presets and selected one
            Array<StringBuf> presetNames;
            for (const auto* preset : m_presets)
                presetNames.pushBack(preset->name);
            block.write("PresetNames", presetNames);
            block.write("ActivePreset", m_presetActive ? m_presetActive->name : StringBuf::EMPTY());

            // write the preset states
            for (auto* preset : m_presets)
            {
                auto localBlock = block.tag(preset->name);
                localBlock.write("ShownNodes", preset->shownNodes);
                localBlock.write("HiddenNodes", preset->hiddenNodes);
            }
        }
    }

    void SceneStructurePanel::presetLoad(const ui::ConfigBlock& block)
    {
        if (m_presetList)
        {
            // load presets
            ScenePreset* defaultPreset = nullptr;
            Array<StringBuf> presetNames;
            if (block.read("PresetNames", presetNames))
            {
                for (const auto& name : presetNames)
                {
                    if (name.empty())
                        continue;

                    ScenePreset* presetToLoad = nullptr;
                    for (auto* preset : m_presets)
                    {
                        if (preset->name == name)
                        {
                            presetToLoad = preset;
                            break;
                        }
                    }

                    if (!presetToLoad)
                    {
                        presetToLoad = new ScenePreset();
                        presetToLoad->name = name;
                        m_presets.pushBack(presetToLoad);
                    }

                    if (presetToLoad->name == "(default)")
                        defaultPreset = presetToLoad;

                    {
                        auto localBlock = block.tag(name);
                        presetToLoad->dirty = false;
                        localBlock.read("ShownNodes", presetToLoad->shownNodes);
                        localBlock.read("HiddenNodes", presetToLoad->hiddenNodes);
                    }
                }
            }

            // make sure we always have the "default" preset
            if (!defaultPreset)
            {
                defaultPreset = new ScenePreset();
                defaultPreset->name = "(default)";
                defaultPreset->dirty = true;
                m_presets.insert(0, defaultPreset);
            }

            // select preset
            StringBuf activePresetName;
            block.read("ActivePreset", activePresetName);
            updatePresetList(activePresetName);

            // apply selected preset
            applySelectedPreset();
        }
    }

    static void CollectExpandedNodes(ui::TreeView* tree, SceneContentTreeModel* model, const ui::ModelIndex& index, Array<StringBuf>& expandedNodesPaths)
    {
        if (tree->isExpanded(index))
        {
            InplaceArray<ui::ModelIndex, 100> children;
            model->children(index, children);

            if (!children.empty())
                if (auto node = model->nodeForIndex(index))
                    if (auto path = node->buildHierarchicalName())
                        expandedNodesPaths.pushBack(path);

            for (const auto& childIndex : children)
                CollectExpandedNodes(tree, model, childIndex, expandedNodesPaths);
        }
    }


    void SceneStructurePanel::configSave(const ui::ConfigBlock& block) const
    {
        TBaseClass::configSave(block);
        presetSave(block.tag("Presets"));

        Array<StringBuf> expandedNodes;
        if (auto rootIndex = m_treeModel->indexForNode(m_scene->root()))
            CollectExpandedNodes(m_tree, m_treeModel, rootIndex, expandedNodes);
        block.write("ExpandedNodes", expandedNodes);
    }

    void SceneStructurePanel::applySelectedPreset()
    {
        // TODO: collect differential state (what to hide what to show)
        // TODO: loading progress bar if lots of stuff to load

        if (m_presetActive)
            applyPresetState(*m_presetActive);
    }

    void SceneStructurePanel::updatePresetList(StringView presetToSelect)
    {
        DEBUG_CHECK_RETURN(m_presetList);

        if (!m_presets.contains(m_presetActive))
            m_presetActive = nullptr;

        std::sort(m_presets.begin(), m_presets.end(), [](const ScenePreset* a, const ScenePreset* b)
            {
                return a->name < b->name;
            });

        m_presetList->clearOptions();
        for (const auto* preset : m_presets)
            m_presetList->addOption(preset->name);

        if (!presetToSelect.empty())
        {
            for (auto* preset : m_presets)
            {
                if (preset->name == presetToSelect)
                {
                    m_presetActive = preset;
                    break;
                }
            }
        }

        if (!m_presetActive)
        {
            for (auto* preset : m_presets)
            {
                if (preset->name == "(default)")
                {
                    m_presetActive = preset;
                    break;
                }
            }
        }

        {
            auto index = m_presets.find(m_presetActive);
            m_presetList->selectOption(index);
        }
    }

    void SceneStructurePanel::configLoad(const ui::ConfigBlock& block)
    {
        TBaseClass::configLoad(block);
        presetLoad(block.tag("Presets"));

        {
            Array<StringBuf> expandedNodes;
            if (block.read("ExpandedNodes", expandedNodes))
            {
                for (const auto& path : expandedNodes)
                    if (const auto* node = m_scene->findNodeByPath(path))
                        if (auto index = m_treeModel->indexForNode(node))
                            m_tree->expandItem(index);
            }
            else
            {
                for (const auto& rootChild : m_scene->root()->children())
                    if (rootChild->type() == SceneContentNodeType::LayerDir || rootChild->type() == SceneContentNodeType::Entity)
                        if (auto index = m_treeModel->indexForNode(rootChild))
                            m_tree->expandItem(index);
            }
        }
    }

    //--

    static void CollectHiddenNodes(const SceneContentNode* node, Array<StringBuf>& outHiddenNodes, Array<StringBuf>& outShownNodes)
    {
        const auto flag = node->visibilityFlagBool();
        if (flag != node->defaultVisibilityFlag())
        {
            if (flag)
                outShownNodes.pushBack(node->buildHierarchicalName());
            else
                outHiddenNodes.pushBack(node->buildHierarchicalName());
        }

        for (const auto& child : node->children())
            CollectHiddenNodes(child, outHiddenNodes, outShownNodes);
    }

    void SceneStructurePanel::collectPresetState(ScenePreset& outState) const
    {
        outState.dirty = true;
        outState.shownNodes.reset();
        outState.hiddenNodes.reset();
        CollectHiddenNodes(m_scene->root(), outState.hiddenNodes, outState.shownNodes);
    }

    static void ApplyVisibilityState(SceneContentNode* node, const HashMap<const SceneContentNode*, SceneContentNodeLocalVisibilityState>& visibilityState)
    {
        auto nodeVisState = SceneContentNodeLocalVisibilityState::Default;
        visibilityState.find(node, nodeVisState);

        node->visibility(nodeVisState);

        for (const auto& child : node->children())
            ApplyVisibilityState(child, visibilityState);        
    }

    void SceneStructurePanel::applyPresetState(const ScenePreset& state)
    {
        HashMap<const SceneContentNode*, SceneContentNodeLocalVisibilityState> visibilityState;
        visibilityState.reserve(state.hiddenNodes.size() + state.shownNodes.size());

        for (const auto& path : state.hiddenNodes)
            if (const auto* node = m_scene->findNodeByPath(path))
                visibilityState[node] = SceneContentNodeLocalVisibilityState::Hidden;

        for (const auto& path : state.shownNodes)
            if (const auto* node = m_scene->findNodeByPath(path))
                visibilityState[node] = SceneContentNodeLocalVisibilityState::Visible;

        ApplyVisibilityState(m_scene->root(), visibilityState);

        m_scene->root()->recalculateVisibility();
    }

    void SceneStructurePanel::presetAddNew()
    {
        StringBuf presetName = "preset";
        if (m_presetActive && m_presetActive->name != "(default)")
            presetName = m_presetActive->name;

        if (ui::ShowInputBox(m_presetList, ui::InputBoxSetup().title("New preset").message("Enter name of new preset:"), presetName))
        {
            if (!presetName.empty())
            {
                ScenePreset* presetToSave = nullptr;
                for (auto* preset : m_presets)
                {
                    if (preset->name == presetName)
                    {
                        presetToSave = preset;
                        break;
                    }
                }

                if (!presetToSave)
                {
                    presetToSave = new ScenePreset();
                    presetToSave->name = presetName;
                }

                collectPresetState(*presetToSave);
                updatePresetList(presetToSave->name);
            }
        }
    }

    void SceneStructurePanel::presetRemove()
    {
        if (!m_presetActive || m_presetActive->name == "(default)")
        {
            ui::ShowMessageBox(m_presetList, ui::MessageBoxSetup().warn().title("Remove preset").message("Current preset can't be removed"));
            return;
        }

        m_presetList->removeOption(m_presetActive->name);
        m_presetActive = nullptr;
    }

    void SceneStructurePanel::presetSave()
    {
        if (m_presetActive)
            collectPresetState(*m_presetActive);
        else
            presetAddNew();
    }

    void SceneStructurePanel::presetSelect()
    {
        auto name = m_presetList->selectedOptionText();
        for (auto* preset : m_presets)
        {
            if (preset->name == name)
            {
                m_presetActive = preset;
                applySelectedPreset();
            }
        }
    }

    //--

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

            m_tree->select(indices, ui::ItemSelectionModeBit::DefaultNoFocus, false);
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
