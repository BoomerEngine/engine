/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes #]
***/

#include "build.h"
#include "sceneEditMode.h"
#include "scenePreviewPanel.h"
#include "engine/ui/include/uiToolBar.h"
#include "scenePreviewContainer.h"
#include "engine/ui/include/uiButton.h"
#include "engine/ui/include/uiMenuBar.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--
     
RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISceneEditMode);
RTTI_END_TYPE();

ISceneEditMode::ISceneEditMode(ActionHistory* actionHistory)
    : m_actionHistory(actionHistory)
{
}

ISceneEditMode::~ISceneEditMode()
{
    DEBUG_CHECK(m_container == nullptr);
}

void ISceneEditMode::acivate(ScenePreviewContainer* container)
{
    DEBUG_CHECK_RETURN(container != nullptr);
    DEBUG_CHECK(m_container == nullptr);
    m_container = container;
}

void ISceneEditMode::deactivate(ScenePreviewContainer* container)
{
    DEBUG_CHECK_RETURN(container == m_container);
    m_container = nullptr;
}

ui::ElementPtr ISceneEditMode::queryUserInterface() const
{
    return nullptr;
}

Array<SceneContentNodePtr> ISceneEditMode::querySelection() const
{
    return {};
}

bool ISceneEditMode::hasSelection() const
{
    return false;
}

GizmoGroupPtr ISceneEditMode::configurePanelGizmos(ScenePreviewContainer* container, const ScenePreviewPanel* panel)
{
    return nullptr;
}

GizmoReferenceSpace ISceneEditMode::calculateGizmoReferenceSpace() const
{
    return GizmoReferenceSpace();
}

GizmoActionContextPtr ISceneEditMode::createGizmoAction(ScenePreviewContainer* container, const ScenePreviewPanel* panel) const
{
    return nullptr;
}

void ISceneEditMode::configureEditMenu(ui::MenuButtonContainer* menu)
{}

void ISceneEditMode::configureViewMenu(ui::MenuButtonContainer* menu)
{}

void ISceneEditMode::configurePanelToolbar(ScenePreviewContainer* container, const ScenePreviewPanel* panel, ui::ToolBar* toolbar)
{
}
    
void ISceneEditMode::handleRender(ScenePreviewPanel* panel, rendering::FrameParams& frame)
{
}

ui::InputActionPtr ISceneEditMode::handleMouseClick(ScenePreviewPanel* panel, const input::MouseClickEvent& evt)
{
    return nullptr;
}

bool ISceneEditMode::handleKeyEvent(ScenePreviewPanel* panel, const input::KeyEvent& evt)
{
    return false;
}

void ISceneEditMode::handlePointSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const Point& clientPosition, const Array<Selectable>& selectables)
{}

void ISceneEditMode::handleAreaSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const Rect& clientRect, const Array<Selectable>& selectables)
{}

void ISceneEditMode::handleContextMenu(ScenePreviewPanel* panel, bool ctrl, bool shift, const ui::Position& absolutePosition, const Point& clientPosition, const Selectable& objectUnderCursor, const ExactPosition* positionUnderCursor)
{}

ui::DragDropHandlerPtr ISceneEditMode::handleDragDrop(ScenePreviewPanel* panel, const ui::DragDropDataPtr& data, const ui::Position& absolutePosition, const Point& clientPosition)
{
    return nullptr;
}

void ISceneEditMode::handleUpdate(float dt)
{}

void ISceneEditMode::handleTreeContextMenu(ui::MenuButtonContainer* menu, const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection)
{}

void ISceneEditMode::handleTreeSelectionChange(const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection)
{}

void ISceneEditMode::handleTreeDeleteNodes(const Array<SceneContentNodePtr>& selection)
{}

void ISceneEditMode::handleTreeCutNodes(const Array<SceneContentNodePtr>& selection)
{}

void ISceneEditMode::handleTreeCopyNodes(const Array<SceneContentNodePtr>& selection)
{}

void ISceneEditMode::handleTreePasteNodes(const SceneContentNodePtr& target, SceneContentNodePasteMode mode)
{}

bool ISceneEditMode::handleTreeResourceDrop(const SceneContentNodePtr& target, StringView depotFilePath)
{
    return false;
}

