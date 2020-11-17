/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "sceneEditor.h"
#include "sceneEditMode.h"
#include "sceneNodePalette.h"
#include "sceneRenderingContainer.h"
#include "sceneEditorStructure.h"
#include "sceneEditorStructureLayer.h"
#include "sceneEditorStructureNode.h"
#include "sceneEditorTempObjects.h"

#include "ui/models/include/uiTreeView.h"
#include "ui/widgets/include/uiDockNotebook.h"
#include "ui/widgets/include/uiMessageBox.h"
#include "ui/inspector/include/uiDataInspector.h"
#include "ui/widgets/include/uiWindowMessage.h"
#include "ui/toolkit/include/uiTooltip.h"
#include "ui/widgets/include/uiComboBox.h"
#include "ui/widgets/include/uiToolBar.h"
#include "base/image/include/image.h"

#include "editor/asset_browser/include/resourceEditorOpener.h"
#include "editor/asset_browser/include/resourceBrowserNotebook.h"

#include "scene/common/include/sceneRuntime.h"
#include "scene/common/include/sceneWorld.h"
#include "sceneEditorStructureNode.h"

namespace ed
{
    namespace world
    {

        //---

        static base::res::StaticResource<base::storage::XMLData> resMainWindow("engine/ui/templates/scene/scene_editor.xml");

        //--

        base::StringBuf GetConfigGroupNameForFileType(const ed::depot::ManagedFilePtr& fileHandler)
        {
            auto fileClass = fileHandler->name().stringAfterLast(".");
            return base::TempString("ResourcesEditor.Type.{}", fileClass);
        }

        base::StringBuf GetConfigGroupNameForFilePath(const ed::depot::ManagedFilePtr& fileHandler)
        {
            auto filePath = fileHandler->depotPath();
            filePath.replaceChar('/', '_');
            filePath.replaceChar('.', '_');
            filePath.replaceChar(' ', '_');
            return base::TempString("ResourcesEditor.File.{}", filePath);
        }

        //--

        RTTI_BEGIN_TYPE_CLASS(SceneEditorTab);
        RTTI_END_TYPE();

        SceneEditorTab::SceneEditorTab(const depot::ManagedFilePtr& file)
            : EditorTab(base::GetService<Editor>()->configRoot()["Resource"][GetConfigGroupNameForFileType(file).c_str()])
            , m_fileConfigPath(base::GetService<Editor>()->configRoot()["File"][GetConfigGroupNameForFilePath(file).c_str()])
        {
            ui::ApplyTemplate(this, resMainWindow.loadAndGet());

            m_content = base::RefNew<ContentStructure>(file);

            m_scene = base::RefNew<scene::Scene>(scene::SceneType::Editor);
            m_content->attachScene(m_scene);

            m_tempObjects = base::RefNew<SceneTempObjectSystem>(m_scene);

            m_gizmoSettings.load(m_fileConfigPath);
            m_gridSettings.load(m_fileConfigPath);
            m_selectionSettings.load(m_fileConfigPath);
            
            initializeActions();
            initializePanels();
            initializeGridTables();
            initializePreviewPanel();
            initializeEditModes();

            fillPositionGridSizes();
            fillRotationGridSizes();
            fillAreaSelectionModes();
            fillPointSelectionModes();
            fillGizmoSpaceModes();
            fillGizmoTargetModes();

            updateGridSetup();

            startRepeatedTimer("SceneTick", 0.001f, [this](UI_TIMER) { internalTick(elapsedTime); });

            restoreSceneConfig();
        }

        SceneEditorTab::~SceneEditorTab()
        {
            removeAllChildren();

            m_content->deattachScene();
            m_content.reset();

            m_tempObjects.reset();
            m_scene.reset();
            

            ASSERT(m_renderingPanels.empty());
        }

        void SceneEditorTab::postInternalConfiguration(base::ClassType classType, const void* data)
        {
            for (auto& mode : m_allEditModes)
            {
                auto isActive = (mode == m_activeEditMode);
                mode->configure(classType, data, isActive);
            }
        }

        void SceneEditorTab::internalTick(float dt)
        {
            // tick the temp object system
            m_tempObjects->tick(dt);

            // tick the scene
            scene::UpdateContext context;
            context.m_dt = dt;
            m_scene->update(context);

            // update active edit mode
            if (m_activeEditMode)
                m_activeEditMode->handleTick(dt);
        }

        void SceneEditorTab::restoreSceneConfig()
        {
            // restore active element
            if (auto pathStr = m_fileConfigPath.get<base::StringBuf>("ActiveElement"))
            {
                scene::NodePath path;
                if (scene::NodePath::Parse(pathStr, path))
                {
                    m_content->activeElement(m_content->findNode(path));
                }
            }

            // restore selection
            {
                m_selection.clear();

                uint32_t numSelectedNodes = m_fileConfigPath.get<uint32_t>("NumSelectedElements");
                for (uint32_t i = 0; i < numSelectedNodes; ++i)
                {
                    if (auto pathStr = m_fileConfigPath.get<base::StringBuf>(base::TempString("SelectedElement{}", i)))
                    {
                        scene::NodePath path;
                        if (scene::NodePath::Parse(pathStr, path))
                        {
                            if (auto node = base::rtti_cast<ContentNode>(m_content->findNode(path)))
                            {
                                m_selection.insert(node);
                            }
                        }
                    }
                }

                if (m_activeEditMode)
                    m_activeEditMode->bind(m_selection.keys());
            }

            // post initial configurations
            postInternalConfiguration(m_selectionSettings);
            postInternalConfiguration(m_gridSettings);
            postInternalConfiguration(m_gizmoSettings);
        }

