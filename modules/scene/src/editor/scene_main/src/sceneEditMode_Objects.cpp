/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#include "build.h"
#include "sceneRenderingPanel.h"
#include "sceneRenderingContainer.h"
#include "sceneNodePalette.h"
#include "sceneEditor.h"
#include "sceneEditorTempObjects.h"
#include "sceneEditorStructure.h"
#include "sceneEditorStructureNode.h"
#include "sceneEditorStructureLayer.h"
#include "sceneEditorStructurePrefabRoot.h"
#include "sceneEditorStructureWorldRoot.h"

#include "sceneEditMode_Objects.h"

#include "scene/common/include/scenePrefab.h"
#include "scene/common/include/sceneRuntime.h"

#include "base/object/include/objectObserver.h"
#include "base/image/include/image.h"

#include "rendering/scene/include/renderingFrameScreenCanvas.h"

#include "ui/gizmo/include/gizmo.h"
#include "ui/gizmo/include/gizmoManager.h"
#include "ui/widgets/include/uiWindowMessage.h"
#include "ui/inspector/include/uiDataInspector.h"
#include "ui/models/include/uiTreeView.h"

#include "editor/asset_browser/include/assetBrowserDragDrop.h"

namespace ed
{
    namespace world
    {

        RTTI_BEGIN_TYPE_CLASS(SceneEditMode_Objects);
        RTTI_END_TYPE();

        //--

        SceneEditMode_Objects::SceneEditMode_Objects()
        {}

        SceneEditMode_Objects::~SceneEditMode_Objects()
        {
        }

        base::StringBuf SceneEditMode_Objects::name() const
        {
            return base::StringBuf("Objects");
        }

        static base::res::StaticResource<base::image::Image> resTable("engine/ui/styles/icons/table.png");

        base::image::ImagePtr SceneEditMode_Objects::icon() const
        {
            return resTable.loadAndGet();
        }

        ui::ElementPtr SceneEditMode_Objects::uI() const
        {
            return m_panel;
        }

        //--

        bool SceneEditMode_Objects::initialize(SceneEditorTab* owner)
        {
            if (!TBaseClass::initialize(owner))
                return false;

            m_panel = base::CreateSharedPtr<ui::IElement>();
            m_panel->layoutMode(ui::LayoutMode::Vertical);
            m_panel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_panel->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            m_panel->customProportion(1.0f);

            m_propertiesBrowser = base::CreateSharedPtr<ui::DataInspector>();
            m_propertiesBrowser->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_propertiesBrowser->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            m_propertiesBrowser->customProportion(1.0f);
            m_panel->attachBuildChild(m_propertiesBrowser);

            m_gizmoManager.create();

            return true;
        }

        bool SceneEditMode_Objects::activate(const base::Array<ContentNodePtr>& activeSceneSelection)
        {
            selectionChanged(activeSceneSelection);
            return true;
        }

        void SceneEditMode_Objects::deactive()
        {
            m_selectedNodes.clear();
            editor()->content().visualizeSelection(m_selectedNodes);

            m_propertiesBrowser->bind(nullptr);
        }

        void SceneEditMode_Objects::bind(const base::Array<ContentNodePtr>& activeSceneSelection)
        {
            selectionChanged(activeSceneSelection);
        }

        void SceneEditMode_Objects::configure(base::ClassType dataType, const void* data, bool active)
        {
            if (dataType == SceneGizmoSettings::GetStaticClass())
            {
                m_gizmoSettings = *(const SceneGizmoSettings*)data;
                recrateGizmos();
            }                
            else if (dataType == SceneGridSettings::GetStaticClass())
            {
                m_gridSettings = *(const SceneGridSettings*)data;
            }
            else if (dataType == SceneSelectionSettings::GetStaticClass())
            {
                m_selectionSettings = *(const SceneSelectionSettings*)data;
            }
        }

        //--

        ui::gizmo::ReferenceSpace SceneEditMode_Objects::calcReferenceSpace() const
        {
            // no active selection
            if (m_selectedNodes.empty())
                return ui::gizmo::ReferenceSpace();

            // get the root object for the reference space - we can make a config for this but for now use the first one
            auto rootObject = m_selectedNodes.front();
            auto& rootTransform = rootObject->absoluteTransform();

            // world transform
            if (m_gizmoSettings.m_space == ui::gizmo::GizmoSpace::Local)
            {
                return ui::gizmo::ReferenceSpace(m_gizmoSettings.m_space, rootTransform);
            }
            else if (m_gizmoSettings.m_space == ui::gizmo::GizmoSpace::Parent)
            {
                // our parent should be another node
                if (auto parentObject = base::rtti_cast<ContentNode>(rootObject->parent()))
                {
                    auto parentTransform = parentObject->absoluteTransform();
                    parentTransform.position(rootTransform.position());
                    return ui::gizmo::ReferenceSpace(m_gizmoSettings.m_space, parentTransform);
                }
            }

            // fallback to world axes
            auto rootPosition = rootTransform.position();
            return ui::gizmo::ReferenceSpace(m_gizmoSettings.m_space, rootPosition);
        }

        //--

