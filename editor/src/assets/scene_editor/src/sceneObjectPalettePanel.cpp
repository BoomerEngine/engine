/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#include "build.h"
#include "sceneObjectPalettePanel.h"
#include "sceneContentNodes.h"
#include "base/ui/include/uiSplitter.h"
#include "base/ui/include/uiListView.h"
#include "base/ui/include/uiSearchBar.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/resource/include/resourceTags.h"
#include "base/ui/include/uiComboBox.h"

namespace ed
{

    class ScenePreviewContainer;

    //---

    RTTI_BEGIN_TYPE_STRUCT(SceneResourceBasedObjectFactoryInfo);
    RTTI_PROPERTY(resourceClass);
    RTTI_PROPERTY(componentClass);
    RTTI_PROPERTY(entityClass);
    RTTI_END_TYPE();

    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneObjectPalettePanel);
    RTTI_END_TYPE();

    SceneObjectPalettePanel::SceneObjectPalettePanel(ScenePreviewContainer* preview)
    {
        enumerateResourceClasses();
        createInterface();
    }

    SceneObjectPalettePanel::~SceneObjectPalettePanel()
    {}

    void SceneObjectPalettePanel::enumerateResourceClasses()
    {
        InplaceArray<SpecificClassType<res::IResource>, 100> allResourceClasses;
        RTTI::GetInstance().enumClasses(allResourceClasses);

        for (const auto resClass : allResourceClasses)
        {
            Array<SpecificClassType<world::Component>> componentClasses;
            SceneContentNode::EnumComponentClassesForResource(resClass, componentClasses);

            Array<SpecificClassType<world::Entity>> entityClasses;
            SceneContentNode::EnumEntityClassesForResource(resClass, entityClasses);

            if (!componentClasses.empty() || !entityClasses.empty())
            {
                auto& info = m_resourceBindings.emplaceBack();
                info.resourceClass = resClass;
                info.allowedComponentClasses = std::move(componentClasses);
                info.allowedEntityClasses = std::move(entityClasses);

                if (!info.allowedEntityClasses.empty())
                    info.entityClass = info.allowedEntityClasses[0];

                if (!info.allowedComponentClasses.empty())
                    info.componentClass = info.allowedComponentClasses[0];
            }            
        }
    }

    //--

    class SceneResourceBasedObjectFactoryListVisElement : public ui::IElement
    {
    public:
        SceneResourceBasedObjectFactoryListVisElement(SceneResourceBasedObjectFactoryInfo* ptr)
            : m_data(ptr)
        {
            layoutVertical();
            createInterface();
        }

    private:
        SceneResourceBasedObjectFactoryInfo* m_data = nullptr;

        static void PrintClassName(StringBuilder& txt, ClassType cls)
        {
            StringView className;

            if (const auto* descData = cls->findMetadata<res::ResourceDescriptionMetadata>())
                className = descData->description();
            else
                className = cls->shortName().view();

            if (const auto* tagData = cls->findMetadata<res::ResourceTagColorMetadata>())
            {
                txt.appendf("[tag:{}]", tagData->color());
                txt.appendf("[img:file_empty_edit] ");

                if (tagData->color().luminanceSRGB() > 0.5f)
                {
                    txt << "[color:#000]";
                    txt << className;
                    txt << "[/color]";
                }
                else
                {
                    txt << className;
                }

                txt << "[/tag]";
            }
            else
            {
                txt << "[tag:#888]";
                txt.appendf("[img:cog] ");
                txt << className;
                txt << "[/tag]";
            }
        }

        void createInterface()
        {
            customPadding(6, 6, 6, 6);
            customMargins(8, 2, 8, 2);
            customStyle<Color>("border-color"_id, Color(80,80,80)); // HACK
            customStyle<float>("border-width"_id, 1.0f);
            //customStyle<float>("border-radius"_id, 8.0f);

            {
                StringBuilder txt;
                PrintClassName(txt, m_data->resourceClass);
                createChild<ui::TextLabel>(txt.view());
            }

            if (!m_data->allowedComponentClasses.empty())
            {
                auto line = createChild<ui::IElement>();
                line->layoutHorizontal();
                line->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                line->customMargins(15, 2, 0, 0);

                auto label = line->createChild<ui::TextLabel>("[img:class] Component class:");
                label->customVerticalAligment(ui::ElementVerticalLayout::Middle);

                auto choice = line->createChild<ui::ComboBox>();
                choice->customMargins(10, 0, 0, 0);
                choice->expand();

                for (const auto& cls : m_data->allowedComponentClasses)
                {
                    StringBuilder txt;
                    txt << "[img:component] ";
                    txt << cls->shortName();
                    choice->addOption(txt.toString());

                    if (m_data->componentClass == cls)
                        choice->selectOption(choice->numOptions() - 1);
                }
            }
        }
    };

    class SceneResourceBasedObjectFactoryListModel : public ui::SimpleTypedListModel<SceneResourceBasedObjectFactoryInfo*, SceneResourceBasedObjectFactoryInfo*>
    {
    public:
        virtual bool compare(SceneResourceBasedObjectFactoryInfo* a, SceneResourceBasedObjectFactoryInfo* b, int colIndex) const override
        {
            return a->resourceClass->name() < b->resourceClass->name();
        }

        virtual bool filter(SceneResourceBasedObjectFactoryInfo* data, const ui::SearchPattern& filter, int colIndex = 0) const override
        {
            return filter.testString(data->resourceClass->name().view());
        }

        virtual base::StringBuf content(SceneResourceBasedObjectFactoryInfo* data, int colIndex = 0) const override
        {
            return base::StringBuf(data->resourceClass->name().view());
        }

        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override
        {
            if (!content)
            {
                if (auto* ptr = data(item, nullptr))
                {
                    content = RefNew<SceneResourceBasedObjectFactoryListVisElement>(ptr);
                }
            }
        }
    };

    void SceneObjectPalettePanel::createInterface()
    {
        auto splitter = createChild<ui::Splitter>(ui::Direction::Horizontal);
        splitter->expand();

        {
            auto panel = splitter->createChild<ui::IElement>();
            panel->layoutVertical();
            panel->expand();

            panel->createChild<ui::TextLabel>("Default component/entity for given resource");

            m_classMappingList = panel->createChild<ui::ListView>();
            m_classMappingList->expand();

            m_classMappingModel = RefNew<SceneResourceBasedObjectFactoryListModel>();
            for (auto& info : m_resourceBindings)
                m_classMappingModel->add(&info);

            m_classMappingList->model(m_classMappingModel);
        }

        {
            auto panel = splitter->createChild<ui::IElement>();
            panel->layoutVertical();
            panel->expand();

            auto searchBar = panel->createChild<ui::SearchBar>();
            searchBar->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

            m_favouritePrefabsList = panel->createChild<ui::ListView>();
            m_favouritePrefabsList->expand();

            searchBar->bindItemView(m_favouritePrefabsList);
        }
    }

    SpecificClassType<world::Component> SceneObjectPalettePanel::selectedComponentClass(ClassType resClass) const
    {
        for (const auto res : m_resourceBindings)
            if (res.resourceClass == resClass)
                return res.componentClass;

        for (const auto res : m_resourceBindings)
            if (res.resourceClass->is(resClass))
                return res.componentClass;

        return nullptr;
    }

    SpecificClassType<world::Entity> SceneObjectPalettePanel::selectedEntityClass(ClassType resClass) const
    {
        for (const auto res : m_resourceBindings)
            if (res.resourceClass == resClass)
                return res.entityClass;

        for (const auto res : m_resourceBindings)
            if (res.resourceClass->is(resClass))
                return res.entityClass;

        return nullptr;
    }

    //--

    void SceneObjectPalettePanel::configSave(const ui::ConfigBlock& block) const
    {

    }

    void SceneObjectPalettePanel::configLoad(const ui::ConfigBlock& block)
    {

    }

    //---

} // ed