        void SceneEditorTab::onSaveConfiguration()
        {
            m_gizmoSettings.save(m_fileConfigPath);
            m_gridSettings.save(m_fileConfigPath);
            m_selectionSettings.save(m_fileConfigPath);

            // store active element
            {
                base::StringBuf activePath;
                if (auto elem = m_content->activeElement())
                    activePath = base::TempString("{}", elem->path());
                m_fileConfigPath.set("ActiveElement", activePath);
            }

            // store list of selected nodes
            {
                base::Array<base::StringBuf> selectedNodes;

                for (auto& obj : m_selection.keys())
                {
                    auto path = base::StringBuf(base::TempString("{}", obj->path()));
                    if (path)
                        selectedNodes.pushBack(path);
                }

                m_fileConfigPath.set<uint32_t>("NumSelectedElements", selectedNodes.size());
                for (uint32_t i = 0; i < selectedNodes.size(); ++i)
                {
                    m_fileConfigPath.set(base::TempString("SelectedElement{}", i), selectedNodes[i]);
                }
            }
        }

        void SceneEditorTab::gizmoSettings(const SceneGizmoSettings& settings)
        {
            m_gizmoSettings = settings;
            postInternalConfiguration(settings);
        }

        void SceneEditorTab::gridSettings(const SceneGridSettings& settings)
        {
            m_gridSettings = settings;
            postInternalConfiguration(settings);
        }

        void SceneEditorTab::selectionSettings(const SceneSelectionSettings& settings)
        {
            m_selectionSettings = settings;
            postInternalConfiguration(settings);
        }

        bool SceneEditorTab::handleExternalCloseRequest()
        {
            // collect modified stuff
            base::InplaceArray<ContentElementPtr, 64> dirtyContent;
            m_content->root()->collectModifiedContent(dirtyContent);

            // if the file is modified we need to make sure we can close the editor
            if (!dirtyContent.empty())
            {
                // show the important question
                auto ret = ui::ShowMessageBox(sharedFromThis(),
                    ui::MessageBoxSetup().message(base::TempString("Scene content is changed, what should happen to the modified content?"))
                    .warn().yes().no().cancel().defaultCancel().title("Close editor"));

                // we want to go back to the editor
                if (ret == ui::MessageButton::Cancel)
                    return false;

                // we want to discard changes
                else if (ret == ui::MessageButton::No)
                {
                    // NOTE: discarding changes may fail
                    /*if (!file->discardChanges(sharedFromThis()))
                        return false;*/
                }

                // save the changes
                else if (ret == ui::MessageButton::Yes)
                {
                    // NOTE: saving may fail
                    ContentStructure::SaveStats stats;
                    m_content->saveContent(stats);

                    // failed save ?
                    if (!stats.m_failedLayers.empty())
                    {
                        base::StringBuilder msg;
                        msg << "Failed to save content:\n";
                        for (auto& failedFile : stats.m_failedLayers)
                            msg << failedFile->filePath() << "\n";

                        ui::PostGeneralMessageWindow(sharedFromThis(), ui::MessageType::Error, msg.toString());

                        auto ret = ui::ShowMessageBox(sharedFromThis(),
                            ui::MessageBoxSetup().message(base::TempString("Failed to save changed to {} files. Close without changes?", stats.m_failedLayers.size()))
                            .warn().yes().no().defaultNo().title("Close editor"));

                        return (ret == ui::MessageButton::Yes);
                    }
                }
            }

            // we can close now
            return true;

        }

        base::StringBuf SceneEditorTab::title() const
        {
            return base::TempString("Boomer Scene Editor - {}", tabCaption());
        }

        base::StringBuf SceneEditorTab::tabCaption() const
        {
            if (m_content->source() == ContentStructureSource::World)
                return base::TempString("World {}", m_content->rootFile()->parentDirectoryPtr()->name());
            else
                return base::TempString("Prefab {}", m_content->rootFile()->name().stringBeforeFirst("."));
        }

        ui::TooltipPtr SceneEditorTab::tabTooltip() const
        {
            return base::RefNew<ui::TooltipString>(m_content->rootFile()->depotPath());
        }

        bool SceneEditorTab::canCloseTab() const
        {
            // TODO: can't close when playing game
            return true;
        }

        bool SceneEditorTab::canDetach() const
        {
            return m_content->source() != ContentStructureSource::World;
        }

        static base::res::StaticResource<base::image::Image> resWorld("engine/ui/styles/icons/world.png");
        static base::res::StaticResource<base::image::Image> resCube("engine/ui/styles/icons/cube.png");

        base::image::ImagePtr SceneEditorTab::tabIcon() const
        {
            if (m_content->source() == ContentStructureSource::World)
                return resWorld.loadAndGet();
            else
                return resCube.loadAndGet();
        }

        bool SceneEditorTab::handleTabContextMenu(const ui::ElementArea& area, const ui::Position& absolutePosition)
        {
            return false;
        }

        bool SceneEditorTab::isEditingFile(const depot::ManagedFilePtr& file) const
        {
            if (file == m_content->rootFile())
                return true;

            // TODO: layers!

            return false;
        }

        void SceneEditorTab::showFile(const depot::ManagedFilePtr& file) const
        {
            // TODO: activate layer
        }

