/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure\presets #]
***/

#include "build.h"
#include "sceneEntityStructurePresets.h"
#include "sceneEditorStructureGroup.h"
#include "sceneEditorStructureLayer.h"
#include "sceneEditorStructureNode.h"
#include "sceneEditorStructure.h"

#include "base/image/include/image.h"
#include "scene/common/include/sceneNodePath.h"
#include "ui/widgets/include/uiWindowMessage.h"
#include "ui/widgets/include/uiComboBox.h"
#include "ui/widgets/include/uiInputBox.h"
#include "ui/widgets/include/uiMessageBox.h"
#include "ui/widgets/include/uiToolBar.h"
#include "ui/models/include/uiTreeView.h"

namespace ed
{
    namespace world
    {

        //---

        static void HashSetToStringArray(const base::HashSet<scene::NodePath>& paths, base::Array<base::StringBuf>& outPaths)
        {
            outPaths.reserve(paths.size());
            outPaths.clear();
            //for (auto& path : paths.keys())
              //  outPaths.pushBack(path.path());
        }

        static void StringArrayToHashSet(const base::Array<base::StringBuf>& paths, base::HashSet<scene::NodePath>& outPaths)
        {
            outPaths.reserve(paths.size());
            outPaths.clear();
            for (auto& path : paths)
            {
                scene::NodePath parsedPath;
                if (scene::NodePath::Parse(path, parsedPath))
                    outPaths.insert(parsedPath);
            }
        }

        SceneStructurePreset::SceneStructurePreset()
        {}

        void SceneStructurePreset::save(ConfigPath& section) const
        {
            base::Array<base::StringBuf> values;

            {
                HashSetToStringArray(m_expandedElements, values);
                section.set("Expanded", values);
            }

            {
                HashSetToStringArray(m_hiddenElements, values);
                section.set("Hidden", values);
            }
        }

        bool SceneStructurePreset::load(const ConfigPath& section)
        {
            {
                auto values = section.get<base::Array<base::StringBuf>>("Expanded");
                StringArrayToHashSet(values, m_expandedElements);
            }

            {
                auto values = section.get<base::Array<base::StringBuf>>("Hidden");
                StringArrayToHashSet(values, m_hiddenElements);
            }

            return true;
        }

        //--

        SceneStructurePresetCollection::SceneStructurePresetCollection(const ConfigPath& configName)
            : m_config(configName)
        {
            loadPresets();
        }

        SceneStructurePresetPtr SceneStructurePresetCollection::findPreset(const base::StringBuf& name) const
        {
            SceneStructurePresetPtr ret;
            if (m_presetMap.find(name, ret))
                return ret;
            return nullptr;         
        }

        void SceneStructurePresetCollection::loadPresets()
        {
            // load the names of the defined preset
            auto presetNames = m_config.get<base::Array<base::StringBuf>>("PresetNames");
            for (auto& presetName : presetNames)
            {
                auto presetSection = m_config[presetName.c_str()];

                auto presetData = base::CreateSharedPtr<SceneStructurePreset>();
                if (presetData->load(presetSection))
                {
                    m_presetMap[presetName] = presetData;
                }
            }

            // if there's no default preset, create it
            if (!m_presetMap.contains("Default"))
            {
                auto defaultPreset = base::CreateSharedPtr<SceneStructurePreset>();
                m_presetMap["Default"] = defaultPreset;
            }
        }

        void SceneStructurePresetCollection::addPreset(const base::StringBuf& name, const SceneStructurePresetPtr& preset)
        {
            ASSERT(!name.empty());
            ASSERT(preset);
            m_presetMap[name] = preset;
            flushPresetNames();
            flushPresets();
        }

        void SceneStructurePresetCollection::removePreset(const base::StringBuf& name)
        {
            if (name != "Default")
            {
                ASSERT(!name.empty());
                if (m_presetMap.remove(name))
                {
                    flushPresetNames();
                }
            }
        }

        void SceneStructurePresetCollection::flushPresetNames()
        {
            m_config.set("PresetNames", m_presetMap.keys());
        }

        void SceneStructurePresetCollection::flushPresets()
        {
            for (uint32_t i = 0; i < m_presetMap.size(); ++i)
            {
                auto& presetName = m_presetMap.keys()[i];
                auto& presetData = m_presetMap.values()[i];

                auto presetSection = m_config[presetName.c_str()];
                presetData->save(presetSection);

            }
        }

