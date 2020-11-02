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
#include "game/world/include/worldNodeTemplate.h"
#include "game/world/include/worldEntityTemplate.h"
#include "game/world/include/worldComponentTemplate.h"
#include "base/object/include/action.h"
#include "base/ui/include/uiGroup.h"

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
            auto group = createChild<ui::Group>("Payload", true);
            m_entityElements.pushBack(group);

            m_partList = group->createChild<ui::ListView>();
            m_partList->expand();
            m_partList->customMinSize(0, 200);
            m_partList->customMaxSize(0, 200);

            m_partList->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]() {
                refreshProperties();
            };

            auto panel = group->createChild<ui::IElement>();
            panel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            panel->layoutHorizontal();

            {
                m_buttonCreateComponent = panel->createChildWithType<ui::Button>("PushButton"_id, "[img:add] Add component");
                m_buttonCreateComponent->addStyleClass("green"_id);
                m_buttonCreateComponent->customProportion(1.0f);
                m_buttonCreateComponent->expand();
                m_buttonCreateComponent->bind(ui::EVENT_CLICKED) = [this]() {
                    cmdAddComponent();
                };
            }

            {
                m_buttonRemoveComponent = panel->createChildWithType<ui::Button>("PushButton"_id, "[img:delete] Remove component");
                m_buttonRemoveComponent->addStyleClass("red"_id);
                m_buttonRemoveComponent->customProportion(1.0f);
                m_buttonRemoveComponent->expand();
                m_buttonRemoveComponent->bind(ui::EVENT_CLICKED) = [this]() {
                    cmdRemoveComponent();
                };
            }
        }

        {
            auto group = createChild<ui::Group>("Properties", true);
            m_entityElements.pushBack(group);

            auto panel = group->createChild<ui::IElement>();
            panel->layoutHorizontal();
            panel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

            {
                m_buttonCreateLocalData = panel->createChildWithType<ui::Button>("PushButton"_id, "[img:add] Add data");
                m_buttonCreateLocalData->addStyleClass("green"_id);
                m_buttonCreateLocalData->customProportion(1.0f);
                m_buttonCreateLocalData->expand();
                m_buttonCreateLocalData->bind(ui::EVENT_CLICKED) = [this]() {
                    cmdCreateLocalData();
                };
            }

            {
                m_buttonRemoveLocalData = panel->createChildWithType<ui::Button>("PushButton"_id, "[img:delete] Remove data");
                m_buttonRemoveLocalData->addStyleClass("red"_id);
                m_buttonRemoveLocalData->customProportion(1.0f);
                m_buttonRemoveLocalData->expand();
                m_buttonRemoveLocalData->bind(ui::EVENT_CLICKED) = [this]() {
                    cmdRemoveLocalData();
                };
            }

            m_properties = group->createChild<ui::DataInspector>();
            m_properties->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_properties->customMinSize(0, 700);
            m_properties->bindActionHistory(host->actionHistory());
        }
    }

    SceneDefaultPropertyInspectorPanel::~SceneDefaultPropertyInspectorPanel()
    {}

    void SceneDefaultPropertyInspectorPanel::unbind()
    {
        m_nodes.reset();

        m_name->text("");
        m_name->enable(false);

        m_prefabList->model(nullptr);
        m_prefabList->enable(false);

        m_partList->model(nullptr);
        m_partList->enable(false);
        m_partListModel.reset();

        m_properties->bindNull();

        for (const auto& elem : m_commonElements)
            elem->visibility(false);

        for (const auto& elem : m_entityElements)
            elem->visibility(false);
    }

    void SceneDefaultPropertyInspectorPanel::bind(const Array<SceneContentNodePtr>& nodes)
    {
        unbind();

        m_nodes = nodes;

        if (!m_nodes.empty())
        {
            bool allowAntityElements = true;
            bool allowCommonElements = true;
            for (const auto& node : m_nodes)
            {
                if (!node->is<SceneContentEntityNode>())
                    allowAntityElements = false;

                if (node->type() == SceneContentNodeType::PrefabRoot)
                    allowCommonElements = false;
            }

            for (const auto& elem : m_commonElements)
                elem->visibility(true);

            if (allowAntityElements)
                for (const auto& elem : m_entityElements)
                    elem->visibility(true);
        }

        refreshName();
        refreshPartList(StringID());
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

    bool SceneDefaultPropertyInspectorPanel::PartListModel::compare(const PartInfo& a, const PartInfo& b, int colIndex) const
    {
        return a.name < b.name;
    }

    bool SceneDefaultPropertyInspectorPanel::PartListModel::filter(const PartInfo& data, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
    {
        return filter.testString(data.name.view());
    }

    base::StringBuf SceneDefaultPropertyInspectorPanel::PartListModel::content(const PartInfo& data, int colIndex /*= 0*/) const
    {
        base::StringBuilder txt;

        if (data.name)
        {
            txt << "[img:component] ";
            txt << data.name;
        }
        else
        {
            txt << "[img:brick_blue] Entity";
        }

        if (data.localData)
            txt << "[tag:#88A]Local[/tag]";

        if (data.overrideData)
            txt << "[tag:#888]Override[/tag]";

        if (!data.localData && !data.overrideData)
            txt << "[tag:#888]No data[/tag]";

        return txt.toString();
    }

    void SceneDefaultPropertyInspectorPanel::collectSelectedParts(Array<PartInfo>& outParts) const
    {
        for (const auto id : m_partList->selection())
            outParts.emplaceBack(m_partListModel->data(id));
    }

    void SceneDefaultPropertyInspectorPanel::refreshPartList(base::StringID autoSelectName)
    {
        m_partList->model(nullptr);
        m_partList->enable(false);
        m_partListModel.reset();

        HashMap<StringID, PartInfo> partsMap;
        for (const auto& node : m_nodes)
        {
            if (auto entityNode = rtti_cast<SceneContentEntityNode>(node))
            {
                for (const auto& payload : entityNode->payloads())
                {
                    auto& part = partsMap[payload->name];
                    part.name = payload->name;
                    part.localData |= (nullptr != payload->editableData);
                    part.overrideData |= (nullptr != payload->baseData);
                }
            }
        }

        if (!partsMap.empty())
        {
            m_partListModel = CreateSharedPtr<PartListModel>();

            ui::ModelIndex selectedIndex;
            for (const auto& data : partsMap.values())
            {
                const auto index = m_partListModel->add(data);
                if (data.name == autoSelectName)
                    selectedIndex = index;
            }

            m_partList->model(m_partListModel);
            m_partList->enable(true);

            if (selectedIndex)
                m_partList->select(selectedIndex, ui::ItemSelectionModeBit::Default, false);
        }

        refreshProperties();
    }

    void SceneDefaultPropertyInspectorPanel::collectSelectedEditableObjects(Array<SceneContentEditableObject>& outList) const
    {
        // collect names of selected parts
        HashSet<StringID> selectedNames;
        for (const auto& id : m_partList->selection())
            selectedNames.insert(m_partListModel->data(id).name);

        // extract data from nodes
        for (const auto& node : m_nodes)
        {
            if (auto entityNode = rtti_cast<SceneContentEntityNode>(node))
            {
                for (const auto& payload : entityNode->payloads())
                {
                    if (selectedNames.contains(payload->name))
                    {
                        auto& outData = outList.emplaceBack();
                        outData.owningNode = entityNode;
                        outData.name = payload->name;
                        outData.baseData = payload->baseData;
                        outData.editableData = payload->editableData;
                    }
                }
            }
        }
    }

    //--

    void SceneDefaultPropertyInspectorPanel::refreshProperties()
    {
        Array<SceneContentEditableObject> content;
        collectSelectedEditableObjects(content);

        bool canCreateLocalData = false;
        bool canRemoveLocalData = false;
        for (const auto& data : content)
        {
            canCreateLocalData |= !data.editableData;
            canRemoveLocalData |= (data.editableData && data.baseData);
        }

        m_buttonCreateLocalData->enable(canCreateLocalData);
        m_buttonRemoveLocalData->enable(canRemoveLocalData);

        if (content.empty())
        {
            m_properties->bindNull();
        }
        else
        {
            auto view = CreateSharedPtr<SceneContentNodeDataView>(std::move(content));
            m_properties->bindData(view, false);
        }
    }

    //--

    void SceneDefaultPropertyInspectorPanel::cmdAddComponent()
    {

    }

    void SceneDefaultPropertyInspectorPanel::cmdRemoveComponent()
    {

    }

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

    void SceneDefaultPropertyInspectorPanel::cmdCreateLocalData()
    {

    }

    void SceneDefaultPropertyInspectorPanel::cmdRemoveLocalData()
    {

    }

    void SceneDefaultPropertyInspectorPanel::cmdChangeClass()
    {

    }

    //--
    
} // ed