        void SceneEditorTab::initializeActions()
        {
            // bind generic file commands
            commandBindings().bindCommand("File.SaveAll") = [this](UI_EVENT) { cmdSave(); };
            commandBindings().bindFilter("File.SaveAll") = [this]() -> bool { return canSave(); };

            // bind generic editor commands
            commandBindings().bindFilter("Edit.Undo") = [this]() -> bool { return canUndo(); };
            commandBindings().bindFilter("Edit.Redo") = [this]() -> bool { return canRedo(); };
            commandBindings().bindCommand("Edit.Undo") = [this](UI_EVENT) { cmdUndo(); };
            commandBindings().bindCommand("Edit.Redo") = [this](UI_EVENT) { cmdRedo(); };

            // clipboard command
            commandBindings().bindFilter("Edit.Copy") = [this]() -> bool { return canCopy(); };
            commandBindings().bindFilter("Edit.Cut") = [this]() -> bool { return canCut(); };
            commandBindings().bindFilter("Edit.Paste") = [this]() -> bool { return canPaste(); };
            commandBindings().bindFilter("Edit.Delete") = [this]() -> bool { return canDelete(); };
            commandBindings().bindCommand("Edit.Copy") = [this](UI_EVENT) { cmdCopy(); };
            commandBindings().bindCommand("Edit.Cut") = [this](UI_EVENT) { cmdCut(); };
            commandBindings().bindCommand("Edit.Paste") = [this](UI_EVENT) { cmdPaste(); };
            commandBindings().bindCommand("Edit.Delete") = [this](UI_EVENT) { cmdDelete(); };

            // grid/gizmo actions
            commandBindings().bindCommand("Edit.ToggleTransparentSelections") = [this](UI_EVENT) { cmdToggleTransparentSelection(callingElement); };
            commandBindings().bindToggle("Edit.ToggleTransparentSelections") = [this]() { return selectionSettings().m_selectTransparent; };

            commandBindings().bindCommand("Edit.ToggleWholePrefabSelection") = [this](UI_EVENT) { cmdToggleWholePrefabSelection(callingElement); };
            commandBindings().bindToggle("Edit.ToggleWholePrefabSelection") = [this]() { return selectionSettings().m_selectWholePrefabs; };

            commandBindings().bindToggle("Gizmo.LockX") = [this]() -> bool { return !gizmoSettings().m_enableX; };
            commandBindings().bindToggle("Gizmo.LockY") = [this]() -> bool { return !gizmoSettings().m_enableY; };
            commandBindings().bindToggle("Gizmo.LockZ") = [this]() -> bool { return !gizmoSettings().m_enableZ; };
            commandBindings().bindCommand("Gizmo.LockX") = [this](UI_EVENT) { cmdToggleFilterX(callingElement); };
            commandBindings().bindCommand("Gizmo.LockY") = [this](UI_EVENT) { cmdToggleFilterY(callingElement); };
            commandBindings().bindCommand("Gizmo.LockZ") = [this](UI_EVENT) { cmdToggleFilterZ(callingElement); };

            commandBindings().bindCommand("Gizmo.TranslationGizmo") = [this](UI_EVENT) { cmdSelectTranslationGizmo(callingElement); };
            commandBindings().bindCommand("Gizmo.RotationGizmo") = [this](UI_EVENT) { cmdSelectRotationGizmo(callingElement); };
            commandBindings().bindCommand("Gizmo.ScaleGizmo") = [this](UI_EVENT) { cmdSelectScaleGizmo(callingElement); };
            commandBindings().bindCommand("Gizmo.CycleGizmo") = [this](UI_EVENT) { cmdCycleGizmo(callingElement); };

            commandBindings().bindToggle("Gizmo.TranslationGizmo") = [this]() -> bool { return gizmoSettings().m_mode == ui::gizmo::GizmoMode::Translation; };
            commandBindings().bindToggle("Gizmo.RotationGizmo") = [this]() -> bool { return gizmoSettings().m_mode == ui::gizmo::GizmoMode::Rotation; };
            commandBindings().bindToggle("Gizmo.ScaleGizmo") = [this]() -> bool { return gizmoSettings().m_mode == ui::gizmo::GizmoMode::Scale; };

            commandBindings().bindToggle("Grid.TogglePositionGrid") = [this]() -> bool { return gridSettings().m_positionGridEnabled; };
            commandBindings().bindToggle("Grid.ToggleRotationGrid") = [this]() -> bool { return gridSettings().m_rotationGridEnabled; };

            commandBindings().bindCommand("Grid.TogglePositionGrid") = [this](UI_EVENT) { cmdTogglePositionGrid(); };
            commandBindings().bindCommand("Grid.ToggleRotationGrid") = [this](UI_EVENT) { cmdToggleRotationGrid(); };
        }

        void SceneEditorTab::initializePanels()
        {
            // load the node palette
            if (auto paletteNotebook = findChildByName<ui::DockNotebook>("PaletteTab"))
            {
                m_palette.create();
                m_palette->loadTemplates();
                m_palette->fillPaletteNotebook(paletteNotebook);
            }

            // create the scene tree
            if (auto tree = findChildByName<ui::TreeView>("SceneTree"))
            {
                tree->bind(m_content);

                tree->OnItemActivated = [this](UI_MODEL_CALLBACK)
                {
                    if (auto elem = index.lock<IContentElement>())
                        treeFocusOnItem(elem);
                };

                tree->OnSelectionChanged = [tree, this](UI_CALLBACK)
                {
                    base::Array<ContentElementPtr> elements;
                    elements.reserve(tree->selection().size());

                    for (auto& index : tree->selection())
                        if (auto elem = index.lock<IContentElement>())
                            elements.pushBack(elem);

                    treeSelectionChanged(elements);
                };
            }
        }

