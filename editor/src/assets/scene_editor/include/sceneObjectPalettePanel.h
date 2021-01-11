/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

#include "base/ui/include/uiElement.h"
#include "base/world/include/worldComponent.h"
#include "base/world/include/worldEntity.h"
#include "base/ui/include/uiSimpleListModel.h"

namespace ed
{
    //--

    struct SceneResourceBasedObjectFactoryInfo
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(SceneResourceBasedObjectFactoryInfo);

    public:
        ClassType resourceClass;
        SpecificClassType<world::Component> componentClass;
        SpecificClassType<world::Entity> entityClass;

        Array<SpecificClassType<world::Component>> allowedComponentClasses;
        Array<SpecificClassType<world::Entity>> allowedEntityClasses;
    };

    class SceneResourceBasedObjectFactoryListModel;

    //--

    /// general object palette panel for scene editor
    class ASSETS_SCENE_EDITOR_API SceneObjectPalettePanel : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneObjectPalettePanel, ui::IElement);

    public:
        SceneObjectPalettePanel(ScenePreviewContainer* preview);
        virtual ~SceneObjectPalettePanel();

        virtual void configSave(const ui::ConfigBlock& block) const;
        virtual void configLoad(const ui::ConfigBlock& block);

        SpecificClassType<world::Component> selectedComponentClass(ClassType resClass) const;
        SpecificClassType<world::Entity> selectedEntityClass(ClassType resClass) const;

    private:
        Array<SceneResourceBasedObjectFactoryInfo> m_resourceBindings;

        ui::ListViewPtr m_classMappingList;
        RefPtr<SceneResourceBasedObjectFactoryListModel> m_classMappingModel;

        ui::ListViewPtr m_favouritePrefabsList;

        void enumerateResourceClasses();
        void createInterface();
    };

    //--

} // ed
