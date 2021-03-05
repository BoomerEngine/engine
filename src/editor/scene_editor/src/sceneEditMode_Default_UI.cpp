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
#include "sceneEditMode_Default_Transform.h"

#include "sceneContentNodes.h"
#include "sceneContentStructure.h"
#include "sceneContentDataView.h"

#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiEditBox.h"
#include "engine/ui/include/uiListView.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiDragger.h"
#include "engine/ui/include/uiGroup.h"
#include "engine/ui/include/uiClassPickerBox.h"
#include "engine/ui/include/uiNotebook.h"
#include "engine/ui/include/uiElement.h"

#include "core/object/include/actionHistory.h"
#include "core/object/include/action.h"

#include "core/resource/include/indirectTemplate.h"
#include "core/resource/include/indirectTemplateCompiler.h"

#include "engine/world/include/nodeTemplate.h"
#include "engine/world/include/entityBehavior.h"
#include "engine/world/include/entity.h"
#include "engine/world/include/prefab.h"

#include "core/containers/include/stringBuilder.h"

#include "editor/common/include/editorService.h"
#include "editor/common/include/assetBrowser.h"
#include "editor/common/include/managedFile.h"
#include "editor/common/include/managedFileFormat.h"
#include "editor/common/include/managedDepot.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

static bool ValidNodeName(StringView name)
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

class SceneEditPrefabList : public ui::ListView
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneEditPrefabList, ui::ListView);

public:
    SceneEditPrefabList();

protected:
    virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;
    virtual void handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;
};

RTTI_BEGIN_TYPE_CLASS(SceneEditPrefabList);
RTTI_END_TYPE();
    
//--

struct PrefabInfoEntry
{
    uint32_t count = 0;
    StringView fileName;
    StringBuf path;
    ResourceID id;
    bool enabled = false;
    bool disabled = false;
    bool inherited = false;
    bool local = false;
};

class SceneEditPrefabListModel : public ui::SimpleTypedListModel<PrefabInfoEntry>
{
public:
    virtual bool compare(const PrefabInfoEntry& a, const PrefabInfoEntry& b, int colIndex) const override
    {
        return a.path < b.path;
    }

    virtual bool filter(const PrefabInfoEntry& data, const ui::SearchPattern& filter, int colIndex) const override
    {
        return filter.testString(data.fileName);
    }

    virtual StringBuf content(const PrefabInfoEntry& data, int colIndex = 0) const override
    {
        StringBuilder txt;

        if (data.enabled && data.disabled)
            txt << "[img:question_red] ";
        else if (data.enabled)
            txt << "[img:tick] ";
        else if (data.disabled)
            txt << "[img:cross] ";

        txt << "[img:brick] ";
        txt << data.fileName;

        if (data.inherited)
            txt << " [tag:#888]Inherited[/tag]";
        if (data.local)
            txt << " [tag:#888]Local[/tag]";

        return txt.toString();
    }
};

//--

SceneEditPrefabList::SceneEditPrefabList()
{}

ui::DragDropHandlerPtr SceneEditPrefabList::handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    if (auto fileData = rtti_cast<AssetBrowserFileDragDrop>(data))
        if (auto file = fileData->file())
            if (file->fileFormat().loadableAsType(Prefab::GetStaticClass()))
                return RefNew<ui::DragDropHandlerGeneric>(data, this, entryPosition);

    return TBaseClass::handleDragDrop(data, entryPosition);
}