        void SceneEditorTab::initializeGridTables()
        {
            m_positionGridSizes.pushBack(0.01f);
            m_positionGridSizes.pushBack(0.02f);
            m_positionGridSizes.pushBack(0.05f);
            m_positionGridSizes.pushBack(0.1f);
            m_positionGridSizes.pushBack(0.2f);
            m_positionGridSizes.pushBack(0.5f);
            m_positionGridSizes.pushBack(1.0f);
            m_positionGridSizes.pushBack(2.0f);
            m_positionGridSizes.pushBack(5.0f);
            m_positionGridSizes.pushBack(10.0f);

            m_rotationGridSizes.pushBack(1.00f);
            m_rotationGridSizes.pushBack(3.00f);
            m_rotationGridSizes.pushBack(5.00f);
            m_rotationGridSizes.pushBack(5.6125f);
            m_rotationGridSizes.pushBack(10.0f);
            m_rotationGridSizes.pushBack(11.25f);
            m_rotationGridSizes.pushBack(15.0f);
            m_rotationGridSizes.pushBack(22.5f);
            m_rotationGridSizes.pushBack(30.0f);
            m_rotationGridSizes.pushBack(45.0f);
            m_rotationGridSizes.pushBack(60.0f);
            m_rotationGridSizes.pushBack(90.0f);
        }

        static base::res::StaticResource<base::image::Image> resEditMode("engine/ui/styles/icons/table.png");

        void SceneEditorTab::initializeEditModes()
        {
            base::Array<base::ClassType> editModeClasses;
            RTTI::GetInstance().enumClasses(ISceneEditMode::GetStaticClass(), editModeClasses);
            TRACE_INFO("Found {} edit mode classes", editModeClasses.size());

            for (auto classPtr  : editModeClasses)
            {
                auto editMode = classPtr->createSharedPtr<ISceneEditMode>();
                if (editMode->initialize(this))
                {
                    TRACE_INFO("Edit mode '{}' initialized with content for '{}'", classPtr->name(), m_content->rootFile()->depotPath());
                    m_allEditModes.pushBack(editMode);

                    // HACK
                    if (classPtr->name().view().endsWith("_Objects"))
                        m_activeEditMode = editMode;
                }
                else
                {
                    TRACE_INFO("Edit mode '{}' disabled for '{}'", classPtr->name(), m_content->rootFile()->depotPath());
                }
            }

            if (!m_activeEditMode && !m_allEditModes.empty())
                m_activeEditMode = m_allEditModes.front();

            if (m_activeEditMode)
            {
                if (!m_activeEditMode->activate(selection()))
                {
                    TRACE_WARNING("Failed to initialize default edit mode");
                    m_activeEditMode = nullptr;
                }
            }

            // tabs
            m_editModePanel = base::RefNew<ui::DockPanel>("EditMode", false, false);
            m_editModePanel->icon(resEditMode.loadAndGet());

            if (auto tabs = findChildByName<ui::DockNotebook>("SideTab"))
                tabs->addPanel(m_editModePanel, 0, true);

            if (m_activeEditMode)
            {
                if (auto ui = m_activeEditMode->uI())
                {
                    m_editModePanel->removeAllChildren();
                    m_editModePanel->attachBuildChild(ui);
                    m_editModePanel->title(m_activeEditMode->name());
                    m_editModePanel->icon(m_activeEditMode->icon());
                }
            }

            // toolbar
            if (auto toolbar = findChildByName<ui::ToolBar>("MainToolbar"))
            {
                toolbar->addItem().separator();

                for (uint32_t i = 0; i < m_allEditModes.size(); i++)
                {
                    auto editMode = m_allEditModes[i].get();
                    auto actionName = base::StringBuf(base::TempString("Edit.SelectEditMode{}", i));

                    commandBindings().defineCommand(actionName, editMode->name(), ui::CommandKeyShortcut(), editMode->icon());
                    commandBindings().bindToggle(actionName) = [this, editMode]() -> bool { return m_activeEditMode.get() == editMode; };
                    commandBindings().bindCommand(actionName) = [this, i](UI_EVENT) { cmdActivateEditMode(i); };

                    toolbar->addItem().action(actionName.c_str());
                }

                toolbar->realize();
            }
        }

        void SceneEditorTab::initializePreviewPanel()
        {
            if (auto tabs = findChildByName<ui::DockNotebook>("CenterTab"))
            {
                auto previewTab = base::RefNew<SceneRenderingContainer>(this, m_fileConfigPath["preview"]);
                tabs->addPanel(previewTab);
            }
        }

        void SceneEditorTab::updateGridSetup()
        {
            if (auto positionGridSize = findChildByName<ui::ComboBox>("PositionGridSize"))
            {
                auto index = m_positionGridSizes.find(gridSettings().m_positionGridSize);
                positionGridSize->enable(gridSettings().m_positionGridEnabled);
                positionGridSize->selectOption(index);
            }

            if (auto rotationGridSize = findChildByName<ui::ComboBox>("RotationGridSize"))
            {
                auto index = m_rotationGridSizes.find(gridSettings().m_rotationGridSize);
                rotationGridSize->enable(gridSettings().m_rotationGridEnabled);
                rotationGridSize->selectOption(index);
            }
        }