        class ObjectsGizmoTransaction : public ui::gizmo::IGizmoTransformTransaction
        {
        public:
            ObjectsGizmoTransaction(const ui::gizmo::ReferenceSpace& referenceSpace, const ui::gizmo::GizmoSettings& gizmoSettings, const ui::gizmo::GridSettings& gridSettings, SceneEditorTab* editor, const base::Array<ContentNodePtr>& nodes)
                : ui::gizmo::IGizmoTransformTransaction(referenceSpace, gizmoSettings, gridSettings)
                , m_editor(editor)
            {
                m_nodes.reserve(nodes.size());

                for (auto& node : nodes)
                {
                    auto& info = m_nodes.emplaceBack();
                    info.m_node = node;
                    info.m_currentPos = node->absoluteTransform();
                    info.m_originalPos = node->absoluteTransform();
                }
            }

            virtual void revert() override final
            {
                for (auto& info : m_nodes)
                    info.m_node->absoluteTransform(info.m_originalPos);
            }

            virtual void preview(const base::Transform& transform) override final
            {
                for (auto& info : m_nodes)
                {
                    auto newTransform = ui::gizmo::TransformByOperation(info.m_originalPos, capturedReferenceSpace(), transform);
                    info.m_node->absoluteTransform(newTransform);
                }
            }

            virtual void apply(const base::Transform& transform) override final
            {
                for (auto& info : m_nodes)
                {
                    auto newTransform = ui::gizmo::TransformByOperation(info.m_originalPos, capturedReferenceSpace(), transform);
                    info.m_node->absoluteTransform(newTransform);
                }

                //m_editor->createSnapshotPoint("Transform objects");
            }

        protected:
            SceneEditorTab* m_editor;

            struct NodeInfo
            {
                ContentNodePtr m_node;
                base::AbsoluteTransform m_originalPos;
                base::AbsoluteTransform m_currentPos;
            };

            base::Array<NodeInfo> m_nodes;
        };

        ui::gizmo::GizmoTransformTransactionPtr SceneEditMode_Objects::createTransformTransaction() const
        {
            // TODO: pivot mode ?
            return base::CreateSharedPtr<ObjectsGizmoTransaction>(calcReferenceSpace(), m_gizmoSettings, m_gridSettings, editor(), m_selectedNodes);
        }

        //--

        void SceneEditMode_Objects::handleViewportSelection(const SceneRenderingPanel* viewport, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
        {
            base::Array<ContentNodePtr> nodes;
            nodes.reserve(selectables.size());

            /*for (auto& id : selectables)
                if (auto obj = base::rtti_cast<ContentNode>(id.objectID().resolve()))
                    nodes.pushBack(obj);*/

            if (shift)
            {
                for (auto& cur : nodes)
                    editor()->changeSelection(cur, true);
            }
            else if (ctrl)
            {
                for (auto& cur : nodes)
                    editor()->changeSelection(cur, !editor()->isSelected(cur));
            }
            else
            {
                editor()->changeSelection(nodes);
            }
        }

        bool SceneEditMode_Objects::handleContextMenu(const SceneRenderingPanel* viewport, const ui::Position& clickPosition, const base::Array<rendering::scene::Selectable>& selectables, const base::Vector3* worldSpacePosition)
        {
            return false;
        }

        ui::InputActionPtr SceneEditMode_Objects::handleMouseClick(const SceneRenderingPanel* viewport, const base::input::MouseClickEvent& evt)
        {
            if (evt.leftClicked())
            {
                auto clientPos = evt.absolutePosition() - viewport->cachedDrawArea().absolutePosition();
                if (auto gizmoInputAction = m_gizmoManager->handleMouseClick(clientPos, viewport))
                    return gizmoInputAction;
            }
            
            return nullptr;
        }

        bool SceneEditMode_Objects::handleKeyEvent(const SceneRenderingPanel* viewport, const base::input::KeyEvent& evt)
        {
            return false;
        }

        bool SceneEditMode_Objects::handleMouseMovement(const SceneRenderingPanel* viewport, const base::input::MouseMovementEvent& evt)
        {
            auto clientPos = evt.absolutePosition() - viewport->cachedDrawArea().absolutePosition();
            m_gizmoManager->handleMouseMovement(clientPos, viewport);

            return true; // gizmo always "handles" the mouse movement
        }

        bool SceneEditMode_Objects::handleMouseWheel(const SceneRenderingPanel* viewport, const base::input::MouseMovementEvent& evt, float delta)
        {
            return false;
        }

        void SceneEditMode_Objects::handleTick(float dt)
        {
            
        }

        void SceneEditMode_Objects::handleRendering(const SceneRenderingPanel* viewport, rendering::scene::FrameInfo& frame)
        {
            if (auto active = editor()->content().activeElement())
            {
                rendering::scene::ScreenCanvas dd(frame);

                if (auto node = base::rtti_cast<ContentNode>(active))
                    dd.text(20, 50, base::TempString("Active node: '{}'", node->path()));
                else if (auto layer = base::rtti_cast<ContentLayer>(active))
                    dd.text(20, 50, base::TempString("Active layer: '{}'", layer->filePath()));
            }

            m_gizmoManager->render(frame, viewport);
        }

        //--

        class SceneRenderingPanelDragDropPreviewHandler : public ui::IDragDropHandler
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneRenderingPanelDragDropPreviewHandler, IDragDropHandler);

        public:
            SceneRenderingPanelDragDropPreviewHandler(SceneEditorTab* editor, const SceneRenderingPanel* panel, const NewNodeSetup& initData, const ui::DragDropDataPtr& data, const ui::Position& initialPosition, const base::RefPtr<ui::RenderingPanelDepthBufferQuery>& depth, const ContentElementPtr& activeParent)
                : IDragDropHandler(data, panel->sharedFromThis(), initialPosition)
                , m_panel(panel)
                , m_editor(editor)
                , m_initData(initData)
                , m_parent(activeParent)
                , m_depth(depth)
            {
            }

