/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

#include "base/ui/include/uiSimpleTreeModel.h"
#include "base/ui/include/uiElement.h"

namespace ed
{
    //--

    // view model for a scene structure tree
    class ASSETS_SCENE_EDITOR_API SceneContentTreeModel : public ui::IAbstractItemModel
    {
    public:
        SceneContentTreeModel(SceneContentStructure* structure, ScenePreviewContainer* preview);
        virtual ~SceneContentTreeModel();

        // IAbstractItemModel
        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const override final;
        virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual void children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const override final;
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;
        virtual base::StringBuf displayContent(const ui::ModelIndex& id, int colIndex = 0) const override final;

        virtual ui::PopupPtr contextMenu(ui::AbstractItemView* view, const base::Array<ui::ModelIndex>& indices) const override final;
        virtual ui::ElementPtr tooltip(ui::AbstractItemView* view, ui::ModelIndex id) const override final;

        virtual ui::DragDropDataPtr queryDragDropData(const base::input::BaseKeyFlags& keys, const ui::ModelIndex& item) override final;
        virtual ui::DragDropHandlerPtr handleDragDropData(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data, const ui::Position& pos) override final;
        virtual bool handleDragDropCompletion(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data) override final;
        virtual bool handleIconClick(const ui::ModelIndex& item, int columnIndex) const override final;

        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;

        SceneContentNodePtr nodeForIndex(const ui::ModelIndex& index) const;
        ui::ModelIndex indexForNode(const SceneContentNode* node) const;

    private:
        SceneContentStructure* m_structure = nullptr;
        ScenePreviewContainer* m_preview = nullptr; // TOOD: remove? 

        SceneContentNodePtr m_root;

        GlobalEventTable m_contentEvents;

        void handleChildNodeAttached(SceneContentNode* child);
        void handleChildNodeDetached(SceneContentNode* child);
        void handleNodeVisualFlagsChanged(SceneContentNode* child);
        void handleNodeModifiedFlagsChanged(SceneContentNode* child);
        void handleNodeVisibilityChanged(SceneContentNode* child);
        void handleNodeNameChanged(SceneContentNode* child);
    };

    //--

    // scene structure panel (tree)
    class ASSETS_SCENE_EDITOR_API SceneStructurePanel : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneStructurePanel, ui::IElement);

    public:
        SceneStructurePanel(SceneContentStructure* scene, ScenePreviewContainer* preview);
        virtual ~SceneStructurePanel();

        void syncExternalSelection(const Array<SceneContentNodePtr>& nodes);
        
        virtual void configSave(const ui::ConfigBlock& block) const;
        virtual void configLoad(const ui::ConfigBlock& block);

    private:
        RefPtr<SceneContentStructure> m_scene;
        RefPtr<ScenePreviewContainer> m_preview;

        //--

        void presetAddNew();
        void presetRemove();
        void presetSave();
        void presetSelect();

        struct ScenePreset
        {
            RTTI_DECLARE_POOL(POOL_UI_OBJECTS);

        public:
            StringBuf name;
            bool dirty = false;
            Array<StringBuf> hiddenNodes;
            Array<StringBuf> shownNodes;
        };

        ui::ComboBoxPtr m_presetList;

        Array<ScenePreset*> m_presets;
        ScenePreset* m_presetActive = nullptr;

        void collectPresetState(ScenePreset& outState) const;
        void applyPresetState(const ScenePreset& state);

        void updatePresetList(StringView presetToSelect);
        void applySelectedPreset();

        void presetSave(const ui::ConfigBlock& block) const;
        void presetLoad(const ui::ConfigBlock& block);

        //--

        ui::SearchBarPtr m_searchBar;

        ui::TreeViewPtr m_tree;
        base::RefPtr<SceneContentTreeModel> m_treeModel;

        void treeSelectionChanged();

        void handleTreeObjectDeletion();
        void handleTreeObjectCopy();
        void handleTreeObjectCut();
        void handleTreeObjectPaste(bool relative);

        //--
    };

    //--

} // ed