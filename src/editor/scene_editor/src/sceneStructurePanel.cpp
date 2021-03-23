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
#include "sceneContentNodesEntity.h"
#include "sceneContentNodesTree.h"
#include "scenePreviewContainer.h"
#include "sceneEditMode.h"

#include "engine/world/include/rawPrefab.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiSearchBar.h"
#include "engine/ui/include/uiComboBox.h"
#include "engine/ui/include/uiElementConfig.h"
#include "engine/ui/include/uiMessageBox.h"
#include "engine/ui/include/uiInputBox.h"
#include "engine/ui/include/uiTreeView.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiColumnHeaderBar.h"
#include "core/object/include/object.h"
#include "engine/ui/include/uiDragDrop.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneStructurePanel);
RTTI_END_TYPE();

SceneStructurePanel::SceneStructurePanel(SceneContentStructure* scene, ScenePreviewContainer* preview)
    : m_preview(AddRef(preview))
    , m_scene(AddRef(scene))
{
    layoutVertical();

    //--

    bindShortcut("Ctrl+C") = [this]() { handleTreeObjectCopy(); };
    bindShortcut("Ctrl+X") = [this]() { handleTreeObjectCut(); };
    bindShortcut("Delete") = [this]() { handleTreeObjectDeletion(); };
    bindShortcut("Ctrl+V") = [this]() { handleTreeObjectPaste(false); };
    bindShortcut("Ctrl+Shift+V") = [this]() { handleTreeObjectPaste(true); };

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
    columns->addColumn("Name", 400, false, false, true);
    columns->addColumn("[img:eye]", 30, true, false, false);
    //columns->addColumn("[img:save]", 30, true, false, false);
    columns->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

    m_tree = createChild<SceneContentTreeView>(m_scene, m_preview);
    m_tree->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_tree->customVerticalAligment(ui::ElementVerticalLayout::Expand);
    m_tree->sort(0);
    
    m_tree->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
    {
        treeSelectionChanged();
    };

    //--

    m_tree->addRoot(scene->root()->treeItem());

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

static void CollectExpandedNodes(SceneContentNode* node, Array<StringBuf>& expandedNodesPaths)
{
    if (node->treeItem()->expanded() && !node->treeItem()->children().empty())
    {
        if (auto path = node->buildHierarchicalName())
            expandedNodesPaths.pushBack(path);

        for (const auto& child : node->children())
            CollectExpandedNodes(child, expandedNodesPaths);
    }
}


void SceneStructurePanel::configSave(const ui::ConfigBlock& block) const
{
    TBaseClass::configSave(block);
    presetSave(block.tag("Presets"));

    Array<StringBuf> expandedNodes;
    CollectExpandedNodes(m_scene->root(), expandedNodes);
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
                if (auto* node = m_scene->findNodeByPath(path))
                    node->treeItem()->expand();
        }
        else
        {
            for (const auto& rootChild : m_scene->root()->children())
                if (rootChild->type() == SceneContentNodeType::LayerDir || rootChild->type() == SceneContentNodeType::Entity)
                    rootChild->treeItem()->expand();
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
    auto name = m_presetList->text();
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
    ui::CollectionItems items;
    for (const auto& node : nodes)
        if (node->treeItem())
            items.add(node->treeItem());

    m_tree->TreeViewEx::select(items, ui::ItemSelectionModeBit::DefaultNoFocus, false);
}

void SceneStructurePanel::treeSelectionChanged()
{
    if (auto mode = m_preview ? m_preview->mode() : nullptr)
    {
        auto current = m_tree->current();
        auto selection = m_tree->selection();
        mode->handleTreeSelectionChange(current, selection);
    }
}

void SceneStructurePanel::handleTreeObjectDeletion()
{
    if (auto mode = m_preview ? m_preview->mode() : nullptr)
    {
        auto selection = m_tree->selection();
        mode->handleTreeDeleteNodes(selection);
    }
}

void SceneStructurePanel::handleTreeObjectCopy()
{
    if (auto mode = m_preview ? m_preview->mode() : nullptr)
    {
        auto selection = m_tree->selection();
        mode->handleTreeCopyNodes(selection);
    }
}

void SceneStructurePanel::handleTreeObjectCut()
{
    if (auto mode = m_preview ? m_preview->mode() : nullptr)
    {
        auto selection = m_tree->selection();
        mode->handleTreeCutNodes(selection);
    }
}

void SceneStructurePanel::handleTreeObjectPaste(bool relative)
{
    if (auto mode = m_preview ? m_preview->mode() : nullptr)
    {
        auto current = m_tree->current();
        mode->handleTreePasteNodes(current, relative ? SceneContentNodePasteMode::Relative : SceneContentNodePasteMode::Absolute);
    }
}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