void SceneEditPrefabList::handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    if (auto fileData = rtti_cast<AssetBrowserFileDragDrop>(data))
    {
        if (auto file = fileData->file())
        {
            if (file->fileFormat().loadableAsType(Prefab::GetStaticClass()))
            {
                call(EVENT_PREFABLIST_ADD_FILE, ManagedFilePtr(AddRef(file)));
            }
        }
    }
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

        m_prefabList = group->createChild<SceneEditPrefabList>();
        m_prefabList->expand();
        m_prefabList->customMinSize(0, 200);
        m_prefabList->customMaxSize(0, 200);

        m_prefabList->bind(EVENT_PREFABLIST_ADD_FILE) = [this](ManagedFilePtr file)
        {
            m_host->cmdAddPrefabFile(m_nodes, file);
        };

        auto panel = group->createChild<ui::IElement>();
        panel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        panel->layoutHorizontal();

        {
            auto button = panel->createChildWithType<ui::Button>("PushButton"_id, "[img:add] Add prefab");
            button->customProportion(1.0f);
            button->expand();
            button->bind(ui::EVENT_CLICKED) = [this]() {
                if (auto file = GetEditor()->selectedFile())
                    if (file->fileFormat().loadableAsType(Prefab::GetStaticClass()))
                        m_host->cmdAddPrefabFile(m_nodes, file);
            };
        }

        {
            auto button = panel->createChildWithType<ui::Button>("PushButton"_id, "[img:delete] Remove prefab");
            button->customProportion(1.0f);
            button->expand();
            button->bind(ui::EVENT_CLICKED) = [this]() {
                InplaceArray<const ManagedFile*, 10> files;
                for (auto item : m_prefabList->selection())
                    if (const auto* data = m_prefabListModel->dataPtr(item))
                        if (const auto* file = GetEditor()->managedDepot().findManagedFile(data->path.view()))
                            files.pushBack(file);

                m_host->cmdRemovePrefabFile(m_nodes, files);
            };
        }
    }

    {
        auto group = createChild<ui::Group>("Transform", true);
        m_dataElements.pushBack(group);

        auto tabs = group->createChild<ui::Notebook>();
        tabs->expand();
        tabs->customMargins(4, 4, 4, 4);

        {
            auto panel = RefNew<ui::IElement>();
            panel->customStyle<StringBuf>("title"_id, "[img:axis] Local");

            m_transformLocalValues = panel->createChild<SceneNodeTransformValuesBox>(GizmoSpace::Local, this);
            m_transformLocalValues->expand();

            tabs->attachTab(panel);
        }

        {
            auto panel = RefNew<ui::IElement>();
            panel->customStyle<StringBuf>("title"_id, "[img:world] World");

            m_transformWorldValues = panel->createChild<SceneNodeTransformValuesBox>(GizmoSpace::World, this);
            m_transformWorldValues->expand();

            tabs->attachTab(panel, nullptr, false);
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

    m_transformWorldValues->show(SceneNodeTransformEntry());
    m_transformLocalValues->show(SceneNodeTransformEntry());

    if (m_classPicker)
        m_classPicker->requestClose();
    m_classPicker.reset();

    m_prefabListModel.reset();
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
    m_nodesData.reset();
    m_nodesData.reserve(m_nodes.size());

    for (auto node : m_nodes)
        if (auto dataNode = rtti_cast<SceneContentDataNode>(node))
            m_nodesData.pushBack(dataNode);

    m_nodeRoots.reset();
    SceneEditMode_Default::ExtractSelectionRoots(m_nodes, m_nodeRoots);

    m_nodeEntityRoots.reset();
    m_nodeEntityRoots.reserve(m_nodeRoots.size());

    for (const auto& ptr : m_nodeRoots)
        if (auto entity = rtti_cast<SceneContentEntityNode>(ptr))
            m_nodeEntityRoots.pushBack(entity);

    //--

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
    refreshTransforms();
    refreshPrefabList();
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
    bool hasBehaviorData = false;

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
                    
                if (dataNode->is<SceneContentEntityNode>())
                    hasEntityData = true;
                else if (dataNode->is<SceneContentBehaviorNode>())
                    hasBehaviorData = true;
            }
        }
    }

    // do not edit anything if both entities and components are selected at the same time
    if (hasEntityData && hasBehaviorData)
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
            const auto dataClass = data.editableData->templateClass();
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

        for (const auto& obj : editableObjects)
        {
            if (auto view = obj.createDataView())
            {
                m_properties->bindData(view, false);
                break;
            }
        }

        if (commonClassInconsistent)
        {
            m_partClass->text("<inconsistent class>");
            m_buttonChangeClass->enable(false);
        }
        else if (commonClass)
        {
            m_commonClassType = commonClass;
            m_partClass->text(commonClass->name().view());
            m_buttonChangeClass->enable(true);
        }
        else
        {
            m_commonClassType = commonClass;
            m_partClass->text("<no class>");
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
        auto action = RefNew<ActionRenameNodes>(std::move(infos), this);
        m_host->actionHistory()->execute(action);
    }
}

