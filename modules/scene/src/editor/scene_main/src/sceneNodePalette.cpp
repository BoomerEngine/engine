/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tools #]
***/

#include "build.h"
#include "sceneNodePalette.h"
#include "base/object/include/resourcePath.h"
#include "base/depot/include/depotStructure.h"
#include "base/depot/include/depotFileSystem.h"
#include "base/image/include/image.h"

#include "ui/toolkit/include/uiWindowOverlay.h"
#include "ui/toolkit/include/uiDragDrop.h"
#include "ui/toolkit/include/uiStaticContent.h"
#include "ui/widgets/include/uiDockPanel.h"
#include "ui/widgets/include/uiDockNotebook.h"
#include "ui/widgets/include/uiMenuPopup.h"
#include "ui/models/include/uiAbstractItemModel.h"
#include "ui/models/include/uiListView.h"

#include "editor/asset_browser/include/managedFile.h"
#include "editor/asset_browser/include/managedFileFormat.h"
#include "editor/asset_browser/include/editorService.h"
#include "editor/asset_browser/include/managedDepot.h"
#include "ui/models/include/uiSimpleListModel.h"
#include "base/image/include/image.h"

namespace ed
{
    namespace world
    {

        //--

        RTTI_BEGIN_TYPE_STRUCT(PlaceableNodeTemplateInfo);
            RTTI_PROPERTY(m_name);
            RTTI_PROPERTY(m_icon);
            RTTI_PROPERTY(m_tab);
            RTTI_PROPERTY(m_category);
            RTTI_PROPERTY(m_description);
            RTTI_PROPERTY(m_prefab);
        RTTI_END_TYPE();

        PlaceableNodeTemplateInfo::PlaceableNodeTemplateInfo()
        {}

        //--

        RTTI_BEGIN_TYPE_CLASS(PlacableNodeTemplateListFile);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4palette");
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Editor Template Palette");
            RTTI_PROPERTY(m_templates);
        RTTI_END_TYPE();

        PlacableNodeTemplateListFile::PlacableNodeTemplateListFile()
        {}

        //--

        scene::NodeTemplatePtr NewNodeSetup::createNode() const
        {
            return nullptr;
        }

        //--

        PlaceableNodeTemplateRegistry::PlaceableNodeTemplateRegistry()
        {}

        PlaceableNodeTemplateRegistry::~PlaceableNodeTemplateRegistry()
        {}

        void PlaceableNodeTemplateRegistry::findPaletteListFiles(base::depot::DepotStructure& loader, base::Array<base::res::ResourcePath>& outPaths) const
        {
            // look in each file system for the "editor/templates" folder
            for (auto fs  : loader.mountedFileSystems())
            {
                auto basePath = fs->mountPoint().path();
                base::StringBuf templatePath = base::TempString("{}editor/templates/", basePath);

                base::Array<base::depot::DepotStructure::FileInfo> files;
                loader.enumFilesAtPath(templatePath, files);
                TRACE_INFO("Found {} file(s) at '{}'", files.size(), templatePath);

                for (auto& fileInfo : files)
                {
                    if (fileInfo.name.endsWith("v4palette"))
                    {
                        base::StringBuf fullPath = base::TempString("{}{}", templatePath, fileInfo.name);
                        outPaths.emplaceBack(base::res::ResourcePath(fullPath));
                    }
                }
            }
        }

        void PlaceableNodeTemplateRegistry::loadTemplates()
        {
            base::Array<base::res::ResourcePath> paths;
            findPaletteListFiles(base::GetService<Editor>()->managedDepot().loader(), paths);
            TRACE_INFO("Found {} pallete list file(s)", paths.size());

            for (auto& path : paths)
            {
                if (auto paletteFile = base::LoadResource<PlacableNodeTemplateListFile>(path))
                {
                    for (auto& tempInfo : paletteFile->templates())
                        m_templates.pushBack(tempInfo);
                }
                else
                {
                    TRACE_ERROR("Failed to load template list from '{}'", path);
                }
            }

            TRACE_INFO("Loaded {} node template(s)", m_templates.size());

            std::sort(m_templates.begin(), m_templates.end(), [](PlaceableNodeTemplateInfo& a, PlaceableNodeTemplateInfo& b)
                {
                    if (a.m_tab != b.m_tab)
                        return a.m_tab.view() < b.m_tab.view();

                    if (a.m_category != b.m_category)
                        return a.m_category.view() < b.m_category.view();

                    return a.m_name < b.m_name;
                });
        }

        //--

        class NodePaletteTemplateDragDropPreview : public ui::DragDropPreview
        {
            RTTI_DECLARE_VIRTUAL_CLASS(NodePaletteTemplateDragDropPreview, ui::DragDropPreview);

