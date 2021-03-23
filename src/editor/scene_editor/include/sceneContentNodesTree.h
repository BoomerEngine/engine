/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#pragma once

#include "engine/world/include/rawEntity.h"
#include "engine/ui/include/uiTreeViewEx.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

// tree view with scene nodes
class EDITOR_SCENE_EDITOR_API SceneContentTreeView : public ui::TreeViewEx
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentTreeView, ui::TreeViewEx);

public:
    SceneContentTreeView(SceneContentStructure* scene, ScenePreviewContainer* preview);

    //--

    INLINE SceneContentStructure* scene() const { return m_scene; }
    INLINE ScenePreviewContainer* preview() const { return m_preview; }

    //--

    SceneContentNodePtr current() const;
    Array<SceneContentNodePtr> selection() const;

    void select(SceneContentNode* node);

    //--

private:
    SceneContentStructure* m_scene = nullptr;
    ScenePreviewContainer* m_preview = nullptr;
};

//--

// display element for scene tree
class EDITOR_SCENE_EDITOR_API SceneContentTreeItem : public ui::ITreeItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentTreeItem, ui::ITreeItem);

public:
    SceneContentTreeItem(SceneContentNode* owner);

    INLINE SceneContentNodePtr data() const { return m_owner.lock(); }

    INLINE SceneContentTreeView* tree() const { return rtti_cast<SceneContentTreeView>(view()); }

    void updateDisplayState();

    virtual bool handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const override;
    virtual void handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outInfo) const override;
    virtual bool handleItemContextMenu(ui::ICollectionView* view, const ui::CollectionItems& items, const ui::Position& pos, input::KeyMask controlKeys) override;

    virtual ui::DragDropDataPtr queryDragDropData(const input::BaseKeyFlags& keys, const ui::Position& position) const;
    virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;
    virtual void handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;

private:
    SceneContentNodeWeakPtr m_owner;
    SceneContentNodeType m_type;
    StringBuf m_cachedName;

    //--

    ui::TextLabelPtr m_labelText;
    ui::ButtonPtr m_visibleButton;
    ui::ButtonPtr m_modifiedButton;

    //--

    void cmdToggleVisibility();
    void cmdSaveChanges();
};

//--

END_BOOMER_NAMESPACE_EX(ed)