//--

struct ChangeClassActionInfo
{
    SceneContentDataNodePtr node;
    ClassType oldClass;
    ClassType newClass;
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
            info.node->changeClass(info.newClass);
        m_host->refreshProperties();
        return true;
    }

    virtual bool undo() override
    {
        for (const auto& info : m_nodes)
            info.node->changeClass(info.oldClass);
        m_host->refreshProperties();
        return true;
    }

private:
    Array<ChangeClassActionInfo> m_nodes;
    SceneDefaultPropertyInspectorPanel* m_host;
};

static bool IsClassCompatible(const SceneContentDataNodePtr& dataNode, ClassType newDataClass)
{
    if (dataNode->is<SceneContentEntityNode>())
        return newDataClass->is<Entity>();

    else if (dataNode->is<SceneContentBehaviorNode>())
        return newDataClass->is<IEntityBehavior>();

    return false;
}

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
                info.oldClass = dataNode->dataClass();
                info.newClass = newDataClass;
            }
        }
    }

    if (!nodes.empty())
    {
        auto action = RefNew<ActionChangeDataClass>(std::move(nodes), this);
        m_host->actionHistory()->execute(action);
    }
}

static ClassType SelectRootClass(ClassType currentClass)
{
    if (currentClass->is<Entity>())
        return Entity::GetStaticClass();

    else if (currentClass->is<IEntityBehavior>())
        return IEntityBehavior::GetStaticClass();

    return nullptr;
}

void SceneDefaultPropertyInspectorPanel::cmdChangeClass()
{
    if (!m_classPicker)
    {
        auto rootClass = SelectRootClass(m_commonClassType);

        if (!rootClass && !m_nodes.empty())
        {
            bool allowEntityClass = true;

            for (const auto& node : m_nodes)
            {
                if (!node->is<SceneContentEntityNode>())
                {
                    allowEntityClass = false;
                    break;
                }
            }

            if (allowEntityClass)
                rootClass = Entity::GetStaticClass();
        }

        if (rootClass)
        {
            m_classPicker = RefNew<ui::ClassPickerBox>(rootClass, m_commonClassType, false, false);

            m_classPicker->bind(ui::EVENT_WINDOW_CLOSED) = [this]() {
                m_classPicker.reset();
            };

            m_classPicker->bind(ui::EVENT_CLASS_SELECTED) = [this](ClassType data) {
                changeDataClass(data, m_nodes);
            };

            m_classPicker->show(this, ui::PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));
        }
    }
}

//--

SceneNodeTransformEntry::SceneNodeTransformEntry()
{
    clear();
}

SceneNodeTransformEntry::SceneNodeTransformEntry(const SceneNodeTransformEntry& other)
{
    for (int i = 0; i < NUM_ENTRIES; ++i)
        values[i] = other.values[i];
}

SceneNodeTransformEntry& SceneNodeTransformEntry::operator=(const SceneNodeTransformEntry& other)
{
    if (this != &other)
        for (int i = 0; i < NUM_ENTRIES; ++i)
            values[i] = other.values[i];

    return *this;
}

void SceneNodeTransformEntry::clear()
{
    for (int i = 0; i < NUM_ENTRIES; ++i)
    {
        values[i] = SceneNodeTransformEntryValue();
        values[i].index = i;
        values[i].flag = FIELD_BITS[i];
    }
}

SceneNodeTransformValueFileldBit SceneNodeTransformEntry::FIELD_BITS[NUM_ENTRIES] =
{
    SceneNodeTransformValueFileldBit::TranslationX,
    SceneNodeTransformValueFileldBit::TranslationY,
    SceneNodeTransformValueFileldBit::TranslationZ,
    SceneNodeTransformValueFileldBit::RotationX,
    SceneNodeTransformValueFileldBit::RotationY,
    SceneNodeTransformValueFileldBit::RotationZ,
    SceneNodeTransformValueFileldBit::ScaleX,
    SceneNodeTransformValueFileldBit::ScaleY,
    SceneNodeTransformValueFileldBit::ScaleZ,
};

