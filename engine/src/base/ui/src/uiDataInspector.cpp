/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"

#include "uiDataBox.h"
#include "uiDataInspector.h"
#include "uiDataProperty.h"

#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiSplitter.h"
#include "uiGroup.h"
#include "uiElement.h"
#include "uiTextLabel.h"
#include "uiInputAction.h"

#include "base/input/include/inputStructures.h"
#include "base/object/include/rttiDataView.h"

namespace ui
{
    ///----

    struct DataGroup
    {
        base::RefPtr<DataInspectorGroup> m_group;
        base::StringBuf m_caption;
        int m_priority = 0;

        INLINE bool operator<(const DataGroup& g) const
        {
            if (m_priority != g.m_priority)
                return m_priority < g.m_priority;
            return m_caption < g.m_caption;
        }
    };

    ///----

    RTTI_BEGIN_TYPE_CLASS(DataInspectorColumnHeader);
        RTTI_METADATA(ElementClassNameMetadata).name("DataInspectorColumnHeader");
    RTTI_END_TYPE();

    DataInspectorColumnHeader::DataInspectorColumnHeader()
        : m_splitFraction(0.4f)
    {}

    void DataInspectorColumnHeader::prepareDynamicSizing(const ElementArea& drawArea, const ElementDynamicSizing*& dataPtr) const
    {
        // cache sizes
        m_cachedColumnAreas.reset();
        m_cachedColumnAreas.reserve(2);

        auto splitPos = base::Lerp(std::clamp<float>(m_splitFraction, 0.0f, 1.0f), drawArea.left(), drawArea.right());

        {
            auto& info = m_cachedColumnAreas.emplaceBack();
            info.m_left = drawArea.left();
            info.m_right = splitPos;
            info.m_center = true;
        }

        {
            auto& info = m_cachedColumnAreas.emplaceBack();
            info.m_left = splitPos;
            info.m_right = drawArea.right();
            info.m_center = true;
        }

        m_cachedSizingData.m_numColumns = m_cachedColumnAreas.size();
        m_cachedSizingData.m_columns = m_cachedColumnAreas.typedData();
    }

    ///----

    RTTI_BEGIN_TYPE_CLASS(DataInspector);
        RTTI_METADATA(ElementClassNameMetadata).name("DataInspector");
    RTTI_END_TYPE();

    DataInspector::DataInspector()
        : ScrollArea(ScrollMode::Auto)
    {
        hitTest(true);
        enableAutoExpand(true, true);
        allowFocusFromKeyboard(true);
        layoutVertical();

        m_columnHeader = createInternalChild<DataInspectorColumnHeader>();
    }

    DataInspector::~DataInspector()
    {
        destroyItems();
        detachData();
    }

    void DataInspector::settings(const DataInspectorSettings& settings)
    {
        m_settings = settings;
    }

    void DataInspector::destroyItems()
    {
        for (const auto& group : m_items)
            detachChild(group);
        m_items.clear();
    }

    static base::Array<base::StringID> GatherCategoryNames(bool sortNames, const base::rtti::DataViewInfo& info)
    {
        base::HashSet<base::StringID> categoryNames;
        for (const auto& propInfo : info.members)
            categoryNames.insert(propInfo.category ? propInfo.category : "Default"_id);

        auto names = categoryNames.keys();

        if (sortNames)
            std::sort(names.begin(), names.end(), [](base::StringID a, base::StringID b) { return a.view() < b.view(); });

        return names;
    }

    base::Array<base::StringID> GatherPropertyNames(bool sortNames, const base::rtti::DataViewInfo& info, base::StringID category)
    {
        base::HashSet<base::StringID> propertyNames;
        for (const auto& propInfo : info.members)
        {
            const auto propCategory = propInfo.category ? propInfo.category : "Default"_id;
            if (!category || propCategory == category)
                propertyNames.insert(propInfo.name);
        }

        auto names = propertyNames.keys();

        if (sortNames)
            std::sort(names.begin(), names.end(), [](base::StringID a, base::StringID b) { return a.view() < b.view(); });

        return names;
    }