            virtual ~SceneRenderingPanelDragDropPreviewHandler()
            {
                DEBUG_CHECK(!m_previewObject);
            }

            void handleCancel() override
            {
                removePreviewRepresentation();
            }

            virtual bool canHideDefaultDataPreview() const override final
            {
                return true;
            }

            virtual bool canHandleDataAt(const ui::Position& absolutePos) const override final
            {
                updatePreviewObject(absolutePos);
                return true;
            }

            virtual void handleMouseWheel(const base::input::MouseMovementEvent& evt, float delta) override final
            {
                auto rotationStep = m_editor->gridSettings().m_rotationGridEnabled ? m_editor->gridSettings().m_rotationGridSize : 1.0f;
                auto step = (delta > 0.0f) ? 1.0f : -1.0f;
                st_insertionRotation = base::Snap(st_insertionRotation + rotationStep * step, rotationStep);
                updatePreviewObject(ui::Position(evt.absolutePosition().x, evt.absolutePosition().y));
            }

            virtual void handleData(const ui::Position& absolutePos) const override final
            {
                // create only if preview object was valid
                if (m_previewObject)
                {
                    // remove the temp preview block
                    removePreviewRepresentation();

                    // get target
                    if (auto activeTarget = m_editor->content().activeElement())
                    {
                        // create node template
                        if (auto nodeTemplate = m_initData.createNode())
                        {
                            auto nodeWrapper = base::CreateSharedPtr<ContentNode>(nodeTemplate);

                            if (!activeTarget->addContent(nodeWrapper))
                            {
                                ui::PostGeneralMessageWindow(m_panel->sharedFromThis(), ui::MessageType::Error, "Failed to add node to scene");
                            }
                            else
                            {
                                base::Array<ContentNodePtr> selection;
                                selection.pushBack(nodeWrapper);
                                m_editor->changeSelection(selection); 
                            }
                        }
                        else
                        {
                            ui::PostGeneralMessageWindow(m_panel->sharedFromThis(), ui::MessageType::Error, "Failed to create preview object");
                        }
                    }
                }
            }

            //--

            void updatePreviewObject(const ui::Position& mousePosition) const
            {
                // calculate the placement for current mouse position
                base::AbsoluteTransform transform;
                if (!calcTransformForMousePosition(mousePosition, transform))
                {
                    removePreviewRepresentation();
                    return;
                }

                // update placement
                m_initData.m_placement.T = transform.position().approximate();
                m_initData.m_placement.R = transform.rotation().toRotator();

                // create the preview object if missing
                if (!m_previewObject)
                {
                    // create preview object
                    if (auto nodeTemplate = m_initData.createNode())
                    {
                        m_previewObject = base::CreateSharedPtr<SceneTempObjectNode>(nodeTemplate);
                        m_previewObject->attachToScene(m_editor->tempObjects());
                    }
                }
                else
                {
                    // move the preview object
                    m_previewObject->placement(m_initData.m_placement);
                }

                // save transform
                m_previewObjectTransform = transform;
            }

            bool calcWorldPositionForMousePosition(const ui::Position& mousePosition, base::AbsolutePosition& outPosition) const
            {
                // get the relative coordinates
                auto relativePosition = mousePosition - m_panel->cachedDrawArea().absolutePosition();

                // calculate the initial ray
                base::AbsolutePosition rayStart;
                base::Vector3 rayDir;
                if (!m_panel->worldSpaceRayForClientPixelExact((int)relativePosition.x, (int)relativePosition.y, rayStart, rayDir))
                    return false;

                // sample depth and get world position this way
                if (m_depth->calcWorldPosition(relativePosition.x, relativePosition.y, rayStart))
                {
                    outPosition = rayStart;
                    return true;
                }

                // TODO: intersect helper geometry

                // align to grid

                // intersect ground plane
                if (rayDir.z != 0.0f)
                {
                    auto dist = rayStart.approximate().z / -rayDir.z;
                    if (dist > 0.0f)
                    {
                        auto pointOnPlane = rayStart.approximate() + (rayDir * dist);
                        pointOnPlane.z = 0.0f;

                        outPosition = pointOnPlane;
                        return true;
                    }
                }

                // we were not able to find a proper position for given coordinates
                return false;
            }

            bool calcTransformForMousePosition(const ui::Position& mousePosition, base::AbsoluteTransform& outTransform) const
            {
                // get the position we are pointing at
                base::AbsolutePosition position;
                if (!calcWorldPositionForMousePosition(mousePosition, position))
                    return false;

                // snap position
                if (m_editor->gridSettings().m_positionGridEnabled)
                {
                    auto gridSize = (double)m_editor->gridSettings().m_positionGridSize;
                    position = base::Snap(position, gridSize);
                }

                // build transform
                outTransform.position(position).rotation(0.0f, st_insertionRotation, 0.0f);
                return true;
            }

        private:
            SceneEditorTab* m_editor;
            const SceneRenderingPanel* m_panel;
            mutable NewNodeSetup m_initData;
            ContentElementPtr m_parent;

            base::RefPtr<ui::RenderingPanelDepthBufferQuery> m_depth;

            mutable base::RefPtr<SceneTempObjectNode> m_previewObject;
            mutable base::AbsoluteTransform m_previewObjectTransform;

            static float st_insertionRotation;

            void removePreviewRepresentation() const
            {
                if (m_previewObject)
                {
                    m_previewObject->deattachFromScene();
                    m_previewObject.reset();
                }
            }
        };