void SceneNodeTransformEntry::setup(const EulerTransform& transform, SceneNodeTransformValueFileldFlags determined /*= SceneNodeTransformValueFileldBit::ALL*/, SceneNodeTransformValueFileldFlags enabled /*= SceneNodeTransformValueFileldBit::ALL*/)
{
    for (int i = 0; i < NUM_ENTRIES; ++i)
    {
        const auto flag = FIELD_BITS[i];
        values[i].determined = determined.test(flag);
        values[i].enabled = enabled.test(flag);
    }

    values[(int)SceneNodeTransformValueFieldType::TranslationX].value = transform.T.x;
    values[(int)SceneNodeTransformValueFieldType::TranslationY].value = transform.T.y;
    values[(int)SceneNodeTransformValueFieldType::TranslationZ].value = transform.T.z;
    values[(int)SceneNodeTransformValueFieldType::RotationX].value = transform.R.roll;
    values[(int)SceneNodeTransformValueFieldType::RotationY].value = transform.R.pitch;
    values[(int)SceneNodeTransformValueFieldType::RotationZ].value = transform.R.yaw;
    values[(int)SceneNodeTransformValueFieldType::ScaleX].value = transform.S.x;
    values[(int)SceneNodeTransformValueFieldType::ScaleY].value = transform.S.y;
    values[(int)SceneNodeTransformValueFieldType::ScaleZ].value = transform.S.z;
}

//--

ISceneNodeTransformValuesBoxEventSink::~ISceneNodeTransformValuesBoxEventSink()
{}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneNodeTransformValuesBox);
    RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxVector"); // stolen style...
RTTI_END_TYPE();

SceneNodeTransformValuesBox::SceneNodeTransformValuesBox(GizmoSpace space, ISceneNodeTransformValuesBoxEventSink* sink)
    : m_space(space)
    , m_sink(sink)
{
    layoutVertical();

    {
        auto group = createChildWithType<ui::IElement>("SceneTransformGroup"_id);
        group->layoutVertical();

        auto header = group->createChild<ui::IElement>();
        header->layoutHorizontal();
        header->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        header->createNamedChild<ui::TextLabel>("GroupCaption"_id, "[img:transform_move] Translation");

        auto reset = header->createChildWithType<ui::Button>("DataPropertyButton"_id, "[img:undo] Reset");
        reset->customMargins(5, 0, 5, 0);
        reset->bind(ui::EVENT_CLICKED) = [this]() {
            m_sink->transformBox_ValueReset(m_space, SceneNodeTransformValueFieldType::TranslationX);
        };

        auto row = group->createChild<ui::IElement>();
        row->layoutHorizontal();
        row->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_elems[0] = createElem(row, "X"_id, "X", SceneNodeTransformValueFieldType::TranslationX);
        m_elems[1] = createElem(row, "Y"_id, "Y", SceneNodeTransformValueFieldType::TranslationY);
        m_elems[2] = createElem(row, "Z"_id, "Z", SceneNodeTransformValueFieldType::TranslationZ);
    }

    {
        auto group = createChildWithType<ui::IElement>("SceneTransformGroup"_id);
        group->layoutVertical();

        auto header = group->createChild<ui::IElement>();
        header->layoutHorizontal();
        header->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        header->createNamedChild<ui::TextLabel>("GroupCaption"_id, "[img:transform_rotate] Rotation");

        auto reset = header->createChildWithType<ui::Button>("DataPropertyButton"_id, "[img:undo] Reset");
        reset->customMargins(5, 0, 5, 0);
        reset->bind(ui::EVENT_CLICKED) = [this]() {
            m_sink->transformBox_ValueReset(m_space, SceneNodeTransformValueFieldType::RotationX);
        };

        auto row = group->createChild<ui::IElement>();
        row->layoutHorizontal();
        row->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_elems[3] = createElem(row, "X"_id, "R", SceneNodeTransformValueFieldType::RotationX);
        m_elems[4] = createElem(row, "Y"_id, "P", SceneNodeTransformValueFieldType::RotationY);
        m_elems[5] = createElem(row, "Z"_id, "Y", SceneNodeTransformValueFieldType::RotationZ);
    }

    {
        auto group = createChildWithType<ui::IElement>("SceneTransformGroup"_id);
        group->layoutVertical();

        auto header = group->createChild<ui::IElement>();
        header->layoutHorizontal();
        header->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        header->createNamedChild<ui::TextLabel>("GroupCaption"_id, "[img:transform_scale] Scale");

        auto reset = header->createChildWithType<ui::Button>("DataPropertyButton"_id, "[img:undo] Reset");
        reset->customMargins(5, 0, 5, 0);
        reset->bind(ui::EVENT_CLICKED) = [this]() {
            m_sink->transformBox_ValueReset(m_space, SceneNodeTransformValueFieldType::ScaleX);
        };

        auto row = group->createChild<ui::IElement>();
        row->layoutHorizontal();
        row->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_elems[6] = createElem(row, "X"_id, "X", SceneNodeTransformValueFieldType::ScaleX);
        m_elems[7] = createElem(row, "Y"_id, "Y", SceneNodeTransformValueFieldType::ScaleY);
        m_elems[8] = createElem(row, "Z"_id, "Z", SceneNodeTransformValueFieldType::ScaleZ);
    }
}

