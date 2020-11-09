/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#pragma once

#include "sceneEditMode.h"
#include "base/ui/include/uiSimpleListModel.h"

namespace ed
{
    //--

    struct SceneContentEditableObject;
    class SceneEditMode_Default;

    /// UI panel for all stuff related to editing entities
    class ASSETS_SCENE_EDITOR_API SceneDefaultPropertyInspectorPanel : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneDefaultPropertyInspectorPanel, ui::IElement)

    public:
        SceneDefaultPropertyInspectorPanel(SceneEditMode_Default* host);
        virtual ~SceneDefaultPropertyInspectorPanel();

        // unbind all objects
        void unbind();

        // show properties of given nodes
        void bind(const Array<SceneContentNodePtr>& nodes);

        //--

        void refreshName();
        void refreshProperties();

    protected:
        ui::EditBoxPtr m_name;
        ui::ListViewPtr m_prefabList;

        SceneEditMode_Default* m_host = nullptr;

        Array<ui::ElementPtr> m_commonElements;
        Array<ui::ElementPtr> m_dataElements;
        Array<ui::ElementPtr> m_entityElements;

        //---

        Array<SceneContentNodePtr> m_nodes; // active selection


        ui::DataInspectorPtr m_properties;
        ClassType m_commonClassType;

        ui::EditBoxPtr m_partClass;
        ui::ButtonPtr m_buttonChangeClass;

        ui::ClassPickerBoxPtr m_classPicker;

        void changeDataClass(ClassType newDataClass, const Array<SceneContentNodePtr>& nodes);

        void cmdChangeClass();
        void cmdChangeName();

        //--
    };

    //--

} // ed