        float SceneRenderingPanelDragDropPreviewHandler::st_insertionRotation = 0.0f;

        RTTI_BEGIN_TYPE_CLASS(SceneRenderingPanelDragDropPreviewHandler);
        RTTI_END_TYPE();

        ui::DragDropHandlerPtr SceneEditMode_Objects::handleDragDrop(const SceneRenderingPanel* viewport, const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
        {
            // we need active layer
            auto active = editor()->content().activeElement();
            if (!active)
            {
                if (m_lastErrorMessageTime.timeTillNow().toSeconds() >= 1.0f)
                {
                    ui::PostGeneralMessageWindow(viewport->sharedFromThis(), ui::MessageType::Error, base::TempString("No context object selected"));
                    m_lastErrorMessageTime.resetToNow();
                }

                return nullptr;
            }
            else
            {
                // we can handle the node palette data
                if (auto nodePaletteTemplate = base::rtti_cast<NodePaletteTemplateDragData>(data))
                {
                    // capture current scene depth
                    auto sceneDepth = const_cast<SceneRenderingPanel*>(viewport)->queryDepth();
                    if (sceneDepth)
                    {
                        // create a drag and drop handler that will move the object around
                        return base::CreateSharedPtr<SceneRenderingPanelDragDropPreviewHandler>(editor(), viewport, nodePaletteTemplate->setup(), data, entryPosition, sceneDepth, active);
                    }
                }

                else if (auto fileData = base::rtti_cast<AssetBrowserFileDragDrop>(data))
                {
                    /*// get the target resources for the file
                    base::InplaceArray<PlaceableNodeTemplateInfo, 8> nodeTemplates;
                    PlaceableNodeTemplateInfo::FindCompatibleNodesForResource(fileData->file(), true, nodeTemplates);

                    // select the FIRST template
                    // TODO: add a context menu

                    if (nodeTemplates.size() >= 1)
                    {
                        // capture current scene depth
                        auto sceneDepth = const_cast<SceneRenderingPanel*>(viewport)->queryDepth();
                        if (sceneDepth)
                        {
                            scene::NodeTemplateInitData initData;
                            initData.className = nodeTemplates[0].m_nodeClass;
                            initData.m_resourceClass = nodeTemplates[0].m_predefinedResource.resourceClass();
                            initData.m_resourcePath = nodeTemplates[0].m_predefinedResource.path();

                            // create a drag and drop handler that will move the object around
                            return base::CreateSharedPtr<SceneRenderingPanelDragDropPreviewHandler>(editor(), viewport, initData, data, entryPosition, sceneDepth, active);
                        }
                    }*/
                }
            }

            // no drag action
            return nullptr;
        }

        //---

        bool SceneEditMode_Objects::canCopy() const
        {
            return !m_selectedNodes.empty();
        }

        bool SceneEditMode_Objects::canCut() const
        {
            return !m_selectedNodes.empty();
        }

        bool SceneEditMode_Objects::canPaste() const
        {
            return editor()->content().activeElement().get() != nullptr;
        }

        bool SceneEditMode_Objects::canDelete() const
        {
            return !m_selectedNodes.empty();
        }

        void SceneEditMode_Objects::prepareHierarchyOfNodesFromSelection(base::Array<ContentNodePtr>& outOrderedNodeList, bool extendToChildren)
        {
            prepareHierarchyOfNodes(m_selectedNodes, outOrderedNodeList, extendToChildren);
        }

        void SceneEditMode_Objects::prepareHierarchyOfNodes(const base::Array<ContentNodePtr>& seedList, base::Array<ContentNodePtr>& outOrderedNodeList, bool extendToChildren)
        {
            // collect all nodes that we will have to remove
            base::HashSet<ContentNode*> allNodes;
            for (auto& ptr : seedList)
            {
                if (extendToChildren)
                    ptr->collectHierarchy(allNodes);
                else
                    allNodes.insert(ptr.get());
            }

            // make sure parents are reported before children
            base::HashSet<ContentNode*> reportedSet;
            base::Array<ContentNode*> reportedNodes;
            for (auto& ptr : allNodes)
            {
                base::InplaceArray<ContentNode*, 15> parents;

                auto cur = ptr;
                while (cur)
                {
                    if (allNodes.contains(cur))
                    {
                        if (reportedSet.insert(cur))
                            parents.pushBack(cur);
                    }

                    cur = base::rtti_cast<ContentNode>(cur->parent());
                }

                for (int i=parents.lastValidIndex(); i >= 0; --i)
                    reportedNodes.pushBack(parents[i]);
            }

            // reverse the list
            outOrderedNodeList.reserve(reportedNodes.size());
            /*for (int i = reportedNodes.lastValidIndex(); i >= 0; --i)
                outOrderedNodeList.pushBack(reportedNodes[i]->sharedFromThisType<ContentNode>());*/
            for (auto ptr  : reportedNodes)
                outOrderedNodeList.pushBack(ptr->sharedFromThisType<ContentNode>());
        }

        void SceneEditMode_Objects::handleCopy()
        {
            bool copyHierarchy = true; // TODO: allow to copy single node, without selection

            base::Array<ContentNodePtr> nodesToCopy;
            prepareHierarchyOfNodesFromSelection(nodesToCopy, copyHierarchy);
            TRACE_INFO("Discovered {} nodes to copy", nodesToCopy.size());

            // nothing to copy
            if (nodesToCopy.empty())
            {
                ui::PostGeneralMessageWindow(m_propertiesBrowser->sharedFromThis(), ui::MessageType::Error, base::TempString("Nothing to copy"));
                return;
            }

            // create a container with the nodes, the container will be stored in the clipboard
            auto container = base::CreateSharedPtr<scene::NodeTemplateContainer>();
            base::HashMap<ContentNode*, int> assignedNodeIndices;
            for (auto& node : nodesToCopy)
            {
                // parent index ?
                int parentIndex = INDEX_NONE;
                if (auto parentNode = base::rtti_cast<ContentNode>(node->parent()))
                    assignedNodeIndices.findSafe(parentNode, parentIndex);

                // add a copy to container
                auto nodeIndex = container->addNode(node->nodeTemplate(), true, parentIndex);
                assignedNodeIndices[node.get()] = nodeIndex;
            }

            // save data to container
            if (!base::GetService<Editor>()->saveToClipboard(container))
            {
                ui::PostGeneralMessageWindow(m_propertiesBrowser->sharedFromThis(), ui::MessageType::Error, base::TempString("Unable to save nodes to clipboard"));
                return;
            }
        }

        void SceneEditMode_Objects::handleCut()
        {
            bool copyHierarchy = true; // TODO: allow to cut single node, without selection

            base::Array<ContentNodePtr> nodesToCut;
            prepareHierarchyOfNodesFromSelection(nodesToCut, copyHierarchy);
            TRACE_INFO("Discovered {} nodes to cut", nodesToCut.size());

            // nothing to copy
            if (nodesToCut.empty())
            {
                ui::PostGeneralMessageWindow(m_propertiesBrowser->sharedFromThis(), ui::MessageType::Error, base::TempString("Nothing to copy"));
                return;
            }

            // create a container with the nodes, the container will be stored in the clipboard
            auto container = base::CreateSharedPtr<scene::NodeTemplateContainer>();
            base::HashMap<ContentNode*, int> assignedNodeIndices;
            for (auto& node : nodesToCut)
            {
                // parent index ?
                int parentIndex = INDEX_NONE;
                if (auto parentNode = base::rtti_cast<ContentNode>(node->parent()))
                    assignedNodeIndices.findSafe(parentNode, parentIndex);

                // add a copy to container
                auto nodeIndex = container->addNode(node->nodeTemplate(), true, parentIndex);
                assignedNodeIndices[node.get()] = nodeIndex;
            }

            // save data to container
            if (!base::GetService<Editor>()->saveToClipboard(container))
            {
                ui::PostGeneralMessageWindow(m_propertiesBrowser->sharedFromThis(), ui::MessageType::Error, base::TempString("Unable to save nodes to clipboard"));
                return;
            }

            // modify current selection
            auto newSelection = editor()->selectionSet();

            // remove in reverse order - children first
            for (int i = nodesToCut.lastValidIndex(); i >= 0; --i)
            {
                auto& ptr = nodesToCut[i];

                newSelection.remove(ptr);

                if (!ptr->isInstancedFromPrefab())
                {
                    auto parent  = ptr->parent();
                    if (parent)
                        parent->removeContent(ptr);
                }
            }

            // deselect all removed nodes
            editor()->changeSelection(newSelection.keys());
        }

        bool SceneEditMode_Objects::pasteNodes(const ContentElementPtr& rootOverride, const base::AbsoluteTransform* rootTransformOverride, const scene::NodeTemplateContainer& container, base::Array<ContentNodePtr>& outCreatedNodes)
        {
            // nothing to paste at
            if (!rootOverride)
                return false;

            // paste nodes
            base::AbsoluteTransform pivotTransform;
            base::HashMap<int, ContentNodePtr> resolvedParents;
            for (uint32_t nodeId=0; nodeId<container.nodes().size(); ++nodeId)
            {
                auto& nodeInfo = container.nodes()[nodeId];

                // find parent
                ContentElementPtr parentNode;
                if (nodeInfo.m_parentId != INDEX_NONE)
                    parentNode = resolvedParents.findSafe(nodeInfo.m_parentId);
                if (!parentNode)
                    parentNode = rootOverride;

                // override transform if a specific reference point was specified (paste at)
                if (rootTransformOverride)
                {
                    if (nodeId == 0)
                    {
                        pivotTransform = nodeInfo.m_data->placement().toAbsoluteTransform();
                        nodeInfo.m_data->placement(*rootTransformOverride);
                    }
                    else
                    {
                        auto relativeToPivot = nodeInfo.m_data->placement().toRelativeTransform(pivotTransform);
                        auto newAbsoluteTransform = *rootTransformOverride * relativeToPivot;
                        nodeInfo.m_data->placement(newAbsoluteTransform);
                    }
                }

                // create a node
                auto node = base::CreateSharedPtr<ContentNode>(nodeInfo.m_data);
                if (parentNode->addContent(node))
                {
                    resolvedParents[nodeId] = node;
                    outCreatedNodes.pushBack(node);
                }
            }

            // pasted
            return true;
        }

        void SceneEditMode_Objects::handlePaste()
        {
            // load content
            auto content = base::rtti_cast<scene::NodeTemplateContainer>(base::GetService<Editor>()->loadFromClipboard());
            if (!content)
            {
                ui::PostGeneralMessageWindow(m_propertiesBrowser->sharedFromThis(), ui::MessageType::Error, base::TempString("Nothing to pase from clipboard"));
                return;
            }

            // get root for pasting
            auto active = editor()->content().activeElement();
            if (!active)
            {
                ui::PostGeneralMessageWindow(m_propertiesBrowser->sharedFromThis(), ui::MessageType::Error, base::TempString("No context object for pasting selected"));
                return;
            }

            // get transform override
            const base::AbsoluteTransform* rootTransform = nullptr;
            if (!m_selectedNodes.empty())
                rootTransform = &m_selectedNodes[0]->absoluteTransform();

            // paste stuff
            base::Array<ContentNodePtr> pastedNodes;
            if (!pasteNodes(active, rootTransform, *content, pastedNodes))
            {
                ui::PostGeneralMessageWindow(m_propertiesBrowser->sharedFromThis(), ui::MessageType::Error, base::TempString("Unable to paste content"));
                return;
            }

            // select pasted nodes
            editor()->changeSelection(pastedNodes);
        }

        void SceneEditMode_Objects::handleDelete()
        {
            base::Array<ContentNodePtr> nodesToDelete;
            prepareHierarchyOfNodesFromSelection(nodesToDelete, true);
            TRACE_INFO("Discovered {} nodes to delete", nodesToDelete.size());
            
            // modify current selection
            auto newSelection = editor()->selectionSet();

            // remove in reverse order - children first
            for (int i = nodesToDelete.lastValidIndex(); i >= 0; --i)
            {
                auto& ptr = nodesToDelete[i];

                newSelection.remove(ptr);

                if (!ptr->isInstancedFromPrefab())
                {
                    auto parent  = ptr->parent();
                    if (parent)
                        parent->removeContent(ptr);
                }
            }

            // deselect all removed nodes
            editor()->changeSelection(newSelection.keys());
        }

        //---

        void SceneEditMode_Objects::selectionChanged(const base::Array<ContentNodePtr>& totalSelection)
        {
            // filter nodes
            m_selectedNodes = totalSelection;

            // create views for property grid
            {
                /*for (auto& element : m_selectedNodes)
                {
                    if (auto view = element->())
                        views.pushBack(view);
                }

                if (m_propertiesBrowser)
                {
                    // TODO: multiple objects
                    auto view = views.empty() ? nullptr : views.front();
                    m_propertiesBrowser->bind(view);
                }*/
            }

            // update visualization
            editor()->content().visualizeSelection(m_selectedNodes);

            // update pivot element for gizmo manager
            recrateGizmos();

            // sync tree view
            {
                if (auto sceneTree = editor()->findChildByName<ui::TreeView>("SceneTree"))
                {
                    base::Array<ui::ModelIndex> indices;
                    for (auto& node : m_selectedNodes)
                    {
                        if (auto index = node->modelIndex())
                            indices.pushBack(index);
                    }

                    sceneTree->select(indices, ui::ItemSelectionModeBit::Default, false);

                    if (!indices.empty())
                        sceneTree->ensureVisible(indices.front());
                }
            }
        }

        //--

        void SceneEditMode_Objects::recrateGizmos()
        {
            // destroy current gizmos
            m_gizmoManager->clear();

            // create gizmos only if there's something selected
            if (!m_selectedNodes.empty())
            {
                uint8_t axisMask = 0;
                axisMask |= m_gizmoSettings.m_enableX ? 1 : 0;
                axisMask |= m_gizmoSettings.m_enableY ? 2 : 0;
                axisMask |= m_gizmoSettings.m_enableZ ? 4 : 0;

                // create the translation group
                if (m_gizmoSettings.m_mode == ui::gizmo::GizmoMode::Translation)
                {
                    ui::gizmo::CreateTranslationArrowGizmos(m_gizmoManager.get(), this, axisMask);
                    ui::gizmo::CreateTranslationPlanesGizmos(m_gizmoManager.get(), this, axisMask);
                }
                // create the rotation group
                else if (m_gizmoSettings.m_mode == ui::gizmo::GizmoMode::Rotation)
                {
                    ui::gizmo::CreateRotationalGizmos(m_gizmoManager.get(), this, axisMask);
                }
            }
        }

        //--

#if 0

        SceneEditMode_Scene::SceneEditMode_Scene()
        {
            m_gizmoManager.create();
        }

        SceneEditMode_Scene::~SceneEditMode_Scene()
        {
            m_gizmoManager.reset();
        }

        bool SceneEditMode_Scene::handleAttach(SceneDocumentHandler* document)
        {
            activeDocument()->bindGizmoManager(m_gizmoManager.get());
            //getActiveDocument()->toggleHelperObjectsCategory("Test"_id, true);
            return true;
        }

        void SceneEditMode_Scene::handleDetach()
        {
            activeDocument()->bindGizmoManager(nullptr);
            //getActiveDocument()->toggleHelperObjectsCategory("Test"_id, false);
        }

        void SceneEditMode_Scene::handleViewportSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables, const SceneRenderingPanel* viewport)
        {
            /*// keep existing objects in the selection
            // NOTE: we re-resolve the objects just to make sure we can really edit them using this edit mode
            base::edit::DocumentObjectIDSet newSelection;

            // add/remove object from incoming set
            for (auto& selectable : selectables)
            {
                auto id = selectable.objectID();
                newSelection.add(id);
            }

            // auto include whole prefabs
            auto& selectionSettings = viewport->editor()->selectionSettings();
            if (selectionSettings.m_selectWholePrefabs)
            {
                // process selection once more
                auto oldSelection = newSelection;
                newSelection.clear();

                // if we find a prefab node get the actual prefab
                for (auto& id : oldSelection.ids())
                {
                    if (auto node = activeDocument()->contentTree().objectMap().findObjectOfClass<ContentNode>(id))
                    {
                        if (node->isInstancedFromPrefab())
                        {
                            if (auto prefabOwner = node->findPrefabOwnerNode())
                            {
                                newSelection.add(prefabOwner->documentId());
                            }
                        }
                        else
                        {
                            // non-prefab related object, keep as is
                            newSelection.add(id);
                        }
                    }
                    else
                    {
                        // not a node
                        newSelection.add(id);
                    }
                }
            }

            // determine the selection mode
            if (shift)
            {
                auto selection = activeDocument()->selection();
                for (auto& id : newSelection.ids())
                    selection.add(id);

                activeDocument()->changeSelection(selection);
            }
            else if (ctrl)
            {
                auto selection = activeDocument()->selection();
                for (auto& id : newSelection.ids())
                    if (selection.contains(id))
                        selection.remove(id);
                    else
                        selection.add(id);

                activeDocument()->changeSelection(selection);
            }
            else
            {
                activeDocument()->changeSelection(newSelection);
            }*/


         
        }

