 /***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#pragma once

#include "uiScrollArea.h"
#include "base/object/include/dataView.h"

namespace ui
{
    ///---

    class DataInspectorNavigationItem;

    ///---

    /// sizing header
    class BASE_UI_API DataInspectorColumnHeader : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataInspectorColumnHeader, IElement);

    public:
        DataInspectorColumnHeader();

    private:
        virtual void prepareDynamicSizing(const ElementArea& drawArea, const ElementDynamicSizing*& dataPtr) const override;

        mutable base::Array<ElementDynamicSizing::ColumnInfo> m_cachedColumnAreas;
        mutable ElementDynamicSizing m_cachedSizingData;

        float m_splitFraction;
    };
  
    ///---

    /// data inspector, displays complex data structures using data boxes to edit the values
    class BASE_UI_API DataInspector : public ScrollArea, public base::IDataProxyObserver
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataInspector, ScrollArea);

    public:
        DataInspector();
        virtual ~DataInspector();

        // get the view we are editing
        INLINE const base::DataProxyPtr& data() const { return m_data; }

        // set the root view
        void bindData(base::DataProxy* data);

        // set the root view from an object
        void bindObject(base::IObject* obj);

    private:
        base::DataProxyPtr m_data;
        base::StringBuf m_rootPath;

        base::Array<base::RefPtr<DataInspectorNavigationItem>> m_items;

        base::RefPtr<DataInspectorColumnHeader> m_columnHeader;
        base::RefWeakPtr<DataInspectorNavigationItem> m_selectedItem;

        void* m_rootObserverToken = nullptr;

        //--

        DataInspectorNavigationItem* itemAtPoint(const Position& pos) const;

        void select(DataInspectorNavigationItem* item, bool focus=true);

        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
        virtual InputActionPtr handleOverlayMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;

        virtual void dataProxyFullObjectChange() override;

        void navigateItem(int delta);

        void destroyItems();
        void createItems();

        void detachData();
        void attachData();
    };

    ///---

    /// group of properties in the data inspector

    ///---

} // ui