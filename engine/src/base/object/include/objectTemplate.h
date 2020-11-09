/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once
#include "base/containers/include/hashSet.h"

namespace base
{
    //--

    /// object that is meant specifically to be used as a "template" to create other objects 
    /// Key difference is that object template can host "template" properties that are very easy to override since we explicitly store a flag for them if they are set or not
    class BASE_OBJECT_API IObjectTemplate : public IObject
    {
    public:
        IObjectTemplate();
        virtual ~IObjectTemplate();

        //--

        // get base object of this template
        // NOTE: the base object does not have to be exactly the same class, can be of parent class
        INLINE const ObjectTemplatePtr& base() const { return m_base; }

        // do we have any overridden properties ?
        INLINE bool hasAnyOverrides() const { return !m_overridenProperties.empty(); }

        //--

        // Get static object class, overloaded in child classes
        static SpecificClassType<IObjectTemplate> GetStaticClass();

        // Get object class, dynamic
        virtual ClassType nativeClass() const;

        //--

        // rebase this object template onto new base object, by default all non-overridden property values will also be replaced with the values from the base
        // NOTE: this will cause full object refresh in editor
        void rebase(const IObjectTemplate* base, bool copyValuesOfNonOverriddenProperties=true);

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
        virtual DataViewResult describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const override;

        /// Read data from memory
        virtual DataViewResult readDataView(StringView<char> viewPath, void* targetData, Type targetType) const override;

        /// Write data to memory
        virtual DataViewResult writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType) override;

        //--

        /// create editable data view
        virtual DataViewPtr createDataView(bool forceReadOnly=false) const override;

        //--

        // initialize class (since we have no automatic reflection in this project)
        static void RegisterType(rtti::TypeSystem& typeSystem);

    protected:
        virtual void onPropertyChanged(StringView<char> path) override;

        virtual bool onPropertyShouldLoad(const rtti::Property* prop) override;
        virtual bool onPropertyShouldSave(const rtti::Property* prop) const override;

    private:
        HashSet<StringID> m_overridenProperties; // properties that were historically changed via onPropertyModified
        int m_localSuppressOverridenPropertyCapture = 0;

        ObjectTemplatePtr m_base; // NOT SAVED - must be set manually every time object is loaded
    };

    ///--

} // base