bool ISceneEditMode::handleTreeNodeDrop(const SceneContentNodePtr& target, const SceneContentNodePtr& source)
{
    return false;
}

//--

void ISceneEditMode::handleGeneralCopy()
{}

void ISceneEditMode::handleGeneralCut()
{}

void ISceneEditMode::handleGeneralPaste()
{}

void ISceneEditMode::handleGeneralDelete()
{}

void ISceneEditMode::handleGeneralDuplicate()
{}

bool ISceneEditMode::checkGeneralCopy() const
{
    return false;
}

bool ISceneEditMode::checkGeneralCut() const
{
    return false;
}

bool ISceneEditMode::checkGeneralPaste() const
{
    return false;
}

bool ISceneEditMode::checkGeneralDelete() const
{
    return false;
}

bool ISceneEditMode::checkGeneralDuplicate() const
{
    return false;
}

//--

void ISceneEditMode::configSave(ScenePreviewContainer* container, const ui::ConfigBlock& block) const
{
}

void ISceneEditMode::configLoad(ScenePreviewContainer* container, const ui::ConfigBlock& block)
{
}

//--

static void PrintGridSize(IFormatStream& f, float size)
{
    if (size < 0.01f)
        f.appendf("{}mm", (int)(size * 1000.0f));
    else if (size < 1.00f)
        f.appendf("{}cm", (int)(size * 100.0f));
    else
        f.appendf("{}m", (int)size);
}

static void PrintRotationSize(IFormatStream& f, float size)
{
    if (size < 1.0f)
        f.appendf("{}'", (int)(size * 60.0f));
    else if (std::trunc(size) < 0.001f)
        f.appendf("{}�", (int)size);
    else if (std::trunc(size*10) < 0.001f)
        f.appendf("{}�", Prec(size, 1)); 
    else
        f.appendf("{}�", Prec(size, 2));
}

void CreateDefaultGridButtons(ScenePreviewContainer* container, ui::ToolBar* toolbar)
{
    auto gridSettings = container->gridSettings();

    toolbar->createSeparator();

    // position grid
    {
        StringBuilder txt;

        if (!gridSettings.positionGridEnabled)
            txt << "[color:#8888]";

        txt << "[img:grid] ";

        PrintGridSize(txt, gridSettings.positionGridSize);

        toolbar->createButton(ui::ToolBarButtonInfo().caption(txt.view())) = [container](ui::Button* button)
        {
            static const float GridSizes[] = { 0.01f, 0.02f, 0.05f, 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 25.0f, 50.0f, 100.0f };

            auto menu = RefNew<ui::MenuButtonContainer>();
            auto gridSettings = container->gridSettings();

            menu->createCallback("Enable", gridSettings.positionGridEnabled ? "[img:tick]" : "") = [container]() {
                auto gridSettings = container->gridSettings();
                gridSettings.positionGridEnabled = !gridSettings.positionGridEnabled;
                container->gridSettings(gridSettings);
            };

            menu->createSeparator();

            for (auto size : GridSizes)
            {
                StringBuilder txt;
                PrintGridSize(txt, size);

                float sizeCopy = size; // compiler crash ;)

                menu->createCallback(txt.view(), size == gridSettings.positionGridSize ? "[img:tick]" : "") = [container, sizeCopy]() {
                    auto gridSettings = container->gridSettings();
                    gridSettings.positionGridSize = sizeCopy;
                    container->gridSettings(gridSettings);
                };
            }

            menu->showAsDropdown(button);
        };
    }

    // rotation grid
    {
        StringBuilder txt;

        if (!gridSettings.rotationGridEnabled)
            txt << "[color:#8888]";

        txt << "[img:angle] ";

        PrintRotationSize(txt, gridSettings.rotationGridSize);

        toolbar->createButton(ui::ToolBarButtonInfo().caption(txt.view())) = [container](ui::Button* button)
        {
            static const float GridSizes[] = { 1.0f, 3.0f, 4.5f, 6.0f, 7.5f, 10.0f, 12.25f, 15.0f, 22.5f, 30.0f, 45.0f, 60.0f, 90.0f };

            auto menu = RefNew<ui::MenuButtonContainer>();
            auto gridSettings = container->gridSettings();

            menu->createCallback("Enable", gridSettings.rotationGridEnabled ? "[img:tick]" : "") = [container]() {
                auto gridSettings = container->gridSettings();
                gridSettings.rotationGridEnabled = !gridSettings.rotationGridEnabled;
                container->gridSettings(gridSettings);
            };

            menu->createSeparator();

            for (auto size : GridSizes)
            {
                StringBuilder txt;
                PrintRotationSize(txt, size);

                float sizeCopy = size; // compiler crash ;)

                menu->createCallback(txt.view(), size == gridSettings.rotationGridSize ? "[img:tick]" : "") = [container, sizeCopy]() {
                    auto gridSettings = container->gridSettings();
                    gridSettings.rotationGridSize = sizeCopy;
                    container->gridSettings(gridSettings);
                };
            }

            menu->showAsDropdown(button);
        };
    }

    // snapping to features
    {
        toolbar->createButton(ui::ToolBarButtonInfo().caption("[img:snap] Snap")) = [container](ui::Button* button)
        {
            auto menu = RefNew<ui::MenuButtonContainer>();

            auto gridSettings = container->gridSettings();

            menu->createCallback("Enable", gridSettings.featureSnappingEnabled ? "[img:tick]" : "") = [container]() {
                auto gridSettings = container->gridSettings();
                gridSettings.featureSnappingEnabled = !gridSettings.featureSnappingEnabled;
                container->gridSettings(gridSettings);
            };

            menu->createCallback("Rotation", gridSettings.rotationSnappingEnabled ? "[img:tick]" : "") = [container]() {
                auto gridSettings = container->gridSettings();
                gridSettings.rotationSnappingEnabled = !gridSettings.rotationSnappingEnabled;
                container->gridSettings(gridSettings);
            };

            menu->showAsDropdown(button);
        };
    }

    toolbar->createSeparator();
}