SceneNodeTransformValuesBox::Elem SceneNodeTransformValuesBox::createElem(ui::IElement* row, StringID style, StringView caption, SceneNodeTransformValueFieldType field)
{
    SceneNodeTransformValuesBox::Elem ret;

    auto label = row->createNamedChild<ui::TextLabel>(style, caption);
    label->customMargins(8.0f, 0.0f, 8.0f, 0.0f);

    auto textFlags = { ui::EditBoxFeatureBit::AcceptsEnter, ui::EditBoxFeatureBit::NoBorder };
    ret.box = row->createChild<ui::EditBox>(textFlags);
    ret.box->expand();
    ret.box->customMaxSize(120.0f, 0.0f);
    ret.box->customProportion(1.0f);
    ret.box->bind(ui::EVENT_TEXT_ACCEPTED) = [this, field](ui::EditBox* box)
    {
        double value = 0.0;
        if (box->text().view().match(value) == MatchResult::OK)
        {
            m_sink->transformBox_ValueChanged(m_space, field, value);
        }
    };

    ret.drag = row->createChild<ui::Dragger>();
    ret.drag->customHorizontalAligment(ui::ElementHorizontalLayout::Right);
    ret.drag->customVerticalAligment(ui::ElementVerticalLayout::Expand);
    ret.drag->bind(ui::EVENT_VALUE_DRAG_STARTED) = [this, field]()
    {
        m_sink->transformBox_ValueDragStart(m_space, field);
    };

    ret.drag->bind(ui::EVENT_VALUE_DRAG_STEP) = [this, field](int val)
    {
        m_sink->transformBox_ValueDragUpdate(m_space, field, val);
    };

    ret.drag->bind(ui::EVENT_VALUE_DRAG_CANCELED) = [this, field]()
    {
        m_sink->transformBox_ValueDragCancel(m_space, field);
    };

    ret.drag->bind(ui::EVENT_VALUE_DRAG_FINISHED) = [this, field]()
    {
        m_sink->transformBox_ValueDragEnd(m_space, field);
    };

    return ret;
}

void SceneNodeTransformValuesBox::show(const SceneNodeTransformEntry& data)
{
    for (const auto& field : data.values)
    {
        DEBUG_CHECK(field.index >= 0 && field.index < SceneNodeTransformEntry::NUM_ENTRIES);

        if (field.index >= 0 && field.index < SceneNodeTransformEntry::NUM_ENTRIES)
        {
            auto& elem = m_elems[field.index];
            if (!field.enabled)
            {
                elem.box->enable(false);
                elem.box->text("<unused>");
                elem.drag->visibility(false);
            }
            else if (field.determined)
            {
                elem.box->enable(true);
                elem.box->text(TempString("{}", Prec(field.value, 3)));
                elem.drag->visibility(true);
            }
            else
            {
                elem.box->enable(true);
                elem.box->text("");
                elem.drag->visibility(true);
            }
        }
    }
}

//--

struct ValueAggregator
{
    double value = 0.0;
    bool determined = false;
    uint32_t count = 0;

    void collect(double val)
    {
        if (count == 0)
        {
            value = val;
            determined = true;
        }
        else if (determined)
        {
            if (val != value)
                determined = false;
        }

        count += 1;
    }
};

