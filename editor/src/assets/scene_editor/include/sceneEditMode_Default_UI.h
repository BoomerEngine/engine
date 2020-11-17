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

    enum class SceneNodeTransformValueFieldType : uint16_t
    {
        TranslationX,
        TranslationY,
        TranslationZ,
        RotationX,
        RotationY,
        RotationZ,
        ScaleX,
        ScaleY,
        ScaleZ,

        MAX,
    };

    enum class SceneNodeTransformValueFileldBit : uint16_t
    {
        TranslationX = FLAG(0),
        TranslationY = FLAG(1),
        TranslationZ = FLAG(2),
        RotationX = FLAG(3),
        RotationY = FLAG(4),
        RotationZ = FLAG(5),
        ScaleX = FLAG(6),
        ScaleY = FLAG(7),
        ScaleZ = FLAG(8),

        ALL_TRANSLATION = TranslationX | TranslationY | TranslationZ,
        ALL_ROTATION = RotationX | RotationY | RotationZ,
        ALL_SCALE = ScaleX | ScaleY | ScaleZ,

        ALL = ALL_TRANSLATION | ALL_ROTATION | ALL_SCALE,
    };

    typedef DirectFlags<SceneNodeTransformValueFileldBit> SceneNodeTransformValueFileldFlags;

    //--

    struct SceneNodeTransformEntryValue
    {
        int index = -1;
        SceneNodeTransformValueFileldBit flag;
        double value = 0.0;
        bool determined = false;
        bool enabled = false;
    };

    struct ASSETS_SCENE_EDITOR_API SceneNodeTransformEntry
    {
        static const auto NUM_ENTRIES = (uint32_t)SceneNodeTransformValueFieldType::MAX;
        static SceneNodeTransformValueFileldBit FIELD_BITS[NUM_ENTRIES];

        SceneNodeTransformEntryValue values[NUM_ENTRIES];

        SceneNodeTransformEntry();
        SceneNodeTransformEntry(const SceneNodeTransformEntry& other);
        SceneNodeTransformEntry& operator=(const SceneNodeTransformEntry& other);
        
        void clear(); // unset all
        void setup(const EulerTransform& transform, SceneNodeTransformValueFileldFlags determined=SceneNodeTransformValueFileldBit::ALL, SceneNodeTransformValueFileldFlags enabled = SceneNodeTransformValueFileldBit::ALL);
    };

    ///---

    class ASSETS_SCENE_EDITOR_API ISceneNodeTransformValuesBoxEventSink : public NoCopy
    {
    public:
        virtual ~ISceneNodeTransformValuesBoxEventSink();

        virtual void transformBox_ValueDragStart(GizmoSpace space, SceneNodeTransformValueFieldType field) = 0;
        virtual void transformBox_ValueDragUpdate(GizmoSpace space, SceneNodeTransformValueFieldType field, int step) = 0;
        virtual void transformBox_ValueDragEnd(GizmoSpace space, SceneNodeTransformValueFieldType field) = 0;
        virtual void transformBox_ValueDragCancel(GizmoSpace space, SceneNodeTransformValueFieldType field) = 0;

        virtual void transformBox_ValueChanged(GizmoSpace space, SceneNodeTransformValueFieldType field, double value) = 0;
        virtual void transformBox_ValueReset(GizmoSpace space, SceneNodeTransformValueFieldType field) = 0;
    };

    /// helper widget with typical TRS setup
    class ASSETS_SCENE_EDITOR_API SceneNodeTransformValuesBox : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneNodeTransformValuesBox, ui::IElement)

    public:
        SceneNodeTransformValuesBox(GizmoSpace space, ISceneNodeTransformValuesBoxEventSink* sink);

        INLINE GizmoSpace space() const { return m_space; }
        INLINE const SceneNodeTransformEntry& data() const { return m_data; }

        void show(const SceneNodeTransformEntry& data);

    protected:
        struct Elem
        {
            ui::EditBoxPtr box;
            ui::DraggerPtr drag;
        };

        Elem createElem(ui::IElement* parent, StringID style, StringView caption, SceneNodeTransformValueFieldType field);

        Elem m_elems[SceneNodeTransformEntry::NUM_ENTRIES];

        SceneNodeTransformEntry m_data;
        GizmoSpace m_space;

        ISceneNodeTransformValuesBoxEventSink* m_sink = nullptr;
    };

    //--

    struct SceneContentEditableObject;
    class SceneEditModeDefaultTransformDragger;
    class SceneEditMode_Default;

    /// UI panel for all stuff related to editing entities
    class ASSETS_SCENE_EDITOR_API SceneDefaultPropertyInspectorPanel : public ui::IElement, public ISceneNodeTransformValuesBoxEventSink
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
        void refreshTransforms();

    protected:
        ui::EditBoxPtr m_name;
        ui::ListViewPtr m_prefabList;

        SceneEditMode_Default* m_host = nullptr;

        Array<ui::ElementPtr> m_commonElements;
        Array<ui::ElementPtr> m_dataElements;
        Array<ui::ElementPtr> m_entityElements;

        //---

        Array<SceneContentNodePtr> m_nodes; // active selection

        //--

        RefPtr<SceneNodeTransformValuesBox> m_transformLocalValues; // values in local space
        RefPtr<SceneNodeTransformValuesBox> m_transformWorldValues; // values in world space

        //--

        ui::DataInspectorPtr m_properties;
        ClassType m_commonClassType;

        ui::EditBoxPtr m_partClass;
        ui::ButtonPtr m_buttonChangeClass;

        ui::ClassPickerBoxPtr m_classPicker;

        RefPtr<SceneEditModeDefaultTransformDragger> m_currentDragTransform;

        //--

        void changeDataClass(ClassType newDataClass, const Array<SceneContentNodePtr>& nodes);

        void cmdChangeClass();
        void cmdChangeName();

        //--

        void cancelTransformAction();
        void expandTransformValuesWithHierarchyChildren(const Array<SceneContentDataNodePtr>& nodes, Array<ActionMoveSceneNodeData>& undoData) const;

        virtual void transformBox_ValueDragStart(GizmoSpace space, SceneNodeTransformValueFieldType field) override final;
        virtual void transformBox_ValueDragUpdate(GizmoSpace space, SceneNodeTransformValueFieldType field, int step) override final;
        virtual void transformBox_ValueDragEnd(GizmoSpace space, SceneNodeTransformValueFieldType field) override final;
        virtual void transformBox_ValueDragCancel(GizmoSpace space, SceneNodeTransformValueFieldType field) override final;
        virtual void transformBox_ValueChanged(GizmoSpace space, SceneNodeTransformValueFieldType field, double value) override final;
        virtual void transformBox_ValueReset(GizmoSpace space, SceneNodeTransformValueFieldType field) override final;
    };

    //--

} // ed