        //--

        static base::res::StaticResource<base::image::Image> resDisk("engine/ui/styles/icons/disk.png");
        static base::res::StaticResource<base::image::Image> resAdd("engine/ui/styles/icons/add.png");
        static base::res::StaticResource<base::image::Image> resDelete("engine/ui/styles/icons/delete.png");

        ScenePresetController::ScenePresetController(const ConfigPath& config, const base::RefPtr<ui::ToolBar>& toolbar, const base::RefPtr<ui::ComboBox>& presetList, const base::RefPtr<ui::TreeView>& sceneTree, ContentStructure& content)
            : m_presetList(presetList)
            , m_presetToolbar(toolbar)
            , m_sceneTree(sceneTree)
            , m_structure(content)
            , m_config(config)
        {
            defineCommand("Navigation.Save", "Save", ui::CommandKeyShortcut(), resDisk.loadAndGet(), "Save current state of the scene as preset");
            defineCommand("Navigation.AddPreset", "Add", ui::CommandKeyShortcut(), resAdd.loadAndGet(), "Add new preset");
            defineCommand("Navigation.RemovePreset", "Delete", ui::CommandKeyShortcut(), resDelete.loadAndGet(), "Delete current preset");

            bindCommand("Navigation.Save") = [this](UI_EVENT) { cmdSavePreset(); };
            bindCommand("Navigation.AddPreset") = [this](UI_EVENT) { cmdAddPreset(); };
            bindCommand("Navigation.RemovePreset") = [this](UI_EVENT) { cmdRemovePreset(); };

            bindFilter("Navigation.Save") = [this]() -> bool { return true; };
            bindFilter("Navigation.AddPreset") = [this]() -> bool { return true; };
            bindFilter("Navigation.RemovePreset") = [this]() -> bool { return (activePreset() != "Default"); };
        
            m_presetList->OnChanged = [this](UI_CALLBACK)
            {
                writePresetConfig();
                applySelectedPreset();
            };

            m_presetToolbar->commandBindings().attachDynamicCommandBindings(this);
            m_presetToolbar->realize();

            m_collection = base::CreateUniquePtr<SceneStructurePresetCollection>(config);

            auto autoPreset = config.get<base::StringBuf>("ActivePreset");
            refreshPresetList(autoPreset);
        }

        ScenePresetController::~ScenePresetController()
        {
            m_presetToolbar->commandBindings().detachDynamicCommandBindings(this);
            m_presetToolbar->realize();
        }

        void ScenePresetController::captureState(SceneStructurePresetPtr& outCapturedState)
        {
            // reset state
            outCapturedState = base::CreateSharedPtr<SceneStructurePreset>();
            outCapturedState->m_expandedElements.clear();
            outCapturedState->m_hiddenElements.clear();

            // walk the groups, collecting the loaded and hidden states
            if (auto rootGroup = base::rtti_cast<ContentGroup>(m_structure.root()))
            {
                captureGroup(rootGroup, outCapturedState);
                captureExpandedNodes(outCapturedState);
            }
        }

        void ScenePresetController::applyState(const SceneStructurePresetPtr& capturedState)
        {
            // expand tree nodes
            applyExpandedNodes(m_structure.root().get(), capturedState);

            // apply groups states
            auto rootGroup = base::rtti_cast<ContentGroup>(m_structure.root());
            if (rootGroup)
                applyGroup(rootGroup, capturedState);
        }

        void ScenePresetController::applyLayer(const ContentLayerPtr& layerPtr, const SceneStructurePresetPtr& capturedState) const
        {
            // is this layer hidden ?
            {
                auto isHiddenInPreset = capturedState->m_hiddenElements.contains(layerPtr->path());
                layerPtr->toggleLocalVisibility(!isHiddenInPreset);
            }

            // check the nodes visibility state as well
            for (auto& nodePtr : layerPtr->nodes())
            {
                auto isHiddenInPreset = capturedState->m_hiddenElements.contains(nodePtr->path());
                nodePtr->toggleLocalVisibility(!isHiddenInPreset);
            }
        }