        void SceneEditorTab::fillPositionGridSizes()
        {
            if (auto positionGridSize = findChildByName<ui::ComboBox>("PositionGridSize"))
            {
                positionGridSize->clearOptions();

                for (auto size : m_positionGridSizes)
                {
                    if (size < 1.0f)
                        positionGridSize->addOption(base::TempString("{} cm", Prec(size * 100.0f, 1)));
                    else
                        positionGridSize->addOption(base::TempString("{} m", Prec(size, 1)));
                }

                positionGridSize->OnChanged = [this](UI_CALLBACK)
                {
                    if (auto positionGridSize = findChildByName<ui::ComboBox>("PositionGridSize"))
                    {
                        auto index = positionGridSize->selectedOption();
                        if (index >= 0 && index <= m_positionGridSizes.lastValidIndex())
                        {
                            auto settings = gridSettings();
                            settings.m_positionGridSize = m_positionGridSizes[index];
                            gridSettings(settings);
                        }
                    }
                };
            }
        }

        void SceneEditorTab::fillRotationGridSizes()
        {
            if (auto rotationGridSize = findChildByName<ui::ComboBox>("RotationGridSize"))
            {
                rotationGridSize->clearOptions();

                for (auto size : m_rotationGridSizes)
                    rotationGridSize->addOption(base::TempString("{}", Prec(size, 1)));

                rotationGridSize->OnChanged = [this](UI_CALLBACK)
                {
                    if (auto rotationGridSize = findChildByName<ui::ComboBox>("RotationGridSize"))
                    {
                        auto index = rotationGridSize->selectedOption();
                        if (index >= 0 && index <= m_rotationGridSizes.lastValidIndex())
                        {
                            auto settings = gridSettings();
                            settings.m_rotationGridSize = m_rotationGridSizes[index];
                            gridSettings(settings);
                        }
                    }
                };
            }
        }

        void SceneEditorTab::fillAreaSelectionModes()
        {
            if (auto areaSelectionMode = findChildByName<ui::ComboBox>("AreaSelectionMode"))
            {
                areaSelectionMode->clearOptions();
                areaSelectionMode->addOption("Everything");
                areaSelectionMode->addOption("Exclude border");
                areaSelectionMode->addOption("Largest area");
                areaSelectionMode->addOption("Smallest area");
                areaSelectionMode->addOption("Closest");
                areaSelectionMode->selectOption((int)selectionSettings().m_areaMode);

                areaSelectionMode->OnChanged = [this](UI_CALLBACK)
                {
                    if (auto areaSelectionMode = findChildByName<ui::ComboBox>("AreaSelectionMode"))
                    {
                        auto settings = selectionSettings();
                        settings.m_areaMode = (ui::RenderingAreaSelectionMode)areaSelectionMode->selectedOption();
                        selectionSettings(settings);
                    }
                };
            }
        }

        void SceneEditorTab::fillPointSelectionModes()
        {
            if (auto pointSelectionMode = findChildByName<ui::ComboBox>("PointSelectionMode"))
            {
                pointSelectionMode->clearOptions();
                pointSelectionMode->addOption("Exact");
                pointSelectionMode->addOption("Largest area");
                pointSelectionMode->addOption("Smallest area");
                pointSelectionMode->addOption("Closest");
                pointSelectionMode->selectOption((int)selectionSettings().m_pointMode);

                pointSelectionMode->OnChanged = [this](UI_CALLBACK)
                {
                    if (auto pointSelectionMode = findChildByName<ui::ComboBox>("PointSelectionMode"))
                    {
                        auto settings = selectionSettings();
                        settings.m_pointMode = (ui::RenderingPointSelectionMode)pointSelectionMode->selectedOption();
                        selectionSettings(settings);
                    }
                };
            }
        }

        void SceneEditorTab::fillGizmoSpaceModes()
        {
            if (auto gizmoSpace = findChildByName<ui::ComboBox>("GizmoSpace"))
            {
                gizmoSpace->addOption("World");
                gizmoSpace->addOption("Local");
                gizmoSpace->addOption("Parent");
                gizmoSpace->addOption("View");
                gizmoSpace->selectOption((int)gizmoSettings().m_space);

                gizmoSpace->OnChanged = [this](UI_CALLBACK)
                {
                    if (auto gizmoSpace = findChildByName<ui::ComboBox>("GizmoSpace"))
                    {
                        auto settings = gizmoSettings();
                        settings.m_space = (ui::gizmo::GizmoSpace) gizmoSpace->selectedOption();
                        gizmoSettings(settings);
                    }
                };
            }
        }

        void SceneEditorTab::fillGizmoTargetModes()
        {
            if (auto gizmoTarget = findChildByName<ui::ComboBox>("GizmoTarget"))
            {
                gizmoTarget->addOption("Hierarchy");
                gizmoTarget->addOption("Selection");
                gizmoTarget->selectOption((int)gizmoSettings().m_target);

                gizmoTarget->OnChanged = [this](UI_CALLBACK)
                {
                    if (auto gizmoTarget = findChildByName<ui::ComboBox>("GizmoTarget"))
                    {
                        auto settings = gizmoSettings();
                        settings.m_target = (ui::gizmo::GizmoTarget) gizmoTarget->selectedOption();
                        gizmoSettings(settings);
                    }
                };
            }
        }

        //--

        void SceneEditorTab::clearSelection()
        {
            m_selection.clear();

            if (m_activeEditMode)
                m_activeEditMode->bind(m_selection.keys());
        }