struct TransformAggregator
{
    ValueAggregator values[SceneNodeTransformEntry::NUM_ENTRIES];

    void collect(const EulerTransform& data)
    {
        values[(int)SceneNodeTransformValueFieldType::TranslationX].collect(data.T.x);
        values[(int)SceneNodeTransformValueFieldType::TranslationY].collect(data.T.y);
        values[(int)SceneNodeTransformValueFieldType::TranslationZ].collect(data.T.z);

        values[(int)SceneNodeTransformValueFieldType::RotationX].collect(data.R.roll);
        values[(int)SceneNodeTransformValueFieldType::RotationY].collect(data.R.pitch);
        values[(int)SceneNodeTransformValueFieldType::RotationZ].collect(data.R.yaw);

        values[(int)SceneNodeTransformValueFieldType::ScaleX].collect(data.S.x);
        values[(int)SceneNodeTransformValueFieldType::ScaleY].collect(data.S.y);
        values[(int)SceneNodeTransformValueFieldType::ScaleZ].collect(data.S.z);
    }

    void apply(SceneNodeTransformEntry& outEntry) const
    {
        outEntry.clear();

        for (uint32_t i = 0; i < SceneNodeTransformEntry::NUM_ENTRIES; ++i)
        {
            const auto& src = values[i];
            auto& dest = outEntry.values[i];

            dest.determined = src.determined;
            dest.value = src.value;
            dest.enabled = src.count > 0;
        }
    }
};

struct AbsoluteTransformAggregator
{
    ValueAggregator values[SceneNodeTransformEntry::NUM_ENTRIES];

    void collect(const AbsoluteTransform& data)
    {
        {
            double x, y, z;
            data.position().expand(x, y, z);
            values[(int)SceneNodeTransformValueFieldType::TranslationX].collect(x);
            values[(int)SceneNodeTransformValueFieldType::TranslationY].collect(y);
            values[(int)SceneNodeTransformValueFieldType::TranslationZ].collect(z);
        }

        {
            auto rotation = data.rotation().toRotator();
            values[(int)SceneNodeTransformValueFieldType::RotationX].collect(rotation.roll);
            values[(int)SceneNodeTransformValueFieldType::RotationY].collect(rotation.pitch);
            values[(int)SceneNodeTransformValueFieldType::RotationZ].collect(rotation.yaw);
        }

        values[(int)SceneNodeTransformValueFieldType::ScaleX].collect(data.scale().x);
        values[(int)SceneNodeTransformValueFieldType::ScaleY].collect(data.scale().y);
        values[(int)SceneNodeTransformValueFieldType::ScaleZ].collect(data.scale().z);
    }

    void apply(SceneNodeTransformEntry& outEntry) const
    {
        outEntry.clear();

        for (uint32_t i = 0; i < SceneNodeTransformEntry::NUM_ENTRIES; ++i)
        {
            const auto& src = values[i];
            auto& dest = outEntry.values[i];

            dest.determined = src.determined;
            dest.value = src.value;
            dest.enabled = src.count > 0;
        }
    }
};

void SceneDefaultPropertyInspectorPanel::refreshTransforms()
{
    TransformAggregator localTransform;
    AbsoluteTransformAggregator worldTransform;

    for (const auto& node : m_nodes)
    {
        if (auto dataNode = rtti_cast<SceneContentDataNode>(node))
        {
            localTransform.collect(dataNode->localToParent());
            worldTransform.collect(dataNode->cachedLocalToWorldTransform());
        }
    }

    {
        SceneNodeTransformEntry values;
        localTransform.apply(values);
        m_transformLocalValues->show(values);
    }

    {
        SceneNodeTransformEntry values;
        worldTransform.apply(values);
        m_transformWorldValues->show(values);
    }
}

//--

void SceneDefaultPropertyInspectorPanel::cancelTransformAction()
{
    if (m_currentDragTransform)
    {
        m_currentDragTransform->cancel();
        m_currentDragTransform.reset();
    }
}

