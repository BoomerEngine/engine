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

    void DataInspector::destroyItems()
    {
        for (const auto& group : m_items)
            detachChild(group);
        m_items.clear();
    }

    void DataInspector::createItems()
    {
        DEBUG_CHECK(m_items.empty());

        if (m_data)
        {
            // describe the root element
            base::rtti::DataViewInfo info;
            m_data->describe(0, m_rootPath, info);

            if (info.flags.test(base::rtti::DataViewInfoFlagBit::LikeStruct))
            {
                // enumerate properties
                info.requestFlags |= base::rtti::DataViewRequestFlagBit::MemberList;
                m_data->describe(0, m_rootPath, info);

                // TODO: conform property list to ones that are common in all objects

                // create property groups
                if (info.flags.test(base::rtti::DataViewInfoFlagBit::Object))
                {
                    base::HashSet<base::StringID> categoryNames;
                    for (const auto& propInfo : info.members)
                        categoryNames.insert(propInfo.category ? propInfo.category : "Default"_id);

                    auto names = categoryNames.keys();
                    std::sort(names.begin(), names.end(), [](base::StringID a, base::StringID b) { return a.view() < b.view(); });

                    for (auto name : names)
                    {
                        auto group = base::CreateSharedPtr<DataInspectorObjectCategoryGroup>(this, nullptr, m_rootPath, name);
                        attachChild(group);
                        m_items.pushBack(group);

                        group->expand();
                    }
                }
            }
        }
    }

    void DataInspector::detachData()
    {
        if (m_data)
        {
            if (m_rootObserverToken != nullptr)
            {
                m_data->unregisterObserver(m_rootObserverToken);
                m_rootObserverToken = nullptr;
            }

            m_data.reset();
        }
    }

    void DataInspector::attachData()
    {
        if (m_data)
        {
            DEBUG_CHECK(m_rootObserverToken == nullptr);
            m_rootObserverToken = m_data->registerObserver("", this);
        }
    }

    void DataInspector::bindData(base::DataProxy* data)
    {
        destroyItems();
        detachData();
        m_data = AddRef(data);
        attachData();
        createItems();
    }

    void DataInspector::bindObject(base::IObject* obj)
    {
        base::DataProxyPtr data;

        if (obj)
        {
            data = base::CreateSharedPtr<base::DataProxy>();
            data->add(base::CreateSharedPtr<base::DataViewNative>(obj));
        }

        bindData(data);
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

    void DataInspector::dataProxyFullObjectChange()
    {
        destroyItems();
        createItems();
    }

/*
    void DataInspector::bind(const base::view::DataBinding& bindings, base::edit::ActionHistory* history)
    {
        if (bindings.empty())
        {
            m_tree->removeAllRootNodes();
        }
        else
        {
            auto rootId = base::edit::DocumentObjectID::CreateFromPath("DataInspector", "DataInspector"_id);
            auto rootNode = base::CreateSharedPtr<model::DataObjectNode>(nullptr, bindings, history, true, rootId);
            m_tree->rootNode(rootNode);
        }
    }

    void DataInspector::bind(const base::edit::DocumentHandler* doc, const base::edit::DocumentObjectIDSet& ids, base::edit::ActionHistory* history)
    {
        // create data views from the selected objects
        base::InplaceArray<base::view::ViewPtr, 4> views;
        views.reserve(ids.size());
        if (nullptr != doc)
        {
            for (const auto& id : ids.ids())
            {
                auto view = doc->objectView(id);
                if (view)
                    views.pushBack(view);
            }
        }

        // bind the data grid to the data bindings
        if (views.empty())
        {
            base::view::DataBinding bindings;
            bind(bindings, history);
        }
        else
        {
            base::view::DataBinding bindings(views);
            bind(bindings, history);
        }
    }*/

    ///----

    /*DataInspectorSelectionHandler::DataInspectorSelectionHandler(const base::RefPtr<DataInspector>& DataInspector)
        : m_grid(DataInspector)
        , m_actionHistory(nullptr)
    {}

    DataInspectorSelectionHandler::~DataInspectorSelectionHandler()
    {}

    void DataInspectorSelectionHandler::bindActionHistory(base::edit::ActionHistory* actionHistory)
    {
        m_actionHistory = actionHistory;
    }

    void DataInspectorSelectionHandler::bindDefaultObjectView(const base::view::ViewPtr& defaultView)
    {
        m_defaultView = defaultView;

        if (m_defaultView)
        {
            base::view::DataBinding bindings(m_defaultView);
            m_grid->bind(bindings, m_actionHistory);
        }
    }

    void DataInspectorSelectionHandler::onSelectionChanged(const base::edit::SelectionPtr& newSelection)
    {
        // create data views from the selected objects
        base::Array<base::view::ViewPtr> views;
        if (newSelection)
        {
            views.reserve(newSelection->size());
            for (const auto& obj : newSelection->objects())
            {
                auto view = obj->createView();
                if (view)
                    views.pushBack(view);
            }
        }

        // bind the data grid to the data bindings
        if (views.empty())
        {
            base::view::DataBinding bindings(m_defaultView);
            m_grid->bind(bindings, m_actionHistory);
        }
        else
        {
            base::view::DataBinding bindings(views);
            m_grid->bind(bindings, m_actionHistory);
        }
    }*/

} // ui