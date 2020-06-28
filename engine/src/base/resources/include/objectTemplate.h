/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: template #]
***/

#pragma once

#include "base/object/include/object.h"
#include "base/reflection/include/variant.h"

namespace base
{

    //--

    // parameter of the templated object
    class BASE_RESOURCES_API ObjectTemplateParam
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(ObjectTemplateParam);

    public:
        ObjectTemplateParam();

        StringID m_name;
        Variant m_value;
    };

    //--

    struct CachedTemplatePropertyInfo
    {
        StringID m_name;
        StringID m_category;
        Type m_type = nullptr;
        Variant  m_defaultValue;
    };

    struct CachedTemplatePropertyList : public IReferencable
    {
        Array<CachedTemplatePropertyInfo> m_properties;
        ClassType m_class = nullptr;

        const CachedTemplatePropertyInfo* findProperty(base::StringID name) const;
    };

    //--

    /// a template of any object
    /// can be used to create (by stacking layers with properties) any object of a class known to the RTTI
    /// NOTE: although creating an instance is not blazingly fast it's still much much faster than any kind of clone()
    class BASE_RESOURCES_API ObjectTemplate : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ObjectTemplate, IObject);

    public:
        ObjectTemplate();
        virtual ~ObjectTemplate();

        //--

        /// get the parameters defined in this template
        /// NOTE: the stored values are for UNRESOLVED properties
        INLINE const Array<ObjectTemplateParam>& parameters() const { return m_parameters; }

        /// get the editor only base template
        INLINE const ObjectTemplate* editorOnlyBaseTemplate() const { return m_editorOnlyBaseTemplate; }

        //--

        // reset value of parameter to a default value
        bool resetParameter(StringID name);

        // get value of parameter
        bool readParameter(StringID name, Variant& outData) const;

        // write value of parameter
        bool writeParameter(StringID name, const Variant& newData);

        //--

        // get the root class that we must implement
        virtual ClassType rootTemplateClass() const = 0;

        // get the object class we should create
        virtual ClassType objectClass() const = 0;

        //--

    protected:
        Array<ObjectTemplateParam> m_parameters;
        ObjectTemplate* m_editorOnlyBaseTemplate;

        mutable base::RefPtr<CachedTemplatePropertyList> m_propertyList;
        mutable SpinLock m_propertyListLock;

        base::RefPtr<CachedTemplatePropertyList> propertyList() const;

        ClassType objectClassForEdit() const;

        static void CopyProperties(const Array<ObjectTemplateParam>& sourceList, Array<ObjectTemplateParam>& outProperties, const base::ObjectPtr& owner);
        static void MergeProperties(const Array<ObjectTemplateParam>** parameterLists, uint32_t numParameterLists, Array<ObjectTemplateParam>& outProperties, const base::ObjectPtr& owner);

        static void TakeOwnershipOfPropertyValue(Variant& inOutValue, const base::ObjectPtr& owner);

        void applyProperties(const base::ObjectPtr& target) const;

        virtual bool applyProperty(const base::ObjectPtr& target, base::StringID name, Type dataType, const void* data) const;
        virtual bool applyPropertyRaw(const base::ObjectPtr& target, base::StringID name, Type dataType, const void* data) const;
    };

} // base