//--

void CreateDefaultSelectionButtons(ScenePreviewContainer* container, ui::ToolBar* toolbar)
{
    toolbar->createSeparator();

    // snapping to features
    {
        toolbar->createButton(ui::ToolBarButtonInfo().caption("[img:selection_point] Selection")) = [container](ui::Button* button)
        {
            auto menu = RefNew<ui::MenuButtonContainer>();

            auto selectionSettings = container->selectionSettings();

            menu->createCallback("[img:brick] Explore prefabs", selectionSettings.explorePrefabs ? "[img:tick]" : "") = [container]() {
                auto data = container->selectionSettings();
                data.explorePrefabs = !data.explorePrefabs;
                container->selectionSettings(data);
            };

            menu->createCallback("[img:transparent] Select transparent", selectionSettings.allowTransparent ? "[img:tick]" : "") = [container]() {
                auto data = container->selectionSettings();
                data.allowTransparent = !data.allowTransparent;
                container->selectionSettings(data);
            };

            menu->showAsDropdown(button);
        };
    }

    toolbar->createSeparator();
}

//--

static void PrintGizmoSpace(IFormatStream& f, GizmoSpace space)
{
    switch (space)
    {
        case GizmoSpace::World: f << "[img:world] World"; break;
        case GizmoSpace::Local: f << "[img:axis_local] Local"; break;
        case GizmoSpace::Parent: f << "[img:axis_global] Parent"; break;
        case GizmoSpace::View: f << "[img:camera] View"; break;
    }
}

static void PrintGizmoTarget(IFormatStream& f, SceneGizmoTarget space)
{
    switch (space)
    {
        case SceneGizmoTarget::WholeHierarchy: f << "[img:home] Hierarchy"; break;
        case SceneGizmoTarget::SelectionOnly: f << "[img:node] Single"; break;
    }
}