static double DefaultDisplacementForFieldType(SceneNodeTransformValueFieldType field)
{
    switch (field)
    {
        case SceneNodeTransformValueFieldType::TranslationX:
        case SceneNodeTransformValueFieldType::TranslationY:
        case SceneNodeTransformValueFieldType::TranslationZ:
            return 0.01;

        case SceneNodeTransformValueFieldType::RotationX:
        case SceneNodeTransformValueFieldType::RotationY:
        case SceneNodeTransformValueFieldType::RotationZ:
            return 0.1;

        case SceneNodeTransformValueFieldType::ScaleX:
        case SceneNodeTransformValueFieldType::ScaleY:
        case SceneNodeTransformValueFieldType::ScaleZ:
            return 0.001;
    }

    return 0.001;
}

void SceneDefaultPropertyInspectorPanel::transformBox_ValueDragStart(GizmoSpace space, SceneNodeTransformValueFieldType field)
{
    cancelTransformAction();

    const auto step = DefaultDisplacementForFieldType(field);

    // if we are in the whole hierarchy mode though we will need to update transform for the rest of the nodes as well
    HashSet<SceneContentDataNode*> coreSelectionSet;
    if (SceneGizmoTarget::WholeHierarchy == m_host->container()->gizmoSettings().target)
    {
        if (space == GizmoSpace::Local)
            m_currentDragTransform = RefNew<SceneEditModeDefaultTransformDragger>(m_host, m_nodesData, m_host->container()->gridSettings(), space, field, step);
        else
            m_currentDragTransform = RefNew<SceneEditModeDefaultTransformDragger>(m_host, m_nodeRoots, m_host->container()->gridSettings(), space, field, step);
    }
    else
    {
        //m_currentDragTransform = RefNew<SceneEditModeDefaultTransformDragger>(m_host, dragNodes, m_host->container()->gridSettings(), space, field, step);
    }
}

void SceneDefaultPropertyInspectorPanel::transformBox_ValueDragUpdate(GizmoSpace space, SceneNodeTransformValueFieldType field, int step)
{
    if (m_currentDragTransform)
    {
        m_currentDragTransform->step(step);
        m_host->handleTransformsChanged();
    }
}

void SceneDefaultPropertyInspectorPanel::transformBox_ValueDragEnd(GizmoSpace space, SceneNodeTransformValueFieldType field)
{
    if (m_currentDragTransform)
    {
        if (auto action = m_currentDragTransform->createAction())
            m_host->actionHistory()->execute(action);

        m_currentDragTransform.reset();
    }
}

void SceneDefaultPropertyInspectorPanel::transformBox_ValueDragCancel(GizmoSpace space, SceneNodeTransformValueFieldType field)
{
    cancelTransformAction();
}

//--

void SceneDefaultPropertyInspectorPanel::transformBox_ValueReset(GizmoSpace space, SceneNodeTransformValueFieldType field)
{
    Array<ActionMoveSceneNodeData> undoData;

    // change transforms of the nodes in the selections
    if (space == GizmoSpace::Local)
    {
        undoData.reserve(m_nodes.size());
        for (const auto& node : m_nodes)
        {
            if (auto dataNode = rtti_cast<SceneContentDataNode>(node))
            {
                auto& data = undoData.emplaceBack();
                data.node = node;
                data.oldTransform = dataNode->localToParent();

                auto localTransform = dataNode->localToParent();
                ResetLocalTransformField(localTransform, field);

                data.newTransform = localTransform;
            }
        }
    }
    else if (space == GizmoSpace::World)
    {
        undoData.reserve(m_nodeRoots.size());
        for (const auto& node : m_nodeRoots)
        {
            auto& data = undoData.emplaceBack();
            data.node = node;
            data.oldTransform = node->localToParent();

            auto worldTransform = node->cachedLocalToWorldTransform();
            ResetWorldTransformField(worldTransform, field);
            data.newTransform = (worldTransform / node->cachedParentToWorldTransform()).toEulerTransform();
        }
    }

    //expandTransformValuesWithHierarchyChildren(dragNodes, undoData);

    if (!undoData.empty())
    {
        auto action = CreateSceneNodeTransformAction(std::move(undoData), m_host, true);
        m_host->actionHistory()->execute(action);
    }
}

