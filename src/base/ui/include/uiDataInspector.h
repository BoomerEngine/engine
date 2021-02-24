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

BEGIN_BOOMER_NAMESPACE(ui)

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

/// display settings for the data inspector
struct BASE_UI_API DataInspectorSettings
{
    bool showCategories = true;
    bool sortAlphabetically = false;
    bool showAdvancedProperties = false;
    bool showOverriddenPropertiesOnly = false; // TODO

    base::StringBuf propertyNameFilter;
};
  
///---

/// data inspector, displays complex data structures using data boxes to edit the values
class BASE_UI_API DataInspector : public ScrollArea, public base::IDataViewObserver
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataInspector, ScrollArea);

public:
    DataInspector();
    virtual ~DataInspector();

    //--

    // get the view we are editing
    INLINE const base::DataViewPtr& data() const { return m_data; }

    // action history we are using to provide undo/redo for the data
    INLINE const base::ActionHistoryPtr& actionHistory() const { return m_actionHistory; }

    // get settings
    INLINE const DataInspectorSettings& settings() const { return m_settings; }

    // is the data inspector read only ?
    INLINE bool readOnly() const { return m_readOnly; }

    //--

    // bind null (remove any existing binding)
    void bindNull();

    // set the data view to show/edit in this inspector
    void bindData(base::IDataView* data, bool readOnly = false);

    // set the root view from an object, shorhand for bindData(obj->createDataView())
    void bindObject(base::IObject* obj, bool readOnly = false);

    // bind action history through which all undo/redo is performed
    void bindActionHistory(base::ActionHistory* ah);

    //--

    // change settings
    void settings(const DataInspectorSettings& settings);

private:
    base::DataViewPtr m_data;
    base::ActionHistoryPtr m_actionHistory;
    base::StringBuf m_rootPath;

    base::Array<base::RefPtr<DataInspectorNavigationItem>> m_items;

    base::RefPtr<DataInspectorColumnHeader> m_columnHeader;
    base::RefWeakPtr<DataInspectorNavigationItem> m_selectedItem;

    DataInspectorSettings m_settings;

    bool m_readOnly = false;

    //--

    DataInspectorNavigationItem* itemAtPoint(const Position& pos) const;

    void select(DataInspectorNavigationItem* item, bool focus=true);

    virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
    virtual InputActionPtr handleOverlayMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;

    virtual void handleFullObjectChange() override;
    virtual void handlePropertyChanged(base::StringView fullPath, bool parentNotification) override;

    void navigateItem(int delta);

    void destroyItems();
    void createItems();

    void detachData();
    void attachData();
};

///---

/// group of properties in the data inspector

///---

END_BOOMER_NAMESPACE(ui)