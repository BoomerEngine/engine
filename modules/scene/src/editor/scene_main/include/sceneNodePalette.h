/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tools #]
***/

#pragma once

#include "scene/common/include/sceneNodeTemplate.h"
#include "scene/common/include/scenePrefab.h"

#include "ui/toolkit/include/uiDragDrop.h"

namespace ed
{
    namespace world
    {

        ///--

        /// placeable node template
        struct EDITOR_SCENE_MAIN_API PlaceableNodeTemplateInfo
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(PlaceableNodeTemplateInfo);

        public:
            // placement
            base::StringID m_tab;
            base::StringID m_category;
            base::StringBuf m_name;

            // vis
            base::StringBuf m_description;
            base::res::Ref<base::image::Image> m_icon;

            // target node class to create
            base::res::Ref<scene::Prefab> m_prefab;

            //--

            PlaceableNodeTemplateInfo();
        };

        ///--

        /// a editor resource file with "palette" objects
        class EDITOR_SCENE_MAIN_API PlacableNodeTemplateListFile : public base::res::ITextResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(PlacableNodeTemplateListFile, base::res::ITextResource);

        public:
            PlacableNodeTemplateListFile();

            INLINE const base::Array<PlaceableNodeTemplateInfo>& templates() const { return m_templates; }

        protected:
            base::Array<PlaceableNodeTemplateInfo> m_templates;
        };

        ///--

        /// placable node template registry
        class EDITOR_SCENE_MAIN_API PlaceableNodeTemplateRegistry : public base::NoCopy
        {
        public:
            PlaceableNodeTemplateRegistry();
            ~PlaceableNodeTemplateRegistry();

            // get all node templates
            INLINE const base::Array<PlaceableNodeTemplateInfo>& templates() const { return m_templates; }

            //--

            // load all template files
            void loadTemplates();

            // fill in a notebook
            void fillPaletteNotebook(const base::RefPtr<ui::DockNotebook>& notebook);

            //--

        private:
            base::Array<PlaceableNodeTemplateInfo> m_templates;

            //--

            void findPaletteListFiles(base::depot::DepotStructure& loader, base::Array<base::res::ResourcePath>& outPaths) const;

            void createNotebookTab(const base::RefPtr<ui::DockNotebook>& notebook, const base::Array<PlaceableNodeTemplateInfo>& nodes);
        };

        ///--

        /// node template drag&drop data
        class EDITOR_SCENE_MAIN_API NodePaletteTemplateDragData : public ui::IDragDropData
        {
            RTTI_DECLARE_VIRTUAL_CLASS(NodePaletteTemplateDragData, ui::IDragDropData);

        public:
            NodePaletteTemplateDragData(const NewNodeSetup& templateInfo, const base::image::ImagePtr& icon);

            // get the dragged class
            INLINE const NewNodeSetup& setup() const { return m_setup; }


            // ui::IDragDropData
            virtual base::StringBuf asString() const override;
            virtual ui::DragDropPreviewPtr createPreview() const override;

        private:
            NewNodeSetup m_setup;
            base::image::ImagePtr m_icon;
        };

        ///--

    } // world
} // ed
