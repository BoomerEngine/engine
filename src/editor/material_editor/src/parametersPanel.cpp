/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "parametersPanel.h"

#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiButton.h"
#include "engine/ui/include/uiEditBox.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiListView.h"
#include "engine/ui/include/uiSimpleListView.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiGroup.h"
#include "engine/ui/include/uiSearchBar.h"
#include "engine/ui/include/uiTextValidation.h"

#include "engine/material/include/materialTemplate.h"
#include "engine/material_graph/include/graph.h"
#include "engine/material/include/parameters.h"

#include "core/object/include/action.h"
#include "core/object/include/actionHistory.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialParametersPanel_ParameterElement);
    RTTI_METADATA(ui::ElementClassNameMetadata).name("SimpleListElement");
RTTI_END_TYPE();

DECLARE_UI_EVENT(EVENT_MATERIAL_PARAM_RENAME_REQUEST, StringBuf);
DECLARE_UI_EVENT(EVENT_MATERIAL_PARAM_DELETE_REQUEST);

MaterialParametersPanel_ParameterElement::MaterialParametersPanel_ParameterElement(IMaterialTemplateParam* param, ActionHistory* ah, bool initiallyExpaned)
    : m_param(AddRef(param))
{
    layoutVertical();

    customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

    auto group = createChild<ui::ExpandableContainer>(initiallyExpaned);
    group->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    group->body()->addStyleClass("opaque"_id);

    {
        auto bar = createChild<ui::IElement>();

        StringBuilder txt;
        txt << "[img:fx] ";
        txt << "[tag:#888]";
        txt << param->cls()->name().view().afterFirst("_");
        txt << "[/tag]";
        auto label = group->header()->createChild<ui::TextLabel>(txt.toString());
        label->customVerticalAligment(ui::ElementVerticalLayout::Middle);

        m_name = group->header()->createChild<ui::EditBox>(ui::EditBoxFeatureFlags{ ui::EditBoxFeatureBit::AcceptsEnter, ui::EditBoxFeatureBit::NoBorder });
        m_name->customMargins(4, 0, 0, 4);
        m_name->text(param->name().c_str());
        m_name->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_name->customVerticalAligment(ui::ElementVerticalLayout::Middle);

        const auto validFunc = ui::MakeAlphaNumValidationFunction("_");
        m_name->validation(validFunc);

        m_name->bind(ui::EVENT_TEXT_ACCEPTED) = [this]()
        {
            cmdTryRename();
        };
    }

    {
        m_data = group->body()->createChild<ui::DataInspector>();
        m_data->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_data->bindActionHistory(ah);

        ui::DataInspectorSettings settings;
        settings.showCategories = false;
        m_data->settings(settings);

        m_data->bindObject(param);

        auto button = group->body()->createChildWithType<ui::Button>("PushButton"_id, "[img:delete] Delete parameter");
        button->customHorizontalAligment(ui::ElementHorizontalLayout::Expand); // Center
        button->addStyleClass("red"_id);

        button->bind(ui::EVENT_CLICKED) = [this]()
        {
            cmdRemove();
        };
    }
}

void MaterialParametersPanel_ParameterElement::applyName(StringView name)
{
    m_name->text(name);
    invalidateDisplayList();
}

void MaterialParametersPanel_ParameterElement::cmdTryRename()
{
    auto name = StringBuf(m_name->text());
    call<StringBuf>(EVENT_MATERIAL_PARAM_RENAME_REQUEST, name);
}

void MaterialParametersPanel_ParameterElement::cmdRemove()
{
    call(EVENT_MATERIAL_PARAM_DELETE_REQUEST);
}

bool MaterialParametersPanel_ParameterElement::handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const
{
    return filter.testString(m_param->name().view());
}

