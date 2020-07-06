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

        // Get static object class, overloaded in child classes
        static SpecificClassType<IObjectTemplate> GetStaticClass();

        // Get object class, dynamic
        virtual ClassType cls() const;

        //--

        /// check if property is overridden
        bool hasPropertyOverride(StringID name) const;

        /// mark property was overridden or not
        void togglePropertyOverride(StringID name, bool flag);

        //--

        /// make this object into an override object, it affects how it edits and saves
        /// NOTE: this does not change anything in the data until the object is saved - when an override object is save than only the override properties are saved, the rest is not saved
        void toggleOverrideState(bool flag);

        //--

        // initialize class (since we have no automatic reflection in this project)
        static void RegisterType(rtti::TypeSystem& typeSystem);

        //--

        /// Get metadata for view - describe what we will find here: flags, list of members, size of array, etc
        virtual DataViewResult describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const override;

        /// Read data from memory
        virtual DataViewResult readDataView(StringView<char> viewPath, void* targetData, Type targetType) const override;

        /// Write data to memory
        virtual DataViewResult writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType) override;

        //--

    private:
        //static const auto NUM_WORDS = (MAX_TEMPLATE_PROPERTIES + 63) / 64;
        //uint64_t m_overridenProperties[NUM_WORDS]; // properties that were explicitly written

        HashSet<StringID> m_overridenProperties; // TODO: migrate to bit set

        bool m_override = false; // this object is indeed an override - only the properties with the "overridden" flag will be saved

        ObjectTemplatePtr m_editBase; // base object - just for editing
    };

    ///--

} // base
