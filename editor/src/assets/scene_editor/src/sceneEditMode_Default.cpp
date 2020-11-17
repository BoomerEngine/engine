/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#include "build.h"

#include "sceneEditMode_Default.h"
#include "sceneEditMode_Default_UI.h"
#include "sceneEditMode_Default_Transform.h"

#include "sceneContentNodes.h"
#include "sceneContentStructure.h"

#include "scenePreviewContainer.h"
#include "scenePreviewPanel.h"

#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiClassPickerBox.h"
#include "base/object/include/actionHistory.h"
#include "base/world/include/worldEntityTemplate.h"
#include "base/world/include/worldComponentTemplate.h"

namespace ed
{

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneEditMode_Default);
    RTTI_END_TYPE();
     
    SceneEditMode_Default::SceneEditMode_Default(ActionHistory* actionHistory)
        : ISceneEditMode(actionHistory)
    {
        m_panel = RefNew<SceneDefaultPropertyInspectorPanel>(this);

        m_entityClassSelector = RefNew<ui::ClassPickerBox>(base::world::EntityTemplate::GetStaticClass(), nullptr, false, false, "", false);
        m_componentClassSelector = RefNew<ui::ClassPickerBox>(base::world::ComponentTemplate::GetStaticClass(), nullptr, false, false, "", false);

        m_entityClassSelector->bind(ui::EVENT_CLASS_SELECTED) = [this](base::ClassType type)
        {
            createEntityAtNodes(m_selection.keys(), type);
        };

        m_componentClassSelector->bind(ui::EVENT_CLASS_SELECTED) = [this](base::ClassType type)
        {
            createComponentAtNodes(m_selection.keys(), type);
        };
    }

    SceneEditMode_Default::~SceneEditMode_Default()
    {}

    void SceneEditMode_Default::activeNode(SceneContentNode* node)
    {
        if (m_activeNode != node)
        {
            if (auto old = m_activeNode.lock())
                old->visualFlag(SceneContentNodeVisualBit::ActiveNode, false);

            m_activeNode = node;

            if (node)
                node->visualFlag(SceneContentNodeVisualBit::ActiveNode, true);
        }
    }

    void SceneEditMode_Default::focusNode(SceneContentNode* node)
    {
        if (node)
        {
            InplaceArray<SceneContentNodePtr, 1> nodes;
            nodes.emplaceBack(AddRef(node));
            focusNodes(nodes);
        }
    }

    void SceneEditMode_Default::focusNodes(const Array<SceneContentNodePtr>& nodes)
    {
        if (container())
            container()->focusNodes(nodes);
    }

    void SceneEditMode_Default::handleSelectionChanged()
    {
        m_panel->bind(m_selection.keys());

        container()->call(EVENT_EDIT_MODE_SELECTION_CHANGED);
    }

    void SceneEditMode_Default::handleTransformsChanged()
    {
        m_panel->refreshTransforms();
    }

    void SceneEditMode_Default::updateSelectionFlags()
    {
        m_canCopySelection = false;
        m_canCutSelection = false;
        m_canDeleteSelection = false;

        if (!m_selection.empty())
        {
            m_canCopySelection = true;
            m_canCutSelection = true;
            m_canDeleteSelection = true;

            bool hasComponent = false;
            bool hasEntity = false;
            for (const auto& data : m_selection.keys())
            {
                if (data->type() == SceneContentNodeType::Entity || data->type() == SceneContentNodeType::Component)
                {
                    if (data->type() == SceneContentNodeType::Entity)
                        hasEntity = true;
                    else
                        hasComponent = true;

                    const auto* dataNode = static_cast<const SceneContentDataNode*>(data.get());
                    if (dataNode->baseData())
                    {
                        m_canCutSelection = false;
                        m_canDeleteSelection = false;
                    }
                }
                else
                {
                    m_canCopySelection = false;
                    m_canCutSelection = false;
                    m_canDeleteSelection = false;
                }
            }

            if (hasEntity && hasComponent)
            {
                m_canCopySelection = false;
                m_canCutSelection = false;
            }
        }
    }

    void SceneEditMode_Default::changeSelection(const Array<SceneContentNodePtr>& selection)
    {
        HashSet<SceneContentNodePtr> newSelectionSet;
        newSelectionSet.reserve(selection.size());

        for (const auto& ptr : selection)
            newSelectionSet.insert(ptr);

        for (const auto& cur : m_selection)
            if (!newSelectionSet.contains(cur))
                cur->visualFlag(SceneContentNodeVisualBit::SelectedNode, false);

        for (const auto& cur : newSelectionSet)
            if (!m_selection.contains(cur))
                cur->visualFlag(SceneContentNodeVisualBit::SelectedNode, true);

        m_selection = std::move(newSelectionSet);

        updateSelectionFlags();
        handleSelectionChanged();
    }

    void SceneEditMode_Default::reset()
    {
        m_selection.reset();
    }

    ui::ElementPtr SceneEditMode_Default::queryUserInterface() const
    {
        return m_panel;
    }

    bool SceneEditMode_Default::hasSelection() const
    {
        return !m_selection.empty();
    }

    Array<SceneContentNodePtr> SceneEditMode_Default::querySelection() const
    {
        return m_selection.keys();
    }

    void SceneEditMode_Default::configurePanelToolbar(ScenePreviewContainer* container, const ScenePreviewPanel* panel, ui::ToolBar* toolbar)
    {
        CreateDefaultGridButtons(container, toolbar);
        CreateDefaultSelectionButtons(container, toolbar);
        CreateDefaultGizmoButtons(container, toolbar);
    }

    void SceneEditMode_Default::changeGizmo(SceneGizmoMode mode)
    {
        if (container())
        {
            auto config = container()->gizmoSettings();
            if (mode != config.mode)
            {
                config.mode = mode;
                container()->gizmoSettings(config);
            }
        }
    }

    void SceneEditMode_Default::changeGizmoNext()
    {
        if (container())
        {
            auto config = container()->gizmoSettings();

            if (config.mode == SceneGizmoMode::Translation)
                config.mode = SceneGizmoMode::Rotation;
            else if (config.mode == SceneGizmoMode::Rotation)
                config.mode = SceneGizmoMode::Scale;
            else if (config.mode == SceneGizmoMode::Scale)
                config.mode = SceneGizmoMode::Translation;

            container()->gizmoSettings(config);
        }
    }

    void SceneEditMode_Default::changePositionGridSize(int delta)
    {

    }

    void SceneEditMode_Default::configureEditMenu(ui::MenuButtonContainer* menu)
    {
        menu->createCallback("Translation", "[img:gizmo_translate16]", "W") = [this]()
        {
            changeGizmo(SceneGizmoMode::Translation);
        };

        menu->createCallback("Rotation", "[img:gizmo_rotate16]", "E") = [this]()
        {
            changeGizmo(SceneGizmoMode::Rotation);
        };

        menu->createCallback("Scale", "[img:gizmo_scale16]", "R") = [this]()
        {
            changeGizmo(SceneGizmoMode::Scale);
        };

        menu->createSeparator();

        menu->createCallback("Next gizmo", "", "Space") = [this]()
        {
            changeGizmoNext();
        };

        menu->createSeparator();
    }

    void SceneEditMode_Default::configureViewMenu(ui::MenuButtonContainer* menu)
    {
        menu->createCallback("Focus on selection", "[img:zoom]", "F", !m_selection.empty()) = [this]()
        {
            focusNodes(m_selection.keys());
        };

        menu->createSeparator();

        menu->createCallback("Hide selected", "[color:#888][img:eye][/color]", "H", !m_selection.empty()) = [this]()
        {
            processObjectHide(m_selection.keys());
        };

        menu->createCallback("Show selected", "[img:eye]", "Ctrl+H", !m_selection.empty()) = [this]()
        {
            processObjectShow(m_selection.keys());
        };

        menu->createSeparator();

        menu->createCallback("Unhide all", "[img:eye_cross]", "Shift+H") = [this]()
        {
            processUnhideAll();
        };

        menu->createSeparator();

    }

    GizmoGroupPtr SceneEditMode_Default::configurePanelGizmos(ScenePreviewContainer* container, const ScenePreviewPanel* panel)
    {
        if (!m_selection.empty())
        {
            const auto& settings = container->gizmoSettings();
            if (settings.mode == SceneGizmoMode::Translation)
            {
                uint8_t axisMask = 7; // TODO: filter based on the panel camera
                return CreateTranslationGizmos(panel, axisMask);
            }
        }

        return nullptr;
    }

    GizmoReferenceSpace SceneEditMode_Default::calculateGizmoReferenceSpace() const
    {
        if (!m_selection.empty() && container())
        {
            // get the root object for the reference space - we can make a config for this but for now use the first one
            if (auto root = rtti_cast<SceneContentDataNode>(m_selection.keys().front()))
            {
                const auto& settings = container()->gizmoSettings();
                const auto& transform = root->localToWorldTransform();

                if (settings.space == GizmoSpace::Local)
                {
                    return GizmoReferenceSpace(settings.space, transform);
                }
                else if (settings.space == GizmoSpace::Parent)
                {
                    if (auto rootParent = rtti_cast<SceneContentDataNode>(root->parent()))
                        return GizmoReferenceSpace(settings.space, rootParent->localToWorldTransform());
                }

                const auto rootPosition = transform.position();
                return GizmoReferenceSpace(GizmoSpace::World, rootPosition);
            }
        }

        return GizmoReferenceSpace();
    }

    //--

    void SceneEditMode_Default::EnsureParentsFirst(const Array<SceneContentNodePtr>& nodes, Array<SceneContentDataNodePtr>& outTransformList)
    {
        HashSet<SceneContentDataNode*> selectedNodes;

        selectedNodes.reserve(nodes.size());
        outTransformList.reserve(nodes.size());

        for (const auto& node : nodes)
        {
            if (auto* dataNode = rtti_cast<SceneContentDataNode>(node.get()))
            {
                selectedNodes.insert(dataNode);
                dataNode->tempFlag = false;
            }
        }

        InplaceArray<SceneContentDataNode*, 16> parentChain;

        for (auto* node : selectedNodes.keys())
        {
            parentChain.reset();

            while (node)
            {
                parentChain.pushBack(node);
                node = rtti_cast<SceneContentDataNode>(node->parent());
            }

            for (int i : parentChain.indexRange().reversed())
            {
                auto* nodeToAdd = parentChain[i];
                if (selectedNodes.contains(nodeToAdd) && !nodeToAdd->tempFlag)
                {
                    nodeToAdd->tempFlag = true;
                    outTransformList.pushBack(AddRef(nodeToAdd));
                }
            }
        }
    }

    void SceneEditMode_Default::ExtractSelectionRoots(const Array<SceneContentNodePtr>& nodes, Array<SceneContentDataNodePtr>& outRoots)
    {
        HashSet<SceneContentDataNode*> selectedNodes;
        selectedNodes.reserve(nodes.size());
        outRoots.reserve(nodes.size());

        for (const auto& node : nodes)
            if (auto* dataNode = rtti_cast<SceneContentDataNode>(node.get()))
                selectedNodes.insert(dataNode);

        for (auto* node : selectedNodes.keys())
        {
            bool parentSelected = false;

            if (auto parent = rtti_cast<SceneContentDataNode>(node->parent()))
            {
                while (parent)
                {
                    if (selectedNodes.contains(parent))
                    {
                        parentSelected = true;
                        break;
                    }

                    parent = rtti_cast<SceneContentDataNode>(parent->parent());
                }
            }

            if (!parentSelected)
                outRoots.pushBack(AddRef(node));
        }
    }

    void SceneEditMode_Default::ExtractSelectionRoots(const Array<SceneContentNodePtr>& nodes, Array<SceneContentNodePtr>& outRoots)
    {
        HashSet<SceneContentNode*> selectedNodes;
        selectedNodes.reserve(nodes.size());
        outRoots.reserve(nodes.size());

        for (const auto& node : nodes)
            selectedNodes.insert(node);

        for (auto* node : selectedNodes.keys())
        {
            bool parentSelected = false;

            if (auto parent = rtti_cast<SceneContentNode>(node->parent()))
            {
                while (parent)
                {
                    if (selectedNodes.contains(parent))
                    {
                        parentSelected = true;
                        break;
                    }

                    parent = parent->parent();
                }
            }

            if (!parentSelected)
                outRoots.pushBack(AddRef(node));
        }
    }

    void SceneEditMode_Default::ExtractSelectionHierarchy(const SceneContentDataNode* node, Array<SceneContentDataNodePtr>& outNodes)
    {
        outNodes.pushBack(AddRef(node));

        for (const auto& comp : node->components())
            outNodes.pushBack(comp);

        for (const auto& child : node->entities())
            ExtractSelectionHierarchy(child, outNodes);
    }

    void SceneEditMode_Default::ExtractSelectionHierarchyWithFilter(const SceneContentDataNode* node, Array<SceneContentDataNodePtr>& outNodes, const HashSet<SceneContentDataNode*>& coreSet, int depth)
    {
        if (depth == 0 || !coreSet.contains(node))
            outNodes.pushBack(AddRef(node));

        for (const auto& child : node->components())
            ExtractSelectionHierarchyWithFilter(child, outNodes, coreSet, depth + 1);

        for (const auto& child : node->entities())
            ExtractSelectionHierarchyWithFilter(child, outNodes, coreSet, depth + 1);
    }

    void SceneEditMode_Default::ExtractSelectionHierarchyWithFilter2(const SceneContentDataNode* node, Array<SceneContentDataNodePtr>& outNodes, const HashMap<SceneContentNode*, int>& coreSet)
    {
        if (!coreSet.contains(node))
            outNodes.pushBack(AddRef(node));

        for (const auto& child : node->components())
            ExtractSelectionHierarchyWithFilter2(child, outNodes, coreSet);

        for (const auto& child : node->entities())
            ExtractSelectionHierarchyWithFilter2(child, outNodes, coreSet);
    }

    void SceneEditMode_Default::buildTransformNodeListFromSelection(Array<SceneContentDataNodePtr>& outTransformList) const
    {
        const auto target = container()->gizmoSettings().target;
        if (target == SceneGizmoTarget::WholeHierarchy)
        {
            Array<SceneContentDataNodePtr> roots;
            ExtractSelectionRoots(m_selection.keys(), roots);

            for (const auto& root : roots)
                ExtractSelectionHierarchy(root, outTransformList);
        }
        else if (target == SceneGizmoTarget::SelectionOnly)
        {
            EnsureParentsFirst(m_selection.keys(), outTransformList);
        }
    }

    //--

    GizmoActionContextPtr SceneEditMode_Default::createGizmoAction(ScenePreviewContainer* container, const ScenePreviewPanel* panel) const
    {
        Array<SceneContentDataNodePtr> transformNodes;
        buildTransformNodeListFromSelection(transformNodes);

        return RefNew<SceneEditModeDefaultTransformAction>(const_cast<SceneEditMode_Default*>(this), panel, transformNodes, container->gridSettings(), container->gizmoSettings());
    }

    void SceneEditMode_Default::handleRender(ScenePreviewPanel* panel, rendering::scene::FrameParams& frame)
    {
        TBaseClass::handleRender(panel, frame);
    }

    ui::InputActionPtr SceneEditMode_Default::handleMouseClick(ScenePreviewPanel* panel, const input::MouseClickEvent& evt)
    {
        return nullptr;
    }

    bool SceneEditMode_Default::handleKeyEvent(ScenePreviewPanel* panel, const base::input::KeyEvent& evt)
    {
        if (evt.pressed())
        {
            const auto buttonAlt = evt.keyMask().isAltDown();
            const auto buttonCtrl = evt.keyMask().isCtrlDown();
            const auto buttonShift = evt.keyMask().isShiftDown();
            return handleInternalKeyAction(evt.keyCode(), buttonShift, buttonAlt, buttonCtrl);
        }

        return false;
    }

    void SceneEditMode_Default::processVisualSelection(bool ctrl, bool shift, const base::Array<rendering::scene::Selectable>& selectables)
    {
        base::HashSet<SceneContentNodePtr> selectedNodes;

        if (shift || ctrl)
            selectedNodes = m_selection;

        for (const auto& selectable : selectables)
        {
            if (const auto node = container()->resolveSelectable(selectable))
            {
                if (ctrl)
                {
                    if (selectedNodes.contains(node))
                        selectedNodes.remove(node);
                    else
                        selectedNodes.insert(node);
                }
                else
                {
                    selectedNodes.insert(node);
                }
            }
        }

        actionChangeSelection(selectedNodes.keys());
    }

    void SceneEditMode_Default::handlePointSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables)
    {
        processVisualSelection(ctrl, shift, selectables);
    }

    void SceneEditMode_Default::handleAreaSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
    {
        processVisualSelection(ctrl, shift, selectables);
    }

    void SceneEditMode_Default::handleUpdate(float dt)
    {

    }

    void SceneEditMode_Default::handleTreeSelectionChange(const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection)
    {
        actionChangeSelection(selection);
    }

    //--
    
} // ed