        bool SceneEditMode_Scene::handleContextMenu(const ui::Position& clickPosition, const SceneRenderingPanel* viewport, const base::edit::DocumentObjectIDSet& objectsUnderCursor, const base::Vector3* worldSpacePosition)
        {
            return false;
        }

        ui::InputActionPtr SceneEditMode_Scene::handleMouseClick(const base::input::MouseClickEvent& evt, const SceneRenderingPanel* viewport)
        {
            base::Point localPos(evt.absolutePosition() - viewport->cachedDrawArea().absolutePosition());

            // pass the LMB click to gizmos
            if (evt.leftClicked())
            {
                if (auto gizmoAction = m_gizmoManager->handleMouseClick(viewport->sharedFromThis(), localPos, viewport))
                    return gizmoAction;
            }

            return nullptr;
        }

        bool SceneEditMode_Scene::handleMouseMovement(const base::input::MouseMovementEvent& evt, const SceneRenderingPanel* viewport)
        {
            base::Point localPos(evt.absolutePosition() - viewport->cachedDrawArea().absolutePosition());

            // pass to gizmos
            m_gizmoManager->handleMouseMovement(viewport->sharedFromThis(), localPos, viewport);
            return true;
        }

        void SceneEditMode_Scene::handleTick(float dt)
        {
        }