/*void SceneDefaultPropertyInspectorPanel::expandTransformValuesWithHierarchyChildren(const Array<SceneContentDataNodePtr>& dragNodes, Array<ActionMoveSceneNodeData>& undoData) const
{
    // if moving the whole hierarchy make sure we update also the child nodes
    if (SceneGizmoTarget::WholeHierarchy == m_host->container()->gizmoSettings().target)
    {
        HashMap<SceneContentNode*, int> undoActionMap;
        undoActionMap.reserve(dragNodes.size());

        for (auto i : dragNodes.indexRange())
            undoActionMap[dragNodes[i]] = i;

        InplaceArray<SceneContentDataNodePtr, 64> additionalNodes;
        for (const auto& root : dragNodes)
        {
            additionalNodes.reset();
            SceneEditMode_Default::ExtractSelectionHierarchyWithFilter2(root, additionalNodes, undoActionMap);

            for (const auto& node : additionalNodes)
            {
                int parentUndoActionIndex = -1;
                if (undoActionMap.find(node->parent(), parentUndoActionIndex))
                {
                    auto& data = undoData.emplaceBack();
                    data.node = node;
                    data.oldTransform = node->localToParent();

                    const auto& newParentTransform = undoData[parentUndoActionIndex].newTransform;
                    data.newTransform = newParentTransform * node->calcLocalToParent();
                }
                else
                {
                    DEBUG_CHECK(parentUndoActionIndex != -1);
                }
            }
        }
    }
}*/

void SceneDefaultPropertyInspectorPanel::transformBox_ValueChanged(GizmoSpace space, SceneNodeTransformValueFieldType field, double value)
{
    // change transforms of the nodes in the selections
    Array<ActionMoveSceneNodeData> undoData;
    if (space == GizmoSpace::Local)
    {
        undoData.reserve(m_nodes.size());
        for (const auto& node : m_nodes)
        {
            if (auto dataNode = rtti_cast<SceneContentDataNode>(node))
            {
                auto& data = undoData.emplaceBack();
                data.node = node;
                data.oldTransform = dataNode->localToParent();

                auto localTransform = dataNode->localToParent();
                ApplyLocalTransformField(localTransform, field, value, false);

                data.newTransform = localTransform;
            }
        }
    }
    else if (space == GizmoSpace::World)
    {
        undoData.reserve(m_nodeRoots.size());
        for (const auto& node : m_nodeRoots)
        {
            auto& data = undoData.emplaceBack();
            data.node = node;
            data.oldTransform = node->localToParent();

            auto worldTransform = node->cachedLocalToWorldTransform();
            ApplyWorldTransformField(worldTransform, field, value, false);

            data.newTransform = (worldTransform / node->cachedParentToWorldTransform()).toEulerTransform();
        }
    }

    //expandTransformValuesWithHierarchyChildren(dragNodes, undoData);

    if (!undoData.empty())
    {
        auto action = CreateSceneNodeTransformAction(std::move(undoData), m_host, true);
        m_host->actionHistory()->execute(action);
    }
}

void SceneDefaultPropertyInspectorPanel::refreshPrefabList()
{
    // collect used prefabs
    HashMap<StringBuf, PrefabInfoEntry> prefabEntries;
    for (const auto& node : m_nodes)
    {
        if (auto entityNode = rtti_cast<SceneContentEntityNode>(node))
        {
            HashSet<StringBuf> localPrefabEntries;
            for (const auto& info : entityNode->localPrefabs())
            {
                StringBuf path;
                if (GetService<DepotService>()->resolvePathForID(info.prefab.id(), path))
                {
                    auto& entry = prefabEntries[path];

                    if (!entry.path)
                    {
                        entry.path = path;
                        entry.id = info.prefab.id();
                        entry.fileName = path.view().fileStem();
                    }

                    if (localPrefabEntries.insert(path))
                        entry.count += 1;

                    if (info.enabled)
                        entry.enabled = true;
                    else
                        entry.disabled = true;
                }
            }
        }
    }

    // display prefab list
    m_prefabListModel = RefNew<SceneEditPrefabListModel>();
    for (const auto& pair : prefabEntries.pairs())
        if (pair.value.count == m_nodes.size()) // exists in all nodes
            m_prefabListModel->add(pair.value);

    // show list
    m_prefabList->model(m_prefabListModel);
}

//--
    
END_BOOMER_NAMESPACE_EX(ed)
