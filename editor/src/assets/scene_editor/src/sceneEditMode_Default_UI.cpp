/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#include "build.h"
#include "sceneEditMode_Default.h"
#include "sceneEditMode_Default_UI.h"
#include "sceneContentNodes.h"
#include "sceneContentStructure.h"
#include "sceneContentDataView.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/object/include/actionHistory.h"
#include "base/ui/include/uiEditBox.h"
#include "base/ui/include/uiListView.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/object/include/action.h"
#include "base/ui/include/uiGroup.h"
#include "base/ui/include/uiClassPickerBox.h"
#include "base/world/include/worldNodeTemplate.h"
#include "base/world/include/worldEntityTemplate.h"
#include "base/world/include/worldComponentTemplate.h"
#include "base/world/include/worldEntity.h"

namespace ed
{
    //--


    static bool ValidNodeName(StringView<char> name)
    {
        if (name.empty())
            return false;

        if (name == "<multiple names>")
            return true;

        for (const auto ch : name)
        {
            if (ch >= 'A' && ch <= 'Z')
                continue;
            if (ch >= 'a' && ch <= 'z')
                continue;
            if (ch >= '0' && ch <= '9')
                continue;
            if (ch == '_' || ch == '(' || ch == ')' || ch == '-' || ch == '+' || ch == '*')
                continue;
            return false;
        }

        return true;
    }

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneDefaultPropertyInspectorPanel);
    RTTI_END_TYPE();

    //--

    SceneDefaultPropertyInspectorPanel::SceneDefaultPropertyInspectorPanel(SceneEditMode_Default* host)
        : m_host(host)
    {
        layoutVertical();

        {
            auto group = createChild<ui::Group>("Name");
            m_commonElements.pushBack(group);

            auto panel = group->createChild<ui::IElement>();
            panel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            panel->layoutHorizontal();

            m_name = panel->createChild<ui::EditBox>(ui::EditBoxFeatureBit::AcceptsEnter);
            m_name->customMargins(5, 5, 5, 5);
            m_name->validation(&ValidNodeName);
            m_name->expand();

            m_name->bind(ui::EVENT_TEXT_ACCEPTED) = [this]()
            {
                cmdChangeName();
            };
        }

        {
            auto group = createChild<ui::Group>("Prefabs", false);
            m_entityElements.pushBack(group);

            m_prefabList = group->createChild<ui::ListView>();
            m_prefabList->expand();
            m_prefabList->customMinSize(0, 200);
            m_prefabList->customMaxSize(0, 200);

            auto panel = group->createChild<ui::IElement>();
            panel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            panel->layoutHorizontal();

            {
                auto button = panel->createChildWithType<ui::Button>("PushButton"_id, "[img:add] Add prefab");
                button->customProportion(1.0f);
                button->expand();
                button->bind(ui::EVENT_CLICKED) = [this]() {
                    //cmdAddPrefab();
                };
            }

            {
                auto button = panel->createChildWithType<ui::Button>("PushButton"_id, "[img:delete] Remove prefab");
                button->customProportion(1.0f);
                button->expand();
                button->bind(ui::EVENT_CLICKED) = [this]() {
                    //cmdRemovePrafab();
                };
            }
        }

        {
            auto group = createChild<ui::Group>("Data", true);
            m_dataElements.pushBack(group);

            {
                auto panel = group->createChild<ui::IElement>();
                panel->layoutHorizontal();
                panel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                panel->customMargins(4, 4, 4, 0);

                m_partClass = panel->createChild<ui::EditBox>();
                m_partClass->enable(false);
                m_partClass->text("<no data>");
                m_partClass->expand();
                m_partClass->customProportion(1.0f);
                m_partClass->customMargins(0, 0, 4, 0);

                {
                    m_buttonChangeClass = panel->createChildWithType<ui::Button>("DataPropertyButton"_id, "[img:class]");
                    m_buttonChangeClass->bind(ui::EVENT_CLICKED) = [this]() {
                        cmdChangeClass();
                    };
                }
            }

            m_properties = group->createChild<ui::DataInspector>();
            m_properties->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_properties->customMinSize(0, 700);
            m_properties->bindActionHistory(host->actionHistory());
            m_properties->customMargins(4, 4, 4, 0);
        }

        unbind();
    }

    SceneDefaultPropertyInspectorPanel::~SceneDefaultPropertyInspectorPanel()
    {}

    void SceneDefaultPropertyInspectorPanel::unbind()
    {
        m_nodes.reset();

        m_commonClassType = ClassType();

        m_name->text("");
        m_name->enable(false);

        if (m_classPicker)
            m_classPicker->requestClose();
        m_classPicker.reset();

        m_prefabList->model(nullptr);
        m_prefabList->enable(false);

        m_properties->bindNull();

        for (const auto& elem : m_commonElements)
            elem->visibility(false);

        for (const auto& elem : m_entityElements)
            elem->visibility(false);

        for (const auto& elem : m_dataElements)
            elem->visibility(false);
    }

    void SceneDefaultPropertyInspectorPanel::bind(const Array<SceneContentNodePtr>& nodes)
    {
        unbind();

        m_nodes = nodes;

        if (!m_nodes.empty())
        {
            bool allowDataElements = true;
            bool allowEntityElements = true;
            bool allowCommonElements = true;
            for (const auto& node : m_nodes)
            {
                if (!node->is<SceneContentDataNode>())
                    allowDataElements = false;

                if (!node->is<SceneContentEntityNode>())
                    allowEntityElements = false;

                if (node->type() == SceneContentNodeType::PrefabRoot)
                    allowCommonElements = false;
            }

            for (const auto& elem : m_commonElements)
                elem->visibility(true);

            if (allowDataElements)
                for (const auto& elem : m_dataElements)
                    elem->visibility(true);

            if (allowEntityElements)
                for (const auto& elem : m_entityElements)
                    elem->visibility(true);
        }

        refreshName();
        refreshProperties();
    }

    void SceneDefaultPropertyInspectorPanel::refreshName()
    {
        if (m_nodes.size() == 1)
        {
            m_name->text(m_nodes[0]->name());
            m_name->enable(true);
        }
        else
        {
            m_name->text("<multiple names>");
            m_name->enable(true);
        }
    }

    //--

    void SceneDefaultPropertyInspectorPanel::refreshProperties()
    {
        bool hasEntityData = false;
        bool hasComponentData = false;

        Array<SceneContentEditableObject> editableObjects;
        for (const auto& node : m_nodes)
        {
            if (auto dataNode = rtti_cast<SceneContentDataNode>(node))
            {
                if (auto editableData = dataNode->editableData())
                {
                    auto& info = editableObjects.emplaceBack();
                    info.baseData = dataNode->baseData();
                    info.editableData = dataNode->editableData();
                    info.name = dataNode->name();
                    info.owningNode = dataNode;
                    
                    if (editableData->is<base::world::EntityTemplate>())
                        hasEntityData = true;
                    else if (editableData->is<base::world::ComponentTemplate>())
                        hasComponentData = true;
                }
            }
        }

        // do not edit anything if both entities and components are selected at the same time
        if (hasEntityData && hasComponentData)
            editableObjects.reset();

        m_commonClassType = ClassType();

        if (m_classPicker)
            m_classPicker->requestClose();
        m_classPicker.reset();

        if (!editableObjects.empty())
        {
            ClassType commonClass;
            bool canResetData = false;
            bool commonClassInconsistent = false;
            for (const auto& data : editableObjects)
            {
                const auto dataClass = data.editableData->cls();
                if (!commonClass)
                {
                    if (!commonClassInconsistent)
                        commonClass = dataClass;
                }
                else if (commonClass != dataClass)
                {
                    commonClassInconsistent = true;
                    commonClass = ClassType();
                }
            }

            auto view = CreateSharedPtr<SceneContentNodeDataView>(std::move(editableObjects));
            m_properties->bindData(view, false);

            if (commonClassInconsistent)
            {
                m_partClass->text("<inconsistent class>");
                m_buttonChangeClass->enable(false);
            }
            else
            {
                m_commonClassType = commonClass;
                m_partClass->text(commonClass->name().view());
                m_buttonChangeClass->enable(true);
            }
        }
        else
        {
            m_properties->bindNull();
            m_buttonChangeClass->enable(false);
            m_partClass->text("<no data>");
        }
    }

    //--

    struct NameActionInfo
    {
        SceneContentNodePtr node;
        StringBuf oldName;
        StringBuf newName;
    };

    struct ActionRenameNodes : public IAction
    {
    public:
        ActionRenameNodes(Array<NameActionInfo>&& nodes, SceneDefaultPropertyInspectorPanel* host)
            : m_nodes(std::move(nodes))
            , m_host(host)
        {
        }

        virtual StringID id() const override
        {
            return "RenameNodes"_id;
        }

        StringBuf description() const override
        {
            if (m_nodes.size() == 1)
                return TempString("Rename node");
            else
                return TempString("Rename {} nodes", m_nodes.size());
        }

        virtual bool execute() override
        {
            for (const auto& info : m_nodes)
                info.node->name(info.newName);
            m_host->refreshName();
            return true;
        }

        virtual bool undo() override
        {
            for (const auto& info : m_nodes)
                info.node->name(info.oldName);
            m_host->refreshName();
            return true;
        }

    private:
        Array<NameActionInfo> m_nodes;
        SceneDefaultPropertyInspectorPanel* m_host;
    };

    void SceneDefaultPropertyInspectorPanel::cmdChangeName()
    {
        Array<NameActionInfo> infos;

        const auto nameText = m_name->text();
        if (!ValidNodeName(nameText))
            return;

        if (nameText == "<multiple names>")
            return;

        if (m_nodes.size() == 1)
        {
            auto parent = m_nodes[0]->parent();
            if (parent)
            {
                auto& info = infos.emplaceBack();
                info.node = m_nodes[0];
                info.oldName = m_nodes[0]->name();
                info.newName = parent->buildUniqueName(nameText, true);
            }
        }
        else
        {
            HashMap<const SceneContentNode*, HashSet<StringBuf>> usedNames;
            usedNames.reserve(m_nodes.size());

            for (const auto& node : m_nodes)
            {
                if (auto parent = node->parent())
                {
                    auto& parentUsedNames = usedNames[parent];

                    auto& info = infos.emplaceBack();
                    info.node = node;
                    info.oldName = node->name();
                    info.newName = parent->buildUniqueName(nameText, true, &parentUsedNames);
                    parentUsedNames.insert(info.newName);
                }
            }
        }

        if (!infos.empty())
        {
            auto action = CreateSharedPtr<ActionRenameNodes>(std::move(infos), this);
            m_host->actionHistory()->execute(action);
        }
    }

    //--

    struct ChangeClassActionInfo
    {
        SceneContentDataNodePtr node;
        ObjectTemplatePtr oldData;
        ObjectTemplatePtr newData;
    };

    struct ActionChangeDataClass : public IAction
    {
    public:
        ActionChangeDataClass(Array<ChangeClassActionInfo>&& nodes, SceneDefaultPropertyInspectorPanel* host)
            : m_nodes(std::move(nodes))
            , m_host(host)
        {
        }

        virtual StringID id() const override
        {
            return "ChangeNodesDataClass"_id;
        }

        StringBuf description() const override
        {
            if (m_nodes.size() == 1)
                return TempString("Change node class");
            else
                return TempString("Change class on {} nodes", m_nodes.size());
        }

        virtual bool execute() override
        {
            for (const auto& info : m_nodes)
                info.node->changeData(info.newData);
            m_host->refreshProperties();
            return true;
        }

        virtual bool undo() override
        {
            for (const auto& info : m_nodes)
                info.node->changeData(info.oldData);
            m_host->refreshProperties();
            return true;
        }

    private:
        Array<ChangeClassActionInfo> m_nodes;
        SceneDefaultPropertyInspectorPanel* m_host;
    };

    static bool IsClassCompatible(const SceneContentDataNodePtr& dataNode, ClassType newDataClass)
    {
        if (dataNode->editableData()->is<world::EntityTemplate>())
        {
            if (dataNode->baseData())
                return newDataClass->is(dataNode->baseData()->cls());
            else
                return newDataClass->is<world::EntityTemplate>();
        }
        else if (dataNode->editableData()->is<world::ComponentTemplate>())
        {
            if (dataNode->baseData())
                return newDataClass->is(dataNode->baseData()->cls());
            else
                return newDataClass->is<world::ComponentTemplate>();
        }
    
        return false;
    }

    //--

    struct DupaEntityTemplate : public base::world::EntityTemplate
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DupaEntityTemplate, base::world::EntityTemplate);

    public:
        virtual world::EntityPtr createEntity() const override { return CreateSharedPtr<world::Entity>(); }

        int m_value = 0;
    };

    RTTI_BEGIN_TYPE_CLASS(DupaEntityTemplate);
        RTTI_PROPERTY(m_value).editable().overriddable();
    RTTI_END_TYPE();

    //--

    struct DupaComponentTemplate : public base::world::ComponentTemplate
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DupaComponentTemplate, base::world::ComponentTemplate);

        virtual world::ComponentPtr createComponent() const override { return nullptr; }

    public:
        int m_value = 0;
    };

    RTTI_BEGIN_TYPE_CLASS(DupaComponentTemplate);
        RTTI_PROPERTY(m_value).editable().overriddable();
    RTTI_END_TYPE();

    //--

    void SceneDefaultPropertyInspectorPanel::changeDataClass(ClassType newDataClass, const Array<SceneContentNodePtr>& inputNodes)
    {
        Array<ChangeClassActionInfo> nodes;

        for (const auto& node : inputNodes)
        {
            if (auto dataNode = rtti_cast<SceneContentDataNode>(node))
            {
                if (IsClassCompatible(dataNode, newDataClass))
                {
                    auto& info = nodes.emplaceBack();
                    info.node = dataNode;
                    info.oldData = dataNode->editableData();
                    info.newData = CloneObject(info.oldData, nullptr, nullptr, newDataClass.cast<IObject>());
                }
            }
        }

        if (!nodes.empty())
        {
            auto action = CreateSharedPtr<ActionChangeDataClass>(std::move(nodes), this);
            m_host->actionHistory()->execute(action);
        }
    }

    static ClassType SelectRootClass(ClassType currentClass)
    {
        if (currentClass->is<base::world::EntityTemplate>())
            return base::world::EntityTemplate::GetStaticClass();

        if (currentClass->is<base::world::ComponentTemplate>())
            return base::world::ComponentTemplate::GetStaticClass();

        return nullptr;
    }

    void SceneDefaultPropertyInspectorPanel::cmdChangeClass()
    {
        if (!m_classPicker && m_commonClassType)
        {
            if (const auto rootClass = SelectRootClass(m_commonClassType))
            {
                m_classPicker = base::CreateSharedPtr<ui::ClassPickerBox>(rootClass, m_commonClassType, false, false);

                m_classPicker->bind(ui::EVENT_WINDOW_CLOSED) = [this]() {
                    m_classPicker.reset();
                };

                m_classPicker->bind(ui::EVENT_CLASS_SELECTED) = [this](base::ClassType data) {
                    changeDataClass(data, m_nodes);
                };

                m_classPicker->show(this, ui::PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));
            }
        }
    }

    //--
    
} // ed