        void SceneEditMode_Scene::handleRendering(rendering::scene::FrameInfo& frame, const SceneRenderingPanel* viewport)
        {
            ISceneEditMode::handleRendering(frame, viewport);

            // render gizmos
            m_gizmoManager->render(frame, viewport);

            // context object
            if (activeDocument())
            {
                rendering::runtime::CanvasDrawer dd(frame);

                if (activeDocument()->contextObject().empty())
                {
                    dd.canvasFillColor(base::Color::RED);
                    dd.addCanvasTextToGroup(rendering::runtime::DebugTextGroup::TopCenter, base::TempString("Context node: Not set"));
                }
                else
                {
                    if (auto exists = activeDocument()->contentTree().objectMap().findObject(activeDocument()->contextObject()))
                    {
                        dd.canvasFillColor(base::Color::GREEN);
                        dd.addCanvasTextToGroup(rendering::runtime::DebugTextGroup::TopCenter, base::TempString("Context node: '{}'", exists->path()));
                    }
                    else
                    {
                        dd.canvasFillColor(base::Color::RED);
                        dd.addCanvasTextToGroup(rendering::runtime::DebugTextGroup::TopCenter, base::TempString("Context node: '{}' (INVALID)", activeDocument()->contextObjectName()));
                    }
                }
            }
        }

