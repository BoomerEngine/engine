/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tools #]
***/

#include "build.h"
#include "sceneNodeClassSelector.h"
//#include "sceneNodeClassDragDropData.h"

#include "base/image/include/image.h"

#include "ui/models/include/uiListView.h"
#include "ui/models/include/uiSimpleListModel.h"
#include "ui/toolkit/include/uiCommandBindings.h"

#include "scene/common/include/sceneNodeTemplate.h"
#include "scene/common/include/sceneNodeTemplate.h"

namespace ed
{
    namespace world
    {

        ///---

        static base::res::StaticResource<base::image::Image> resNode("engine/ui/styles/icons/node.png");

        class NodeClassListModel : public ui::SimpleListModel
        {
        public:
            NodeClassListModel(const base::Array<base::ClassType>& templateClasses)
                : SimpleListModel(templateClasses.size())
            {
            }

            virtual base::StringBuf caption(uint32_t index) const override
            {
                auto cls  = m_classes[index];

                /*auto classCaption = cls->findMetadata<scene::NodeTemplateClassName>();
                if (classCaption)
                    return base::StringBuf(classCaption->name());*/

                auto shortName = cls->name().view().afterLast("::");
                return base::StringBuf(shortName.data());
            }

            virtual base::image::ImagePtr icon(uint32_t index) const override
            {
                auto cls  = m_classes[index];

                /*auto iconNameMetaData = cls->findMetadata<scene::NodeTemplateIconName>();
                if (iconNameMetaData)
                    return base::StringID(iconNameMetaData->name());
                else*/
                return resNode.loadAndGet();
            }

        private:
            base::Array<base::ClassType> m_classes;
        };

        //--

        RTTI_BEGIN_TYPE_CLASS(NodeClassSelector);
            RTTI_PROPERTY(m_classList).metadata<ui::ElementBindingName>("ClassList");
        RTTI_END_TYPE();

        static base::res::StaticResource<base::storage::XMLData> resSceneNodeClassSelector("engine/ui/templates/scene/class_selector.xml");

        NodeClassSelector::NodeClassSelector()
            : m_rootClass(scene::NodeTemplate::GetStaticClass())
        {
            ui::ApplyTemplate(this, resSceneNodeClassSelector.loadAndGet());

            // refresh class list
            refreshClassList();

            // bind actions
            commandBindings().bindCommand("Create") = [this](UI_EVENT)
            {
                auto selectedClass  = this->selectedClass();
                if (selectedClass && selectedClass->is(m_rootClass))
                {
                    if (OnClassSelected)
                        OnClassSelected(selectedClass);

                    detach();
                }
            };

            commandBindings().bindCommand("Cancel") = [this](UI_EVENT)
            {
                detach();
            };
        }

        base::ClassType NodeClassSelector::selectedClass() const
        {
            /*auto selected = m_classList->selectedItem();
            auto classNode = base::rtti_cast<NodeClassListElement>(selected);
            return classNode ? classNode->templateElementClass() : nullptr;*/
            return nullptr;
        }

        void NodeClassSelector::refreshClassList()
        {
            // get all classes of template elements
            base::Array<const base::rtti::IClassType *> templateClasses;
            base::rtti::ITypeSystem::GetInstance().enumClasses(m_rootClass, templateClasses);

            // create elements
            auto model = base::CreateSharedPtr<NodeClassListModel>(templateClasses);
            m_classList->bind(model);
        }

        ///---

    } // mesh
} // ed