/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once
#include "core/containers/include/hashSet.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// Object that is meant specifically to be used as a "template" to create other objects without changing class
/// Properties that have meaningful (assigned) values are stored in the hash set and only they are saved
class CORE_RESOURCE_API IObjectDirectTemplate : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IObjectDirectTemplate, IObject);

public:
    IObjectDirectTemplate();
    virtual ~IObjectDirectTemplate();

    //--

    // get base object of this template
    // NOTE: the base object does not have to be exactly the same class, can be of parent class
    INLINE const ObjectDirectTemplatePtr& base() const { return m_base; }

    // do we have any overridden properties ?
    INLINE bool hasAnyOverrides() const { return !m_overridenProperties.empty(); }

    //--

    // rebase this object template onto new base object, by default all non-overridden property values will also be replaced with the values from the base
    // NOTE: this will cause full object refresh in editor
    void rebase(const IObjectDirectTemplate* base, bool copyValuesOfNonOverriddenProperties=true);

    // detach base object - basically makes all set values marked as "overridden"
    void detach(bool mergeOverrideTables = true);

    //--

    /// check if property can have the override semantic
    bool checkPropertyOverrideCaps(StringID name) const;

    /// check if property is overridden
    bool hasPropertyOverride(StringID name) const;

    /// reset property, returns true if property was reset
    bool resetPropertyOverride(StringID name);

    /// mark property as having an override
    void markPropertyOverride(StringID name);

    //--

    /// Get metadata for view - describe what we will find here: flags, list of members, size of array, etc
    virtual DataViewResult describeDataView(StringView viewPath, rtti::DataViewInfo& outInfo) const override;

    /// Read data from memory
    virtual DataViewResult readDataView(StringView viewPath, void* targetData, Type targetType) const override;

    /// Write data to memory
    virtual DataViewResult writeDataView(StringView viewPath, const void* sourceData, Type sourceType) override;

    //--

    /// create editable data view
    virtual DataViewPtr createDataView(bool forceReadOnly=false) const override;

    //--

protected:
    virtual void onPropertyChanged(StringView path) override;

    virtual bool onPropertyShouldLoad(const rtti::Property* prop) override;
    virtual bool onPropertyShouldSave(const rtti::Property* prop) const override;

private:
    HashSet<StringID> m_overridenProperties; // properties that were historically changed via onPropertyModified
    int m_localSuppressOverridenPropertyCapture = 0;

    ObjectDirectTemplatePtr m_base; // NOT SAVED - must be set manually every time object is loaded
};

///--

END_BOOMER_NAMESPACE()
