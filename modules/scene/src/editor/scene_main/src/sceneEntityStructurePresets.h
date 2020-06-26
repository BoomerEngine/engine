/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: structure\presets #]
***/

#pragma once

#include "editor/asset_browser/include/editorConfig.h"
#include "base/containers/include/hashSet.h"
#include "ui/toolkit/include/uiCommandBindings.h"

namespace ed
{
    namespace world
    {
        //--

        /// preset for the scene structure
        class SceneStructurePreset : public base::NoCopy
        {
        public:
            base::HashSet<scene::NodePath> m_expandedElements; // groups and layers
            base::HashSet<scene::NodePath> m_hiddenElements; // elements explicitly hidden

            SceneStructurePreset();

            // store into the config section
            void save(ConfigPath& section) const;

            // load from the config section
            bool load(const ConfigPath& section);
        };

        //--

        typedef base::RefPtr<SceneStructurePreset> SceneStructurePresetPtr;

        //--

        /// collection of scene presets
        class SceneStructurePresetCollection : public base::NoCopy
        {
        public:
            SceneStructurePresetCollection(const ConfigPath& cofigPath);

            /// get preset names
            INLINE const base::Array<base::StringBuf>& presetNames() const { return m_presetMap.keys(); }

            /// find preset
            SceneStructurePresetPtr findPreset(const base::StringBuf& name) const;

            /// add preset, if preset already exists it's overridden
            void addPreset(const base::StringBuf& name, const SceneStructurePresetPtr& preset);

            /// remove preset
            void removePreset(const base::StringBuf& name);

        private:
            ConfigPath m_config;

            typedef base::HashMap<base::StringBuf, SceneStructurePresetPtr> TPresetMap;
            TPresetMap m_presetMap;

            void loadPresets();
            void flushPresetNames();
            void flushPresets();
        };

        //--
        
        /// controller for the scene preset system
        class ScenePresetController : public ui::CommandBindings
        {
        public:
            ScenePresetController(const ConfigPath& config, const base::RefPtr<ui::ToolBar>& toolbar, const base::RefPtr<ui::ComboBox>& presetList, const base::RefPtr<ui::TreeView>& sceneTree, ContentStructure& content);
            ~ScenePresetController();

            // get name of active preset
            base::StringBuf activePreset() const;

            // apply state of the selected preset to scene
            void applySelectedPreset();

        private:
            ConfigPath m_config;
            ContentStructure& m_structure;

            base::RefPtr<ui::ToolBar> m_presetToolbar;
            base::RefPtr<ui::ComboBox> m_presetList;
            base::RefPtr<ui::TreeView> m_sceneTree;

            base::UniquePtr<SceneStructurePresetCollection> m_collection;

            //--

            void refreshPresetList(const base::StringBuf& presetToSelect);
            void writePresetConfig();

            void cmdAddPreset();
            void cmdRemovePreset();
            void cmdSavePreset();

            //--

            void captureState(SceneStructurePresetPtr& outCapturedState);
            void applyState(const SceneStructurePresetPtr& capturedState);

            void captureGroup(const ContentGroupPtr& groupPtr, SceneStructurePresetPtr& outCapturedState) const;
            void captureLayer(const ContentLayerPtr& layerPtr, SceneStructurePresetPtr& outCapturedState) const;
            void captureExpandedNodes(SceneStructurePresetPtr& outCapturedState) const;

            typedef base::Array<ContentLayer*> LayerList;

            void applyExpandedNodes(const IContentElement* elem, const SceneStructurePresetPtr& capturedState) const;
            void applyGroup(const ContentGroupPtr& groupPtr, const SceneStructurePresetPtr& capturedState) const;
            void applyLayer(const ContentLayerPtr& groupPtr, const SceneStructurePresetPtr& capturedState) const;
        };

        //--

    } // world
} // ed