        void ScenePresetController::applyGroup(const ContentGroupPtr& groupPtr, const SceneStructurePresetPtr& capturedState) const
        {
            // is this group hidden ?
            {
                auto isHiddenInPreset = capturedState->m_hiddenElements.contains(groupPtr->path());
                groupPtr->toggleLocalVisibility(!isHiddenInPreset);
            }

            // apply state to layers
            for (auto& layer : groupPtr->layers())
                applyLayer(layer, capturedState);

            // check other groups
            for (auto& subGroup : groupPtr->groups())
                applyGroup(subGroup, capturedState);
        }

        void ScenePresetController::applyExpandedNodes(const IContentElement* elem, const SceneStructurePresetPtr& capturedState) const
        {
            if (capturedState->m_expandedElements.contains(elem->path()))
                m_sceneTree->expandItem(elem->modelIndex());

            for (auto child  : elem->children())
                applyExpandedNodes(child, capturedState);
        }

        void ScenePresetController::captureExpandedNodes(SceneStructurePresetPtr& outCapturedState) const
        {
            base::InplaceArray<ui::ModelIndex, 256> expandedIndices;
            m_sceneTree->collectExpandedItems(expandedIndices);

            for (auto& id : expandedIndices)
                if (auto element  = id.unsafe<IContentElement>())
                    outCapturedState->m_expandedElements.insert(element->path());
        }

        void ScenePresetController::captureLayer(const ContentLayerPtr& layerPtr, SceneStructurePresetPtr& outCapturedState) const
        {
            // is this group hidden ?
            if (!layerPtr->localVisibility())
                outCapturedState->m_hiddenElements.insert(layerPtr->path());

            // process nodes
            for (auto& nodePtr : layerPtr->nodes())
            {
                if (!nodePtr->localVisibility())
                    outCapturedState->m_hiddenElements.insert(nodePtr->path());
            }
        }

        void ScenePresetController::captureGroup(const ContentGroupPtr& groupPtr, SceneStructurePresetPtr& outCapturedState) const
        {
            // is this group hidden ?
            if (!groupPtr->localVisibility())
                outCapturedState->m_hiddenElements.insert(groupPtr->path());

            // is this group loaded locally
            bool hasLocalLoadedLayers = false;
            for (auto& layer : groupPtr->layers())
                captureLayer(layer, outCapturedState);

            // check other groups
            for (auto& subGroup : groupPtr->groups())
                captureGroup(subGroup, outCapturedState);
        }

        void ScenePresetController::writePresetConfig()
        {
            auto activePresetName = activePreset();
            m_config.set<base::StringBuf>("ActivePreset", activePresetName);
        }

        void ScenePresetController::applySelectedPreset()
        {
            auto presetName = activePreset();
            if (!presetName.empty())
            {
                if (auto preset = m_collection->findPreset(presetName))
                    applyState(preset);
            }
        }

        base::StringBuf ScenePresetController::activePreset() const
        {
            return m_presetList->selectedOptionText();
        }

        void ScenePresetController::refreshPresetList(const base::StringBuf& presetToSelect)
        {
            m_presetList->clearOptions();

            auto presetNames = m_collection->presetNames();
            std::sort(presetNames.begin(), presetNames.end());

            for (auto& name : presetNames)
                m_presetList->addOption(name);

            if (!presetToSelect.empty())
                m_presetList->selectOption(presetToSelect);
        }

        void ScenePresetController::cmdAddPreset()
        {
            SceneStructurePresetPtr statePtr;
            captureState(statePtr);

            auto presetName = activePreset();
            if (ui::ShowInputBox(m_presetList, ui::InputBoxSetup().title("Create preset").message("Enter preset name:"), presetName) && !presetName.empty())
            {
                // set preset into the list
                m_collection->addPreset(presetName, statePtr);

                // refresh preset list
                writePresetConfig();
                refreshPresetList(presetName);

                // select the new preset
                m_presetList->selectOption(presetName);
            }
        }

        void ScenePresetController::cmdRemovePreset()
        {
            auto presetName = activePreset();
            if (presetName != "Default")
            {
                m_collection->removePreset(presetName);
                refreshPresetList("");
            }
        }

        void ScenePresetController::cmdSavePreset()
        {
            SceneStructurePresetPtr statePtr;
            captureState(statePtr);

            auto currentPresetName = activePreset();
            if (!currentPresetName.empty())
                m_collection->addPreset(currentPresetName, statePtr);
        }

        //--

    } // mesh
} // ed