void CreateDefaultGizmoButtons(ScenePreviewContainer* container, ui::ToolBar* toolbar)
{
    auto data = container->gizmoSettings();

    toolbar->createSeparator();

    {
        const auto title = (data.mode == SceneGizmoMode::Translation) ? "[img:gizmo_translate16]" : "[color:#8888][img:gizmo_translate16]";
        toolbar->createButton(ui::ToolBarButtonInfo().caption(title)) = [container](ui::Button* button)
            {
                auto data = container->gizmoSettings();
                data.mode = SceneGizmoMode::Translation;
                container->gizmoSettings(data);
            };
    }

    {
        const auto title = (data.mode == SceneGizmoMode::Rotation) ? "[img:gizmo_rotate16]" : "[color:#8888][img:gizmo_rotate16]";
        toolbar->createButton(ui::ToolBarButtonInfo().caption(title)) = [container](ui::Button* button)
        {
            auto data = container->gizmoSettings();
            data.mode = SceneGizmoMode::Rotation;
            container->gizmoSettings(data);
        };
    }

    {
        const auto title = (data.mode == SceneGizmoMode::Scale) ? "[img:gizmo_scale16]" : "[color:#8888][img:gizmo_scale16]";
        toolbar->createButton(ui::ToolBarButtonInfo().caption(title)) = [container](ui::Button* button)
        {
            auto data = container->gizmoSettings();
            data.mode = SceneGizmoMode::Scale;
            container->gizmoSettings(data);
        };
    }

    toolbar->createSeparator();

    // space
    {
        StringBuilder txt;

        PrintGizmoSpace(txt, data.space);

        toolbar->createButton(ui::ToolBarButtonInfo().caption(txt.view())) = [container](ui::Button* button)
        {
            static const GizmoSpace GizmoSpaces[] = { GizmoSpace::World, GizmoSpace::Local, GizmoSpace::Parent, GizmoSpace::View };

            auto menu = RefNew<ui::MenuButtonContainer>();
            auto data = container->gizmoSettings();

            for (auto space : GizmoSpaces)
            {
                StringBuilder txt;
                PrintGizmoSpace(txt, space);

                auto spaceCopy = space; // compiler crash ;)

                menu->createCallback(txt.view(), space == data.space ? "[img:tick]" : "") = [container, spaceCopy]() {
                    auto data = container->gizmoSettings();
                    data.space = spaceCopy;
                    container->gizmoSettings(data);
                };
            }

            menu->showAsDropdown(button);
        };
    }

    // movement mode
    {
        StringBuilder txt;

        PrintGizmoTarget(txt, data.target);

        toolbar->createButton(ui::ToolBarButtonInfo().caption(txt.view())) = [container](ui::Button* button)
        {
            static const SceneGizmoTarget GizmoSpaces[] = { SceneGizmoTarget::WholeHierarchy, SceneGizmoTarget::SelectionOnly };

            auto menu = RefNew<ui::MenuButtonContainer>();
            auto data = container->gizmoSettings();

            for (auto space : GizmoSpaces)
            {
                StringBuilder txt;
                PrintGizmoTarget(txt, space);

                auto spaceCopy = space; // compiler crash ;)

                menu->createCallback(txt.view(), space == data.target ? "[img:tick]" : "") = [container, spaceCopy]() {
                    auto data = container->gizmoSettings();
                    data.target = spaceCopy;
                    container->gizmoSettings(data);
                };
            }

            menu->showAsDropdown(button);
        };
    }

    toolbar->createSeparator();

    {
        const auto title = data.enableX ? "[img:lock_x]" : "[color:#8888][img:lock_x]";
        toolbar->createButton(ui::ToolBarButtonInfo().caption(title)) = [container](ui::Button* button)
        {
            auto data = container->gizmoSettings();
            data.enableX = !data.enableX;
            container->gizmoSettings(data);
        };
    }

    {
        const auto title = data.enableY ? "[img:lock_y]" : "[color:#8888][img:lock_y]";
        toolbar->createButton(ui::ToolBarButtonInfo().caption(title)) = [container](ui::Button* button)
        {
            auto data = container->gizmoSettings();
            data.enableY = !data.enableY;
            container->gizmoSettings(data);
        };
    }

    {
        const auto title = data.enableZ ? "[img:lock_z]" : "[color:#8888][img:lock_z]";
        toolbar->createButton(ui::ToolBarButtonInfo().caption(title)) = [container](ui::Button* button)
        {
            auto data = container->gizmoSettings();
            data.enableZ = !data.enableZ;
            container->gizmoSettings(data);
        };
    }

    toolbar->createSeparator();
}

//--

void CreateDefaultCreationButtons(ScenePreviewContainer* container, ui::ToolBar* toolbar)
{

}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
