/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

/// instance object property value
struct CORE_RESOURCE_API ObjectIndirectTemplateProperty
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ObjectIndirectTemplateProperty);

public:
    ObjectIndirectTemplateProperty();

    Type type;
    StringID name;
    Variant data;
};

//--

/// Object that is meant specifically to be used as a "template" to create other objects and allow any class to be chosen
/// Properties that have meaningful (assigned) values are stored in a table
class CORE_RESOURCE_API ObjectIndirectTemplate : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(ObjectIndirectTemplate, IObject);

public:
    ObjectIndirectTemplate();
    virtual ~ObjectIndirectTemplate();

    //--

    // is this template enabled? property values from disabled template will not be applied (as if it was deleted)
    INLINE bool enabled() const { return m_enabled; }

    // placement
    // NOTE: this property is not overridden but stacks (ie. transforms are stacking one on the other)
    INLINE const EulerTransform& placement() const { return m_placement; }

    // raw property list
    INLINE const Array<ObjectIndirectTemplateProperty>& properties() const { return m_properties; }

    // object template class
    INLINE ClassType templateClass() const { return m_templateClass; }

    //--

    // toggle the enable flag
    void enable(bool flag, bool callEvent = true);

    // set placement
    void placement(const EulerTransform& placement, bool callEvent = true);

    // change template class
    void templateClass(ClassType cls, bool callEvent = true);

    //--

    // find template property
    const ObjectIndirectTemplateProperty* findProperty(StringID name) const;

    // remove template property (reset value to default)
    bool removeProperty(StringID name);

    // create template property
    bool createProperty(const rtti::TemplateProperty* source);

    //--

    // add template property
    bool addProperty(const ObjectIndirectTemplateProperty& prop);

    // write template property value
    bool writeProperty(StringID name, const void* data, const Type type);

    // write property value
    template< typename T >
    INLINE bool writeProperty(StringID name, const T& data)
    {
        return writeProperty(name, &data, reflection::GetTypeObject<T>());
    }

    //--

private:
    bool m_enabled = true;
    EulerTransform m_placement; // HACK: allow the indirect template objects to store placement info directly, this is a special case... :(

    ClassType m_templateClass;

    Array<ObjectIndirectTemplateProperty> m_properties;
};

///--

END_BOOMER_NAMESPACE()