void MaterialParametersPanel_ParameterElement::handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outInfo) const
{
    outInfo.index = uniqueIndex();
    outInfo.caption = m_param->name().view();
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(MaterialParametersPanel);
RTTI_END_TYPE();

MaterialParametersPanel::MaterialParametersPanel(const ActionHistoryPtr& actions)
    : m_actionHistrory(actions)
{
    layoutVertical();

    auto box = createChild<ui::SearchBar>();
    box->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

    m_list = createChild<ui::ListViewEx>();
    m_list->expand();
    m_list->sort(0);

    //box->bindItemView(m_list);

    {
        auto panel = RefNew<ui::IElement>();
        panel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        panel->customPadding(10, 10, 10, 10);

        auto button = panel->createChildWithType<ui::Button>("PushButton"_id, "[img:add] Add new parameter");
        button->customHorizontalAligment(ui::ElementHorizontalLayout::Expand); // Center
        button->addStyleClass("green"_id);

        auto buttonPtr = button.get();
        button->bind(ui::EVENT_CLICKED) = [this, buttonPtr]()
        {
            InplaceArray<SpecificClassType<IMaterialTemplateParam>, 10> paramClasses;
            RTTI::GetInstance().enumClasses(paramClasses);

            std::sort(paramClasses.begin(), paramClasses.end(), [](auto x, auto y)
                {
                    return x.name().view() < y.name().view();
                });

            auto popup = RefNew<ui::MenuButtonContainer>();

            for (const auto cls : paramClasses)
            {
                const auto paramName = cls->name().view().afterLastOrFull("_");
                popup->createCallback(TempString("Add {} parameter", paramName), "[img:fx]") = [this, cls]
                {
                    addParameter(cls);
                };
            }

            popup->showAsDropdown(buttonPtr);
        };

        //m_list->attachStaticElement(panel);
    }    
}

MaterialParametersPanel::~MaterialParametersPanel()
{}

void MaterialParametersPanel::bindGraph(const MaterialGraphPtr& graph)
{
    m_graph = graph;
    rebuildParameterList();
}

RefPtr<MaterialParametersPanel_ParameterElement> MaterialParametersPanel::findUI(IMaterialTemplateParam* param) const
{
    return AddRef(m_list->find<MaterialParametersPanel_ParameterElement>([param](MaterialParametersPanel_ParameterElement* elem)
        {
            return elem->param() == param;
        }));
}

RefPtr<MaterialParametersPanel_ParameterElement> MaterialParametersPanel::createUI(IMaterialTemplateParam* param, bool expanded)
{
    auto ui = RefNew<MaterialParametersPanel_ParameterElement>(param, m_actionHistrory, expanded);

    ui->bind(EVENT_MATERIAL_PARAM_DELETE_REQUEST) = [this, param]()
    {
        removeParameter(param);
    };

    ui->bind(EVENT_MATERIAL_PARAM_RENAME_REQUEST) = [this, param](StringBuf newName)
    {
        renameParameter(param, newName);
    };

    return ui;
}

void MaterialParametersPanel::rebuildParameterList()
{
    m_list->clear();

    for (const auto& param : m_graph->parameters())
        m_list->addItem(createUI(param, false));
}

bool MaterialParametersPanel::removeParameter(IMaterialTemplateParam* param)
{
    if (m_graph->checkParameterUsed(param))
    {
        ui::PostWindowMessage(this, ui::MessageType::Error, "Delete"_id, "Parameter is in use, cannot delete");
        return false;
    }

    if (auto ui = findUI(param))
    {
        auto action = RefNew<ActionInplace>(TempString("Remove parameter {}", param->name()), "RemoveMaterialParam"_id);

        auto obj = MaterialTemplateParamPtr(AddRef(param));

        action->doFunc = [this, obj, ui]() {
            m_graph->detachParameter(obj);
            m_list->removeItem(ui);
            return true;
        };

        action->undoFunc = [this, obj, ui]() {
            m_graph->attachParameter(obj);
            m_list->addItem(ui);
            return true;
        };

        return m_actionHistrory->execute(action);
    }
    else
    {
        return false;
    }
}

bool MaterialParametersPanel::renameParameter(IMaterialTemplateParam* param, StringView newName)
{
    auto ui = findUI(param);
    if (!ui)
        return false;

    if (!ui::ValidateIdentifier(newName))
    {
        ui::PostWindowMessage(this, ui::MessageType::Error, "Rename"_id, "Invalid parameter name");
        ui->applyName(param->name().view());
        return false;
    }

    auto oldName = param->name();

    StringID correctedName;
    uint32_t index = 0;
    for (;;)
    {
        if (index == 0)
            correctedName = StringID(newName);
        else
            correctedName = StringID(TempString("{}{}", newName.trimTailNumbers(), index));

        bool hasNameCollision = false;
        for (const auto& otherParam : m_graph->parameters())
        {
            if (param != otherParam && otherParam->name() == correctedName)
            {
                hasNameCollision = true;
                break;
            }
        }

        if (!hasNameCollision)
            break;

        index += 1;
    }

    {
        auto action = RefNew<ActionInplace>(TempString("Rename parameter {} -> {}", param->name(), correctedName), "Rename param"_id);

        auto obj = MaterialTemplateParamPtr(AddRef(param));

        action->doFunc = [this, obj, ui, correctedName]() {
            m_graph->renameParameter(obj, correctedName);
            ui->applyName(correctedName.view());
            return true;
        };

        action->undoFunc = [this, obj, ui, oldName]() {
            m_graph->renameParameter(obj, oldName);
            ui->applyName(oldName.view());
            return true;
        };

        return m_actionHistrory->execute(action);
    }
}

void MaterialParametersPanel::addParameter(SpecificClassType<IMaterialTemplateParam> paramType)
{
    const auto paramCoreName = paramType->name().view().afterLastOrFull("_");

    uint32_t index = 0;
    StringID paramName;
    for (;;)
    {
        paramName = StringID(TempString("{}{}", paramCoreName, index));
        if (!m_graph->findParameter(paramName))
            break;
        index += 1;
    }

    auto obj = paramType->create<IMaterialTemplateParam>();
    obj->rename(paramName);

    auto ui = createUI(obj, true);

    auto action = RefNew<ActionInplace>(TempString("Add parameter {}", paramName), "AddMaterialParam"_id);

    action->doFunc = [this, obj, ui]() {
        m_graph->attachParameter(obj);
        m_list->addItem(ui);
        return true;
    };

    action->undoFunc = [this, obj, ui]() {
        m_graph->detachParameter(obj);
        m_list->removeItem(ui);
        return true;
    };

    if (m_actionHistrory->execute(action))
    {
        m_list->select(ui);
        m_list->ensureVisible(ui);
    }
}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
