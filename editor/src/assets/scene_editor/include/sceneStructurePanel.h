/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once
#include "base/ui/include/uiSimpleTreeModel.h"

namespace ed
{
    //--

    // model for a mesh structure tree
    class ASSETS_SCENE_EDITOR_API SceneContentTreeModel : public ui::IAbstractItemModel
    {
    public:
        SceneContentTreeModel(SceneContentStructure* structure, ScenePreviewContainer* preview);
        virtual ~SceneContentTreeModel();

        // IAbstractItemModel
        virtual uint32_t rowCount(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual bool hasIndex(int row, int col, const ui::ModelIndex& parent = ui::ModelIndex()) const override final;
        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const override final;
        virtual ui::ModelIndex index(int row, int column, const ui::ModelIndex& parent = ui::ModelIndex()) const override final;

        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;
        virtual base::StringBuf displayContent(const ui::ModelIndex& id, int colIndex = 0) const override final;

        virtual ui::PopupPtr contextMenu(ui::AbstractItemView* view, const base::Array<ui::ModelIndex>& indices) const override final;
        virtual ui::ElementPtr tooltip(ui::AbstractItemView* view, ui::ModelIndex id) const override final;

        virtual ui::DragDropDataPtr queryDragDropData(const base::input::BaseKeyFlags& keys, const ui::ModelIndex& item) override final;
        virtual ui::DragDropHandlerPtr handleDragDropData(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data, const ui::Position& pos) override final;
        virtual bool handleDragDropCompletion(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data) override final;

        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;

        SceneContentNodePtr nodeForIndex(const ui::ModelIndex& index) const;
        ui::ModelIndex indexForNode(const SceneContentNode* node) const;

    private:
        class EditorNodeProxy : public IReferencable
        {
        public:
            SceneContentNodeWeakPtr m_node; // actual node data
            EditorNodeProxy* m_parent = nullptr;
            Array<EditorNodeProxy*> m_children; // only the created ones

            EditorNodeProxy(SceneContentNode* node, EditorNodeProxy* parent);
            ~EditorNodeProxy();

            EditorNodeProxy* addChild(SceneContentTreeModel* model, SceneContentNode* node);
            EditorNodeProxy* removeChild(SceneContentTreeModel* model, SceneContentNode* node);

            int indexInParent() const;
            ui::ModelIndex index(const SceneContentTreeModel* model) const;
        };

        SceneContentStructure* m_structure = nullptr;
        ScenePreviewContainer* m_preview = nullptr; // TOOD: remove? 
            
        HashMap<SceneContentNodeWeakPtr, EditorNodeProxy*> m_proxiesMap;
        EditorNodeProxy* m_rootProxy = nullptr;

        GlobalEventTable m_contentEvents;

        void handleChildNodeAttached(SceneContentNode* child);
        void handleChildNodeDetached(SceneContentNode* child);
        void handleNodeVisualFlagsChanged(SceneContentNode* child);
        void handleNodeVisibilityChanged(SceneContentNode* child);
        void handleNodeNameChanged(SceneContentNode* child);
    };

    //--

    // a preview panel for an image
    class ASSETS_SCENE_EDITOR_API SceneStructurePanel : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneStructurePanel, ui::IElement);

    public:
        SceneStructurePanel();
        virtual ~SceneStructurePanel();

        void bindScene(SceneContentStructure* scene, ScenePreviewContainer* preview);

        void syncExternalSelection(const Array<SceneContentNodePtr>& nodes);

    private:
        RefPtr<SceneContentStructure> m_scene;
        RefPtr<ScenePreviewContainer> m_preview;

        ui::TreeViewPtr m_tree;
        base::RefPtr<SceneContentTreeModel> m_treeModel;

        void treeSelectionChanged();

        void handleTreeObjectDeletion();
        void handleTreeObjectCopy();
        void handleTreeObjectCut();
        void handleTreeObjectPaste(bool relative);
    };

    //--

} // ed