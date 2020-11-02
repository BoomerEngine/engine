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

    protected:
        ui::EditBoxPtr m_name;
        ui::ListViewPtr m_prefabList;

        SceneEditMode_Default* m_host = nullptr;

        Array<ui::ElementPtr> m_commonElements;
        Array<ui::ElementPtr> m_entityElements;

        //---

        struct PartInfo
        {
            StringID name; // null for entity, not null for components
            bool localData = false;
            bool overrideData = false;
        };

        class PartListModel : public ui::SimpleTypedListModel<PartInfo>
        {
        public:
            virtual bool compare(const PartInfo& a, const PartInfo& b, int colIndex) const override;
            virtual bool filter(const PartInfo& data, const ui::SearchPattern& filter, int colIndex = 0) const override;
            virtual base::StringBuf content(const PartInfo& data, int colIndex = 0) const override;
        };

        ui::ListViewPtr m_partList;
        ui::ButtonPtr m_buttonCreateComponent;
        ui::ButtonPtr m_buttonRemoveComponent;

        RefPtr<PartListModel> m_partListModel;

        Array<SceneContentNodePtr> m_nodes;

        void collectSelectedEditableObjects(Array<SceneContentEditableObject>& outList) const;
        void refreshPartList(base::StringID autoSelectName);
        void collectSelectedParts(Array<PartInfo>& outParts) const;

        void cmdAddComponent();
        void cmdRemoveComponent();
        void cmdChangeName();

        //---


        ui::DataInspectorPtr m_properties;
        ui::ButtonPtr m_buttonCreateLocalData;
        ui::ButtonPtr m_buttonRemoveLocalData;

        void refreshProperties();
        void cmdCreateLocalData();
        void cmdRemoveLocalData();
        void cmdChangeClass();

        //--
    };

    //--

} // ed