    void DataInspector::createItems()
    {
        DEBUG_CHECK(m_items.empty());

        if (m_data)
        {
            // describe the root element
            base::rtti::DataViewInfo info;
            if (m_data->describeDataView(m_rootPath, info).valid())
            {
                if (info.flags.test(base::rtti::DataViewInfoFlagBit::LikeStruct))
                {
                    // enumerate properties
                    info.requestFlags |= base::rtti::DataViewRequestFlagBit::MemberList;
                    if (m_data->describeDataView(m_rootPath, info).valid())
                    {
                        if (m_settings.showCategories)
                        {
                            const auto names = GatherCategoryNames(m_settings.sortAlphabetically, info);
                            for (auto name : names)
                            {
                                auto group = base::RefNew<DataInspectorObjectCategoryGroup>(this, nullptr, m_rootPath, name);
                                attachChild(group);
                                m_items.pushBack(group);

                                group->expand();
                            }
                        }
                        else
                        {
                            const auto names = GatherPropertyNames(m_settings.sortAlphabetically, info, base::StringID::EMPTY());
                            for (auto name : names)
                            {
                                auto childPath = base::StringBuf(name.c_str());

                                base::rtti::DataViewInfo childInfo;
                                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::PropertyMetadata;
                                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::TypeMetadata;
                                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::MemberList;
                                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::CheckIfResetable;

                                if (data()->describeDataView(childPath, childInfo).valid())
                                {
                                    auto prop = base::RefNew<DataProperty>(this, nullptr, 0, childPath, childPath, childInfo, false);
                                    attachChild(prop);
                                    m_items.pushBack(prop);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void DataInspector::detachData()
    {
        if (m_data)
        {
            m_data->detachObserver("", this);
            m_data.reset();
        }
    }

    void DataInspector::attachData()
    {
        if (m_data)
            m_data->attachObserver("", this);
    }

    void DataInspector::bindActionHistory(base::ActionHistory* ah)
    {
        m_actionHistory = AddRef(ah);
    }

    void DataInspector::bindNull()
    {
        destroyItems();
        detachData();
        m_readOnly = false;
        m_data.reset();
    }

    void DataInspector::bindData(base::IDataView* data, bool readOnly)
    {
        destroyItems();
        detachData();
        m_readOnly = readOnly;
        m_data = AddRef(data);
        attachData();
        createItems();
    }

    void DataInspector::bindObject(base::IObject* obj, bool readOnly)
    {
        auto view = obj ? obj->createDataView() : nullptr;
        bindData(view, readOnly);
    }

    void DataInspector::select(DataInspectorNavigationItem* item, bool focus)
    {
        if (m_selectedItem != item)
        {
            if (auto cur = m_selectedItem.unsafe())
                cur->handleSelectionLost();
            m_selectedItem.reset();

            if (item)
            {
                m_selectedItem = item;
                item->handleSelectionGain(focus);

                if (!item->cachedDrawArea().empty())
                    scrollToMakeAreaVisible(item->cachedDrawArea());
            }
        }
    }

    DataInspectorNavigationItem* DataInspector::itemAtPoint(const Position& pos) const
    {
        for (const auto& item : m_items)
            if (auto ret = item->itemAtPosition(pos))
                return ret;

        return nullptr;
    }

    void DataInspector::navigateItem(int delta)
    {
        base::InplaceArray<DataInspectorNavigationItem*, 1024> allItems;
        for (const auto& item : m_items)
            item->collect(allItems);

        auto index = allItems.find(m_selectedItem.unsafe());
        if (index == -1)
        {
            if (delta < 0)
                index = 0;
            else
                index = allItems.lastValidIndex();
        }
        else
        {
            index += delta;
            if (index < 0) index = 0;
            else if (index > allItems.lastValidIndex()) index = allItems.lastValidIndex();
        }

        if (index >= 0 && index <= allItems.lastValidIndex())
            select(allItems[index], true);
    }

    bool DataInspector::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (evt.pressedOrRepeated())
        {
            if (evt.keyCode() == base::input::KeyCode::KEY_UP)
            {
                navigateItem(-1);
                return true;
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_DOWN)
            {
                navigateItem(1);
                return true;
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_PRIOR)
            {
                navigateItem(-10);
                return true;
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_NEXT)
            {
                navigateItem(10);
                return true;
            }
            else if (evt.keyCode() == base::input::KeyCode::KEY_RETURN)
            {
                if (auto * item = m_selectedItem.unsafe())
                {
                    if (item->expandable())
                    {
                        if (item->expanded())
                            item->collapse();
                        else
                            item->expand();

                        return true;
                    }
                }
            }
        }

        return TBaseClass::handleKeyEvent(evt);
    }

    InputActionPtr DataInspector::handleOverlayMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.leftClicked())
        {
            auto* item = itemAtPoint(evt.absolutePosition().toVector());
            select(item);
        }

        return TBaseClass::handleOverlayMouseClick(area, evt);
    }

    void DataInspector::handleFullObjectChange()
    {
        destroyItems();
        createItems();
    }

    void DataInspector::handlePropertyChanged(base::StringView fullPath, bool parentNotification)
    {
    }

    //---

} // ui