/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#include "build.h"
#include "uiClassPickerBox.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiWindow.h"
#include "uiImage.h"
#include "uiRenderer.h"
#include "uiTreeView.h"
#include "uiSearchBar.h"

#include "core/containers/include/stringBuilder.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

struct ClassInfo
{
    ClassType type;
    Array<ClassInfo*> children;
    ClassInfo* parent = nullptr;
};

struct ClassListing
{
    StructurePool<ClassInfo> pool;
    HashMap<ClassType, ClassInfo*> classMap;
    ClassInfo* rootClassInfo = nullptr;

    ~ClassListing()
    {
        for (auto* info : classMap.values())
            pool.free(info);
        classMap.clear();
    }

    void build(ClassType rootClass)
    {
        Array<ClassType> allClasses;
        RTTI::GetInstance().enumClasses(rootClass, allClasses, TypeSystem::TClassFilter(), true);

        for (const auto type : allClasses)
        {
            auto* info = pool.create();
            info->type = type;
            classMap[type] = info;

            if (type == rootClass)
                rootClassInfo = info;
        }

        for (auto* info : classMap.values())
        {
            if (auto baseClass = info->type->baseClass())
            {
                ClassInfo* baseInfo = nullptr;
                if (classMap.find(baseClass, baseInfo))
                {
                    baseInfo->children.pushBack(info);
                    info->parent = baseInfo;
                }
            }
        }
    }

    void addToTreeModel(const ClassInfo* info, ModelIndex parent, ClassTreeModel& outModel) const
    {
        ModelIndex index;

        if (!parent)
            index = outModel.addRootNode(info->type);
        else
            index = outModel.addChildNode(parent, info->type);

        for (const auto* childInfo : info->children)
            addToTreeModel(childInfo, index, outModel);
    }

    void addToTreeModel(ClassTreeModel& outModel) const
    {
        if (rootClassInfo)
        {
            addToTreeModel(rootClassInfo, ModelIndex(), outModel);
        }
        else
        {
            for (auto* info : classMap.values())
            {
                if (info->parent == nullptr)
                    addToTreeModel(info, ModelIndex(), outModel);
            }
        }
    }
};

ClassTreeModel::ClassTreeModel(ClassType rootClass)
{
    ClassListing listing;
    listing.build(rootClass);
    listing.addToTreeModel(*this);            
}

bool ClassTreeModel::compare(ClassType a, ClassType b, int colIndex) const
{
    return a->name().view() < b->name().view();
}

bool ClassTreeModel::filter(ClassType data, const SearchPattern& filter, int colIndex) const
{
    return filter.testString(data->name().view());
}

StringBuf ClassTreeModel::displayContent(ClassType data, int colIndex) const
{
    StringBuilder txt;
    if (data->isAbstract())
        txt << "[img:vs2012/object_interface][i] ";
    else
        txt << "[img:vs2012/object_class] ";
        
    txt << data->name();

    return txt.toString();
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(ClassPickerBox);
RTTI_END_TYPE();

//---

ClassPickerBox::ClassPickerBox(ClassType rootClass, ClassType initialType, bool allowAbstract, bool allowNull, StringView caption, bool showButtons)
    : PopupWindow(showButtons ? WindowFeatureFlagBit::DEFAULT_POPUP_DIALOG : WindowFeatureFlagBit::DEFAULT_TOOLTIP, "Select class")
    , m_allowAbstract(allowAbstract)
    , m_allowNull(allowNull)
    , m_hasButtons(showButtons)
{
    layoutVertical();

    // comment text
    if (!caption.empty())
    {
        auto captionLabel = createChild<TextLabel>(caption);
        captionLabel->customMargins(5);
    }

    // filter
    auto filter = createChild<SearchBar>();

    // list of types
    m_tree = createChild<TreeView>();
    m_tree->customInitialSize(500, 400);
    m_tree->expand();
    filter->bindItemView(m_tree);

    // buttons
    if (showButtons)
    {
        auto buttons = createChild();
        buttons->customPadding(5);
        buttons->layoutHorizontal();

        {
            auto button = buttons->createChildWithType<Button>("PushButton"_id, "[img:accept] Select");
            button->addStyleClass("green"_id);
            button->bind(EVENT_CLICKED) = [this]() { closeIfValidTypeSelected(); };
        }

        {
            auto button = buttons->createChildWithType<Button>("PushButton"_id, "Cancel");
            button->bind(EVENT_CLICKED) = [this]() { requestClose(); };
        }
    }

    // attach model
    m_treeModel = RefNew<ClassTreeModel>(rootClass);
    m_tree->model(m_treeModel);

    if (rootClass)
        m_tree->expandAll(false);

    if (const auto id = m_treeModel->findNodeForData(initialType))
    {
        m_tree->select(id);
        m_tree->ensureVisible(id);
    }

    m_tree->bind(EVENT_ITEM_ACTIVATED) = [this]()
    {
        closeIfValidTypeSelected();
        return true;
    };
}

bool ClassPickerBox::handleKeyEvent(const InputKeyEvent & evt)
{
    if (evt.pressed() && evt.keyCode() == InputKey::KEY_ESCAPE)
    {
        if (m_hasButtons)
        {
            requestClose();
            return true;
        }
    }
    else if (evt.pressed() && evt.keyCode() == InputKey::KEY_RETURN)
    {
        closeIfValidTypeSelected();
        return true;
    }

    return TBaseClass::handleKeyEvent(evt);
}

void ClassPickerBox::closeIfValidTypeSelected()
{
    if (m_tree)
    {
        auto selected = m_treeModel->dataForNode(m_tree->selectionRoot());

        if (!selected && !m_allowNull)
            return;

        if (!m_allowAbstract && (selected && selected->isAbstract()))
            return;

        closeWithType(selected);
    }
}

void ClassPickerBox::closeWithType(ClassType value)
{
    call(EVENT_CLASS_SELECTED, value);
    requestClose();
    closeParentPopup();
}

//---

END_BOOMER_NAMESPACE_EX(ui)