        void SceneEditorTab::changeSelection(const ContentNodePtr& node, bool selected)
        {
            if (selected && !m_selection.contains(node))
            {
                m_selection.insert(node);

                if (m_activeEditMode)
                    m_activeEditMode->bind(m_selection.keys());
            }
            else if (!selected && m_selection.contains(node))
            {
                m_selection.remove(node);

                if (m_activeEditMode)
                    m_activeEditMode->bind(m_selection.keys());
            }
        }

        void SceneEditorTab::changeSelection(const base::Array<ContentNodePtr>& newSelection)
        {
            m_selection.clear();
            
            for (auto& ptr : newSelection)
                m_selection.insert(ptr);

            if (m_activeEditMode)
                m_activeEditMode->bind(m_selection.keys());
        }

        bool SceneEditorTab::isSelected(const ContentNodePtr& node) const
        {
            return m_selection.contains(node);
        }

        void SceneEditorTab::attachRenderingPanel(SceneRenderingPanel* panel)
        {
            ASSERT(!m_renderingPanels.contains(panel));
            m_renderingPanels.pushBack(panel);
        }

        void SceneEditorTab::detachRenderingPanel(SceneRenderingPanel* panel)
        {
            ASSERT(m_renderingPanels.contains(panel));
            m_renderingPanels.remove(panel);
        }

        //--

        void SceneEditorTab::treeFocusOnItem(const ContentElementPtr& elem)
        {

        }

        void SceneEditorTab::treeSelectionChanged(const base::Array<ContentElementPtr>& elements)
        {
            base::Array<ContentNodePtr> nodes;
            nodes.reserve(elements.size());

            for (auto& elem : elements)
                if (auto node = base::rtti_cast<ContentNode>(elem))
                    nodes.pushBack(node);

            changeSelection(nodes);
        }

        void SceneEditorTab::viewportSelectionChanged(SceneRenderingPanel* panel, bool ctrl, bool shift, const base::Array<rendering::scene::Selectable>& elements)
        {
            if (m_activeEditMode)
                m_activeEditMode->handleViewportSelection(panel, ctrl, shift, base::Rect(), elements);
        }

        ui::InputActionPtr SceneEditorTab::viewportMouseClickEvent(SceneRenderingPanel* panel, const ui::ElementArea& area, const base::input::MouseClickEvent& evt)
        {
            if (m_activeEditMode)
            {
                if (auto action = m_activeEditMode->handleMouseClick(panel, evt))
                    return action;
            }

            return nullptr;
        }

        bool SceneEditorTab::viewportMouseMoveEvent(SceneRenderingPanel* panel, const base::input::MouseMovementEvent& evt)
        {
            if (m_activeEditMode)
            {
                if (m_activeEditMode->handleMouseMovement(panel, evt))
                    return true;
            }

            return false;
        }

        bool SceneEditorTab::viewportMouseWheelEvent(SceneRenderingPanel* panel, const base::input::MouseMovementEvent& evt, float delta)
        {
            if (m_activeEditMode)
            {
                if (m_activeEditMode->handleMouseWheel(panel, evt, delta))
                    return true;
            }

            return false;
        }

        bool SceneEditorTab::viewportContextMenu(SceneRenderingPanel* panel, const ui::ElementArea& area, const ui::Position& absolutePosition, const base::Array<rendering::scene::Selectable>& objectUnderCursor, const base::Vector3* knownWorldPositionUnderCursor)
        {
            if (m_activeEditMode)
            {
                auto localPosition = absolutePosition - area.absolutePosition();
                if (m_activeEditMode->handleContextMenu(panel, absolutePosition, objectUnderCursor, knownWorldPositionUnderCursor))
                    return true;
            }

            return false;
        }

        bool SceneEditorTab::viewportKeyEvent(SceneRenderingPanel* panel, const base::input::KeyEvent& evt)
        {
            if (m_activeEditMode)
            {
                if (m_activeEditMode->handleKeyEvent(panel, evt))
                    return true;
            }

            return false;
        }

