/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "sceneEditMode.h"
#include "ui/inspector/include/uiDataInspector.h"
#include "ui/gizmo/include/gizmoInterface.h"

namespace ed
{
    namespace world
    {

        class ContentStructure;

        /*//--

        /// helper object TEST
        class HelperObject_Test : public IHelperObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(HelperObject_Test, IHelperObject);

        public:
            HelperObject_Test(const base::RefPtr<ContentNode>& owner);

            virtual bool placement(const base::edit::DocumentObjectID& id, base::AbsoluteTransform& outTransform) override final;
            virtual bool placement(const base::edit::DocumentObjectID& id, const base::AbsoluteTransform& newTransform) override final;
            virtual void handleSelectionChange(const base::edit::DocumentObjectIDSet& oldSelection, const base::edit::DocumentObjectIDSet& newSelection) override final;
            virtual void handleRendering(const base::edit::DocumentObjectIDSet& currentSelection, rendering::scene::FrameInfo& frame) override final;

            static base::RefPtr<HelperObject_Test> CreateFromNode(const base::RefPtr<ContentNode>& node);

        public:
            struct Point
            {
                base::edit::DocumentObjectID id;
                base::Vector3 m_localPos;
                bool m_selected;
            };

            base::Array<Point> points;
            base::HashMap<base::edit::DocumentObjectID, int> m_pointMap;
        };

        //--

        class HelperObjectHandler_Test : public IHelperObjectHandler
        {
            RTTI_DECLARE_VIRTUAL_CLASS(HelperObjectHandler_Test, IHelperObjectHandler);

        public:
            HelperObjectHandler_Test();

            virtual bool supportsObjectCategory(base::StringID category) const override final;
            virtual const base::RefPtr<IHelperObject> createHelperObject(const base::RefPtr<ContentNode>& node) const override final;
        };*/

        //--

        /// scene edit mode for editing objects 
        class SceneEditMode_Objects : public ISceneEditMode, public ui::gizmo::IGizmoTransformContext
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneEditMode_Objects, ISceneEditMode);

        public:
            SceneEditMode_Objects();
            virtual ~SceneEditMode_Objects();

            //--

            virtual base::StringBuf name() const override final;
            virtual base::image::ImagePtr icon() const override final;
            virtual ui::ElementPtr uI() const override final;

            //--

            virtual bool initialize(SceneEditorTab* owner) override final;
            virtual bool activate(const base::Array<ContentNodePtr>& activeSceneSelection) override final;
            virtual void deactive() override final;
            virtual void bind(const base::Array<ContentNodePtr>& activeSceneSelection) override final;
            virtual void configure(base::ClassType dataType, const void* data, bool active) override final;

            //--

            virtual void handleViewportSelection(const SceneRenderingPanel* viewport, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables) override final;
            virtual bool handleContextMenu(const SceneRenderingPanel* viewport, const ui::Position& clickPosition, const base::Array<rendering::scene::Selectable>& selectables, const base::Vector3* worldSpacePosition) override final;
            virtual ui::InputActionPtr handleMouseClick(const SceneRenderingPanel* viewport, const base::input::MouseClickEvent& evt) override final;
            virtual bool handleKeyEvent(const SceneRenderingPanel* viewport, const base::input::KeyEvent& evt) override final;
            virtual bool handleMouseMovement(const SceneRenderingPanel* viewport, const base::input::MouseMovementEvent& evt) override final;
            virtual bool handleMouseWheel(const SceneRenderingPanel* viewport, const base::input::MouseMovementEvent& evt, float delta) override final;
            virtual void handleTick(float dt) override final;
            virtual void handleRendering(const SceneRenderingPanel* viewport, rendering::scene::FrameInfo& frame) override final;
            virtual ui::DragDropHandlerPtr handleDragDrop(const SceneRenderingPanel* viewport, const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override final;

            //---

            virtual bool canCopy() const override final;
            virtual bool canCut() const override final;
            virtual bool canPaste() const override final;
            virtual bool canDelete() const override final;
            virtual void handleCopy() override final;
            virtual void handleCut() override final;
            virtual void handlePaste() override final;
            virtual void handleDelete() override final;

            //---

            // ui::IGizmoTransformContext
            virtual ui::gizmo::ReferenceSpace calcReferenceSpace() const override final;
            virtual ui::gizmo::GizmoTransformTransactionPtr createTransformTransaction() const override final;

            bool pasteNodes(const ContentElementPtr& rootOverride, const base::AbsoluteTransform* rootTransformOverride, const scene::NodeTemplateContainer& container, base::Array<ContentNodePtr>& outCreatedNodes);

        private:
            base::NativeTimePoint m_lastErrorMessageTime;

            ui::ElementPtr m_panel;
            base::RefPtr<ui::DataInspector> m_propertiesBrowser;

            base::Array<ContentNodePtr> m_selectedNodes;

            base::UniquePtr<ui::gizmo::GizmoManager> m_gizmoManager;

            SceneGizmoSettings m_gizmoSettings;
            SceneGridSettings m_gridSettings;
            SceneSelectionSettings m_selectionSettings;

            void selectionChanged(const base::Array<ContentNodePtr>& totalSelection);
            void recrateGizmos();

            void prepareHierarchyOfNodes(const base::Array<ContentNodePtr>& seedList, base::Array<ContentNodePtr>& outOrderedNodeList, bool extendToChildren);
            void prepareHierarchyOfNodesFromSelection(base::Array<ContentNodePtr>& outOrderedNodeList, bool extendToChildren);
            
        };

    } // world
} // ed