        bool SceneEditMode_Scene::canCopy() const
        {
            return activeDocument() ? activeDocument()->canCopySelection() : false;
        }

        void SceneEditMode_Scene::handleCopy(const ui::ElementPtr& owner)
        {
            if (canCopy())
            {
                if (!activeDocument()->copyObjects(activeDocument()->selection()))
                    ui::PostGeneralMessageWindow(owner, ui::MessageType::Error, "Failed to copy selected object");
            }
            else
            {
                ui::PostGeneralMessageWindow(owner, ui::MessageType::Error, "Cannot copy selected objects");
            }
        }

        bool SceneEditMode_Scene::canCut() const
        {
            return activeDocument() ? (activeDocument()->canCopySelection() && activeDocument()->canDeleteSelection()) :  false;
        }

        void SceneEditMode_Scene::handleCut(const ui::ElementPtr& owner)
        {
            if (canCut())
            {
                if (!activeDocument()->cutObjects(activeDocument()->selection()))
                    ui::PostGeneralMessageWindow(owner, ui::MessageType::Error, "Failed to cut selected object");
            }
            else
            {
                ui::PostGeneralMessageWindow(owner, ui::MessageType::Error, "Cannot cut selected objects");
            }
        }

        bool SceneEditMode_Scene::canPaste() const
        {
            if (!activeDocument())
                return false;

            auto format = activeDocument()->clipboardDataFormat();
            if (!activeDocument()->clipboard().hasClipboardData(format))
                return false;

            if (activeDocument()->contextObject().empty() && activeDocument()->selection().empty())
                return false;

            return true;
        }