        ui::DragDropHandlerPtr SceneEditorTab::viewportDragAndDrop(SceneRenderingPanel* panel, const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
        {
            if (m_activeEditMode)
            {
                if (auto handler = m_activeEditMode->handleDragDrop(panel, data, entryPosition))
                    return handler;
            }

            return nullptr;
        }

        void SceneEditorTab::viewportRenderDebugFragments(SceneRenderingPanel* panel, rendering::scene::FrameInfo& frame)
        {
            if (m_tempObjects)
                m_tempObjects->render(frame);

            if (m_content)
                m_content->render(frame);

            if (m_activeEditMode)
                m_activeEditMode->handleRendering(panel, frame);
        }

        //--

        void SceneEditorTab::cmdSave()
        {
            ContentStructure::SaveStats stats;
            m_content->saveContent(stats);

            base::StringBuilder msg;
            msg.appendf("Saved {} layer(s) ({} added, {} removed)\n", stats.m_numSavedLayers, stats.m_numLayersAdded, stats.m_numLayersRemoved);

            if (stats.m_numGroupsRemoved)
                msg.appendf("Added {} group(s)\n", stats.m_numGroupsRemoved);
            if (stats.m_numGroupsRemoved)
                msg.appendf("Removed {} group(s)\n", stats.m_numGroupsRemoved);

            msg.appendf("Saved {} node(s), {} total node(s)", stats.m_numSavedNodes, stats.m_numTotalNodes);

            if (!stats.m_failedLayers.empty())
            {
                msg << "Failed to save content:\n";
                for (auto& failedFile : stats.m_failedLayers)
                    msg << failedFile->filePath() << "\n";
            }

            auto messageType = stats.m_failedLayers.empty() ? ui::MessageType::Info : ui::MessageType::Error;
            ui::PostGeneralMessageWindow(sharedFromThis(), messageType, msg.toString());
        }

        void SceneEditorTab::cmdUndo()
        {

        }

        void SceneEditorTab::cmdRedo()
        {

        }

        bool SceneEditorTab::canUndo() const
        {
            return false;
        }

        bool SceneEditorTab::canRedo() const
        {
            return false;
        }

        bool SceneEditorTab::canSave() const
        {
            return true;
        }

        void SceneEditorTab::cmdCopy()
        {
            if (m_activeEditMode)
                m_activeEditMode->handleCopy();
        }

        void SceneEditorTab::cmdCut()
        {
            if (m_activeEditMode)
                m_activeEditMode->handleCut();
        }

        void SceneEditorTab::cmdPaste()
        {
            if (m_activeEditMode)
                m_activeEditMode->handlePaste();
        }

        void SceneEditorTab::cmdDelete()
        {
            if (m_activeEditMode)
                m_activeEditMode->handleDelete();
        }

        bool SceneEditorTab::canCopy() const
        {
            return m_activeEditMode ? m_activeEditMode->canCopy() : false;
        }

        bool SceneEditorTab::canCut() const
        {
            return m_activeEditMode ? m_activeEditMode->canCut() : false;
        }

        bool SceneEditorTab::canPaste() const
        {
            return m_activeEditMode ? m_activeEditMode->canPaste() : false;
        }

        bool SceneEditorTab::canDelete() const
        {
            return m_activeEditMode ? m_activeEditMode->canDelete() : false;
        }

        void SceneEditorTab::cmdToggleTransparentSelection(const ui::ElementPtr& ptr)
        {
            auto settings = selectionSettings();
            settings.m_selectTransparent = !settings.m_selectTransparent;
            selectionSettings(settings);
        }

        void SceneEditorTab::cmdToggleWholePrefabSelection(const ui::ElementPtr& ptr)
        {
            auto settings = selectionSettings();
            settings.m_selectWholePrefabs = !settings.m_selectWholePrefabs;
            selectionSettings(settings);
        }

        void SceneEditorTab::cmdSelectTranslationGizmo(const ui::ElementPtr& owner)
        {
            auto settings = gizmoSettings();
            if (settings.m_mode != ui::gizmo::GizmoMode::Translation)
            {
                settings.m_mode = ui::gizmo::GizmoMode::Translation;
                gizmoSettings(settings);
            }
        }

        void SceneEditorTab::cmdSelectRotationGizmo(const ui::ElementPtr& owner)
        {
            auto settings = gizmoSettings();
            if (settings.m_mode != ui::gizmo::GizmoMode::Rotation)
            {
                settings.m_mode = ui::gizmo::GizmoMode::Rotation;
                gizmoSettings(settings);
            }
        }

        void SceneEditorTab::cmdSelectScaleGizmo(const ui::ElementPtr& owner)
        {
            auto settings = gizmoSettings();
            if (settings.m_mode != ui::gizmo::GizmoMode::Scale)
            {
                settings.m_mode = ui::gizmo::GizmoMode::Scale;
                gizmoSettings(settings);
            }
        }

        void SceneEditorTab::cmdCycleGizmo(const ui::ElementPtr& owner)
        {
            auto settings = gizmoSettings();
            if (settings.m_mode == ui::gizmo::GizmoMode::Scale)
                settings.m_mode = ui::gizmo::GizmoMode::Translation;
            else if (settings.m_mode == ui::gizmo::GizmoMode::Translation)
                settings.m_mode = ui::gizmo::GizmoMode::Rotation;
            else if (settings.m_mode == ui::gizmo::GizmoMode::Rotation)
                settings.m_mode = ui::gizmo::GizmoMode::Scale;

            gizmoSettings(settings);
        }

        void SceneEditorTab::cmdSelectGizmoSpace(const ui::ElementPtr& owner)
        {
        }

        void SceneEditorTab::cmdToggleFilterX(const ui::ElementPtr& owner)
        {
            auto settings = gizmoSettings();
            settings.m_enableX = !settings.m_enableX;
            gizmoSettings(settings);
        }

        void SceneEditorTab::cmdToggleFilterY(const ui::ElementPtr& owner)
        {
            auto settings = gizmoSettings();
            settings.m_enableY = !settings.m_enableY;
            gizmoSettings(settings);
        }

        void SceneEditorTab::cmdToggleFilterZ(const ui::ElementPtr& owner)
        {
            auto settings = gizmoSettings();
            settings.m_enableZ = !settings.m_enableZ;
            gizmoSettings(settings);
        }

        void SceneEditorTab::cmdTogglePositionGrid()
        {
            auto settings = gridSettings();
            settings.m_positionGridEnabled = !settings.m_positionGridEnabled;
            gridSettings(settings);

            updateGridSetup();
        }

        void SceneEditorTab::cmdToggleRotationGrid()
        {
            auto settings = gridSettings();
            settings.m_rotationGridEnabled = !settings.m_rotationGridEnabled;
            gridSettings(settings);

            updateGridSetup();
        }

        void SceneEditorTab::cmdActivateEditMode(int index)
        {
            auto mode = m_allEditModes[index];
            if (mode != m_activeEditMode)
            {
                if (m_activeEditMode)
                    m_activeEditMode->deactive();

                if (mode->activate(selection()))
                    m_activeEditMode = mode;
                else
                    m_activeEditMode->activate(selection());

                if (m_activeEditMode)
                {
                    if (auto ui = m_activeEditMode->uI())
                    {
                        m_editModePanel->removeAllChildren();
                        m_editModePanel->attachBuildChild(ui);
                        m_editModePanel->title(m_activeEditMode->name());
                        m_editModePanel->icon(m_activeEditMode->icon());
                    }
                }
            }
        }

        //--

        /// helper class for opening scene editor
        class SceneEditorOpener : public resources::IEditorOpener
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneEditorOpener, resources::IEditorOpener);