        public:
            NodePaletteTemplateDragDropPreview(const NewNodeSetup& templateInfo, const base::image::ImagePtr& icon)
            {
                layoutMode(ui::LayoutMode::Horizontal);

                auto iconNode = base::CreateSharedPtr<ui::StaticContent>();
                iconNode->customImage(icon);
                attachChild(iconNode);

                auto textNode = base::CreateSharedPtr<ui::StaticContent>();
                textNode->text(templateInfo.m_template->m_name);
                textNode->customMargins(ui::Offsets(5,0,5,0));
                attachChild(textNode);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(NodePaletteTemplateDragDropPreview);
        RTTI_END_TYPE();

        //--

        NodePaletteTemplateDragData::NodePaletteTemplateDragData(const NewNodeSetup& templateInfo, const base::image::ImagePtr& icon)
            : m_setup(templateInfo)
            , m_icon(icon)
        {
        }

        base::StringBuf NodePaletteTemplateDragData::asString() const
        {
            return "";
        }

        ui::DragDropPreviewPtr NodePaletteTemplateDragData::createPreview() const
        {
            return base::CreateSharedPtr<NodePaletteTemplateDragDropPreview>(m_setup, m_icon);
        }

        RTTI_BEGIN_TYPE_CLASS(NodePaletteTemplateDragData);
        RTTI_END_TYPE();

        //--

        static base::res::StaticResource<base::image::Image> resNode("engine/ui/styles/icons/viewmode.png");

        class NodePaletteListModel : public ui::SimpleListModel
        {
            RTTI_DECLARE_VIRTUAL_CLASS(NodePaletteListModel, ui::SimpleListModel);

        public:
            NodePaletteListModel(const base::Array<PlaceableNodeTemplateInfo>& templates)
                : SimpleListModel(templates.size())
                , m_templates(templates)
            {}

            virtual base::StringBuf caption(uint32_t index) const override
            {
                return m_templates[index].m_name;
            }

            virtual base::image::ImagePtr icon(uint32_t index) const override
            {
                if (auto icon = m_templates[index].m_icon.peak())
                    return icon;

                return resNode.loadAndGet();
            }

            virtual ui::DragDropDataPtr queryDragDropData(const base::input::BaseKeyFlags& keys, const ui::ModelIndex& item) override final
            {
                NewNodeSetup setup;
                setup.m_template = &m_templates[item.row()];
                
                auto icon = this->icon(item.row());
                return base::CreateSharedPtr<NodePaletteTemplateDragData>(setup, icon);
            }

        private:
            base::Array<PlaceableNodeTemplateInfo> m_templates;
        };

        RTTI_BEGIN_TYPE_CLASS(NodePaletteListModel);
        RTTI_END_TYPE();

        //--

        static base::res::StaticResource<base::image::Image> resClass("engine/ui/styles/icons/class.png");

        void PlaceableNodeTemplateRegistry::createNotebookTab(const base::RefPtr<ui::DockNotebook>& notebook, const base::Array<PlaceableNodeTemplateInfo>& nodes)
        {
            if (!nodes.empty())
            {
                auto listView = base::CreateSharedPtr<ui::ListView>();
                listView->customProportion(1.0f);
                listView->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                listView->customVerticalAligment(ui::ElementVerticalLayout::Expand);
                listView->layoutMode(ui::LayoutMode::Icons);
                listView->visualizationMode(ui::ItemVisiualizationMode::ListIcon);

                auto model = base::CreateSharedPtr<NodePaletteListModel>(nodes);
                listView->bind(model);

                auto panel = base::CreateSharedPtr<ui::DockPanel>(nodes.front().m_tab.c_str(), false, false);
                panel->icon(resClass.loadAndGet());
                panel->attachChild(listView);
                panel->layoutMode(ui::LayoutMode::Vertical);

                notebook->addPanel(panel);
            }
        }

        void PlaceableNodeTemplateRegistry::fillPaletteNotebook(const base::RefPtr<ui::DockNotebook>& notebook)
        {
            base::StringID lastTabName;
            base::Array<PlaceableNodeTemplateInfo> tabEntries;

            for (auto& entry : m_templates)
            {
                if (entry.m_tab.empty())
                    continue;

                if (lastTabName != entry.m_tab)
                {
                    createNotebookTab(notebook, tabEntries);
                    lastTabName = entry.m_tab;

                    tabEntries.clear();
                }

                tabEntries.pushBack(entry);
            }

            createNotebookTab(notebook, tabEntries);
        }

        //--

    } // world
} // ui
