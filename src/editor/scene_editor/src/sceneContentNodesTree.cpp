/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#include "build.h"

#include "sceneContentStructure.h"
#include "sceneContentNodes.h"
#include "sceneContentNodesEntity.h"
#include "sceneContentNodesTree.h"
#include "sceneContentDragDrop.h"
#include "scenePreviewContainer.h"
#include "sceneEditMode.h"

#include "editor/assets/include/browserService.h"

#include "engine/rendering/include/debug.h"
#include "engine/rendering/include/params.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiButton.h"
#include "engine/ui/include/uiDragDrop.h"
#include "engine/ui/include/uiMenuBar.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentTreeView);
RTTI_END_TYPE();

SceneContentTreeView::SceneContentTreeView(SceneContentStructure* scene, ScenePreviewContainer* preview)
    : m_scene(scene)
    , m_preview(preview)
{}

SceneContentNodePtr SceneContentTreeView::current() const
{
    if (auto node = TBaseClass::current<SceneContentTreeItem>())
        return node->data();
    return nullptr;
}

Array<SceneContentNodePtr> SceneContentTreeView::selection() const
{
    Array<SceneContentNodePtr> ret;

    ret.reserve(TBaseClass::selection().items().size());

    for (const auto& ptr : TBaseClass::selection().items())
        if (auto node = rtti_cast<SceneContentTreeItem>(ptr))
            if (auto data = node->data())
                ret.pushBack(data);

    return ret;
}

void SceneContentTreeView::select(SceneContentNode* node)
{
    if (node && node->treeItem() && node->treeItem()->view() == this)
        TBaseClass::select(node->treeItem());
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentTreeItem);
RTTI_END_TYPE();

SceneContentTreeItem::SceneContentTreeItem(SceneContentNode* owner)
    : m_owner(owner)
    , m_type(owner->type())
    , m_cachedName(owner->name())
{
    auto container = createChild<ui::IElement>(); // TODO: remove
    container->layoutColumns();
    container->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

    m_labelText = container->createChild<ui::TextLabel>(TempString("[img:node] {}", m_cachedName));

    m_visibleButton = container->createChild<ui::Button>("[img:eye]");
    m_visibleButton->bind(ui::EVENT_CLICKED) = [this]() { cmdToggleVisibility(); };

    /*m_modifiedButton = container->createChild<ui::Button>("[img:save]");
    m_modifiedButton->visibility(false);
    m_modifiedButton->bind(ui::EVENT_CLICKED) = [this]() { cmdSaveChanges(); };*/
}

static StringView IconTextForType(SceneContentNodeType type)
{
    switch (type)
    {
        case SceneContentNodeType::Entity: return "[img:entity]";
        case SceneContentNodeType::LayerFile: return "[img:page]";
        case SceneContentNodeType::LayerDir: return "[img:folder]";
        case SceneContentNodeType::PrefabRoot: return "[img:brick]";
        case SceneContentNodeType::WorldRoot: return "[img:world]";
    }

    return "";
}

static StringView VisibilityIconForNode(const SceneContentNode* node)
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

void SceneContentTreeItem::updateDisplayState()
{
    if (auto node = m_owner.lock())
    {
        m_cachedName = node->name();

        {
            StringBuilder txt;

            if (node->visualFlags().test(SceneContentNodeVisualBit::ActiveNode))
                txt << "[b][i]";

            txt << IconTextForType(m_type);
            txt << " ";
            txt << m_cachedName;

            if (node->modified())
                txt << "*";

            if (node->visualFlags().test(SceneContentNodeVisualBit::ActiveNode))
                txt << "[/i][/b]";

            if (node->visualFlags().test(SceneContentNodeVisualBit::ActiveNode))
                txt << " [tag:#8A8]Active[/tag]";

            node->displayTags(txt);

            m_labelText->text(txt.view());
        }

        //m_modifiedButton->visibility(node->modified());
        m_visibleButton->text(VisibilityIconForNode(node));
    }
}

bool SceneContentTreeItem::handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const
{
    return filter.testString(m_cachedName.view());
}

void SceneContentTreeItem::handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outInfo) const
{
    outInfo.index = uniqueIndex();
    outInfo.type = (int)m_type;
    outInfo.caption = m_cachedName.view();
}

bool SceneContentTreeItem::handleItemContextMenu(ui::ICollectionView* view, const ui::CollectionItems& items, const ui::Position& pos, input::KeyMask controlKeys)
{
    if (auto node = m_owner.lock())
    {
        if (auto* treePtr = tree())
        {
            if (auto mode = treePtr->preview()->mode())
            {
                auto menu = RefNew<ui::MenuButtonContainer>();
                mode->handleTreeContextMenu(menu, node, treePtr->selection());
                return menu->show(view);
            }
        }
    }

    return false;
}

ui::DragDropDataPtr SceneContentTreeItem::queryDragDropData(const input::BaseKeyFlags& keys, const ui::Position& position) const
{
    if (m_type == SceneContentNodeType::Entity)
        if (auto node = m_owner.lock())
            return RefNew<SceneNodeDragDropData>(node);
    return nullptr;
}

ui::DragDropHandlerPtr SceneContentTreeItem::handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    if (auto node = m_owner.lock())
    {
        if (auto fileData = rtti_cast<AssetBrowserFileDragDrop>(data))
        {
            if (node->canAttach(SceneContentNodeType::Entity))
                return RefNew<ui::DragDropHandlerGeneric>(data, this, entryPosition);
        }
        else if (auto nodeData = rtti_cast<SceneNodeDragDropData>(data))
        {
            if (node->canAttach(nodeData->data()->type()))
                return RefNew<ui::DragDropHandlerGeneric>(data, this, entryPosition);
        }
    }

    return nullptr;
}

void SceneContentTreeItem::handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    if (auto node = m_owner.lock())
    {
        if (auto* treePtr = tree())
        {
            if (auto mode = treePtr->preview()->mode())
            {
                if (auto fileData = rtti_cast<AssetBrowserFileDragDrop>(data))
                {
                    if (auto path = fileData->depotPath())
                        mode->handleTreeResourceDrop(node, path);
                }
                else if (auto nodeData = rtti_cast<SceneNodeDragDropData>(data))
                {
                    mode->handleTreeNodeDrop(node, nodeData->data());
                }
            }
        }
    }
}

//---

void SceneContentTreeItem::cmdToggleVisibility()
{
    if (auto node = m_owner.lock())
    {
        if (node->visibilityFlagRaw() == SceneContentNodeLocalVisibilityState::Hidden)
            node->visibility(SceneContentNodeLocalVisibilityState::Visible);
        else
            node->visibility(SceneContentNodeLocalVisibilityState::Hidden);
    }
}

void SceneContentTreeItem::cmdSaveChanges()
{
    
}

//---

END_BOOMER_NAMESPACE_EX(ed)