        public:
            virtual uint32_t canHandleFileFormat(const depot::ManagedFileFormat& fileFormat) const override final
            {
                return fileFormat.extension() == "v4world" || fileFormat.extension() == "v4prefab";
            }

            virtual bool openResourceEditor(const resources::BrowserNotebookPtr& tabs, const depot::ManagedFilePtr& file) const override final
            {
                auto tab = base::RefNew<SceneEditorTab>(file);
                tabs->attachTab(tab, true);
                return true;
            }
        };

        RTTI_BEGIN_TYPE_CLASS(SceneEditorOpener);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_STRUCT(SceneGridSettings);
        RTTI_END_TYPE();

        SceneGridSettings::SceneGridSettings()
        {}

        void SceneGridSettings::save(ConfigPath& config) const
        {
            config.set("GridPositionGridEnabled", m_positionGridEnabled);
            config.set("GridRotationGridEnabled", m_rotationGridEnabled);
            config.set("GridPositionGridSize", m_positionGridSize);
            config.set("GridRotationGridSize", m_rotationGridSize);
            config.set("GridFeatureSnappingEnabled", m_featureSnappingEnabled);
            config.set("GridRotationSnappingEnabled", m_rotationSnappingEnabled);
            config.set("GridSnappingDistance", m_snappingDistance);
        }

        void SceneGridSettings::load(const ConfigPath& config)
        {
            m_positionGridEnabled = config.get("GridPositionGridEnabled", m_positionGridEnabled);
            m_rotationGridEnabled = config.get("GridRotationGridEnabled", m_rotationGridEnabled);
            m_positionGridSize = config.get("GridPositionGridSize", m_positionGridSize);
            m_rotationGridSize = config.get("GridRotationGridSize", m_rotationGridSize);
            m_featureSnappingEnabled = config.get("GridFeatureSnappingEnabled", m_featureSnappingEnabled);
            m_rotationSnappingEnabled = config.get("GridRotationSnappingEnabled", m_rotationSnappingEnabled);
            m_snappingDistance = config.get("GridSnappingDistance", m_snappingDistance);
        }

        //---

        RTTI_BEGIN_TYPE_STRUCT(SceneGizmoSettings);
        RTTI_END_TYPE();

        SceneGizmoSettings::SceneGizmoSettings()
        {}

        void SceneGizmoSettings::save(ConfigPath& config) const
        {
            config.set("GizmoMode", m_mode);
            config.set("GizmoSpace", m_space);
            config.set("GizmoTarget", m_target);
            config.set("GizmoEnableX", m_enableX);
            config.set("GizmoEnableY", m_enableY);
            config.set("GizmoEnableZ", m_enableZ);
        }

        void SceneGizmoSettings::load(const ConfigPath& config)
        {
            m_mode = config.get("GizmoMode", m_mode);
            m_space = config.get("GizmoSpace", m_space);
            m_target = config.get("GizmoTarget", m_target);
            m_enableX = config.get("GizmoEnableX", m_enableX);
            m_enableY = config.get("GizmoEnableY", m_enableY);
            m_enableZ = config.get("GizmoEnableZ", m_enableZ);
        }

        //---

        RTTI_BEGIN_TYPE_STRUCT(SceneSelectionSettings);
            RTTI_PROPERTY(m_areaMode);
            RTTI_PROPERTY(m_pointMode);
            RTTI_PROPERTY(m_selectTransparent);
            RTTI_PROPERTY(m_selectWholePrefabs);
        RTTI_END_TYPE();

        SceneSelectionSettings::SceneSelectionSettings()
            : m_areaMode(ui::RenderingAreaSelectionMode::ExcludeBorder)
            , m_pointMode(ui::RenderingPointSelectionMode::Closest)
            , m_selectTransparent(false)
            , m_selectWholePrefabs(true)
        {}

        void SceneSelectionSettings::save(ConfigPath& config) const
        {
            config.set("SelectionAreaMode", m_areaMode);
            config.set("SelectionPointMode", m_pointMode);
            config.set("SelectionEnableTransparent", m_selectTransparent);
            config.set("SelectionEnableWholePrefabSelection", m_selectWholePrefabs);
        }

        void SceneSelectionSettings::load(const ConfigPath& config)
        {
            m_areaMode = config.get("SelectionAreaMode", m_areaMode);
            m_pointMode = config.get("SelectionPointMode", m_pointMode);
            m_selectTransparent = config.get("SelectionEnableTransparent", m_selectTransparent);
            m_selectWholePrefabs = config.get("SelectionEnableWholePrefabSelection", m_selectWholePrefabs);
        }

        //--

    } // world
} // ed