        void SceneEditMode_Scene::handlePaste(const ui::ElementPtr& owner)
        {
            if (activeDocument())
            {
                base::edit::DocumentObjectPlacementInfo placementInfo;

                // use the selection as paste root
                if (!activeDocument()->selection().empty())
                {
                    auto selectionId = activeDocument()->selection().ids().front();

                    if (activeDocument()->gizmoGetObjectParent(selectionId, placementInfo.m_overrideParentId))
                    {
                        if (activeDocument()->gizmoGetObjectAbsoluteTransform(selectionId, placementInfo.m_referencePlacement))
                        {
                            placementInfo.m_useReferencePlacement = true; // paste in relation to the selected object
                        }
                    }
                }

                // if we don't have a proper default use the layer
                if (placementInfo.m_overrideParentId.empty())
                {
                    placementInfo.m_overrideParentId = activeDocument()->contextObject();
                    placementInfo.m_useReferencePlacement = false; // paste where it was
                }

                activeDocument()->pasteObjects(placementInfo);
            }
        }

        bool SceneEditMode_Scene::canDelete() const
        {
            return activeDocument() ? activeDocument()->canDeleteSelection() : false;
        }

        void SceneEditMode_Scene::handleDelete(const ui::ElementPtr& owner)
        {
            if (canDelete())
                activeDocument()->deleteObjects(activeDocument()->selection());
            else
                ui::PostGeneralMessageWindow(owner, ui::MessageType::Error, "Cannot delete selected objects");
        }

        //---


        //--

        //RTTI_BEGIN_TYPE_CLASS(HelperObject_Test);
        //RTTI_END_TYPE();

        HelperObject_Test::HelperObject_Test(const base::RefPtr<ContentNode>& owner)
            : IHelperObject(owner)
        {
            base::random::FastGenerator rnd;

            for (uint32_t i=0; i<100; ++i)
            {
                auto& p = points.emplaceBack();
                p.m_localPos.x = -1.0f + 2.0f * rnd.nextFloat();
                p.m_localPos.y = -1.0f + 2.0f * rnd.nextFloat();
                p.m_localPos.z = -1.0f + 2.0f * rnd.nextFloat();
                p.id = base::edit::DocumentObjectID::CreateChildID(ownerId(), "Vertex", i, "Vertex"_id);\
                p.m_selected = false;
                m_pointMap[p.id] = i;
            }
        }

        base::RefPtr<HelperObject_Test> HelperObject_Test::CreateFromNode(const base::RefPtr<ContentNode>& node)
        {
            return base::CreateSharedPtr<HelperObject_Test>(node);
        }

        bool HelperObject_Test::placement(const base::edit::DocumentObjectID& id, base::AbsoluteTransform& outTransform)
        {
            auto node = owner();
            if (!node)
                return false;

            int pointIndex = -1;
            if (m_pointMap.find(id, pointIndex))
            {
                auto& point = points[pointIndex];
                outTransform = node->cachedAbsoluteTransform() * base::Transform(point.m_localPos);
                return true;
            }

            return false;
        }

        bool HelperObject_Test::placement(const base::edit::DocumentObjectID& id, const base::AbsoluteTransform& newTransform)
        {
            auto node = owner();
            if (!node)
                return false;

            int pointIndex = -1;
            if (m_pointMap.find(id, pointIndex))
            {
                auto& point = points[pointIndex];
                point.m_localPos = (newTransform / node->cachedAbsoluteTransform()).translation();
                return true;
            }

            return false;
        }

        void HelperObject_Test::handleSelectionChange(const base::edit::DocumentObjectIDSet& oldSelection, const base::edit::DocumentObjectIDSet& newSelection)
        {
            base::InplaceArray<base::edit::DocumentObjectID, 100> deselected, selected;
            newSelection.buildDiff(oldSelection, deselected, selected);

            for (auto& id : deselected)
            {
                int pointIndex = -1;
                if (m_pointMap.find(id, pointIndex))
                    points[pointIndex].m_selected = false;
            }

            for (auto& id : selected)
            {
                int pointIndex = -1;
                if (m_pointMap.find(id, pointIndex))
                    points[pointIndex].m_selected = true;
            }
        }

        static base::res::StaticResource<rendering::content::MaterialInstance> resIconPointLight("engine/icons/point_light.v4mi", true);

        void HelperObject_Test::handleRendering(const base::edit::DocumentObjectIDSet& currentSelection, rendering::scene::FrameInfo& frame)
        {
            auto node = owner();
            if (node)
            {
                auto& referenceTransform = node->cachedAbsoluteTransform();

                for (auto& point : points)
                {
                    auto pos = referenceTransform.transformPointFromSpace(point.m_localPos);

                    rendering::runtime::FrameGeometryParams params;
                    params.m_selected = point.m_selected;
                    params.size = -10.0f; // absolute size
                    params.m_material = resIconPointLight.loadAndGet().get() ? resIconPointLight.loadAndGet()->renderingObject() : nullptr;
                    params.selectable = point.id;
                    frame.addSprite(pos.approximate(), base::Color::WHITE, params);
                }
            }
        }

        //--

        //RTTI_BEGIN_TYPE_CLASS(HelperObjectHandler_Test);
        //RTTI_END_TYPE();

        HelperObjectHandler_Test::HelperObjectHandler_Test()
        {}

        bool HelperObjectHandler_Test::supportsObjectCategory(base::StringID category) const
        {
            return category == "Test"_id;
        }

        const base::RefPtr<IHelperObject> HelperObjectHandler_Test::createHelperObject(const base::RefPtr<ContentNode>& node) const
        {
            return base::CreateSharedPtr<HelperObject_Test>(node);
        }

        //--
#endif

    } // world
} // ui