/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: object #]
***/

#include "build.h"
#include "directTemplate.h"

#include "core/containers/include/inplaceArray.h"
#include "core/object/include/rttiNativeClassType.h"
#include "core/object/include/rttiDataView.h"
#include "core/object/include/dataViewNative.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IObjectDirectTemplate);
RTTI_END_TYPE();

IObjectDirectTemplate::IObjectDirectTemplate()
{}

IObjectDirectTemplate::~IObjectDirectTemplate()
{}

void IObjectDirectTemplate::rebase(const IObjectDirectTemplate* base, bool copyValuesOfNonOverriddenProperties /*= true*/)
{
    if (m_base != base)
    {
        // unbind current base, this frees the object
        m_base.reset();

        // TODO: register in base so we get notified when it changes ?

        // copy properties from base that are not indicated here as overridden
        InplaceArray<StringID, 20> changedProperties;
        if (base)
        {
            for (const auto* baseProp : base->cls()->allProperties())
            {
                // should we copy it ? we copy ALL properties that are not overridden - that includes also the ones that were NOT overridable
                if (!baseProp->overridable() || !m_overridenProperties.contains(baseProp->name()))
                {
                    // find local property
                    if (const auto* prop = cls()->findProperty(baseProp->name()))
                    {
                        // same type, copy only if changed
                        const void* propValue = prop->offsetPtr(this);
                        const void* basePropValue = baseProp->offsetPtr(base);
                        if (prop->type() == baseProp->type())
                        {
                            // copy only if the value is different indeed
                            if (!prop->type()->compare(propValue, basePropValue))
                            {
                                if (CopyPropertyValue(base, baseProp, this, prop))
                                {
                                    // make sure object's are notified only if the value did change
                                    changedProperties.pushBack(prop->name());
                                }
                            }
                        }
                        else
                        {
                            // capture current value
                            DataHolder oldValue(prop->type(), propValue);
                            if (CopyPropertyValue(base, baseProp, this, prop))
                            {
                                // make sure object's are notified only if the value did change
                                if (!prop->type()->compare(propValue, oldValue.data()))
                                {
                                    changedProperties.pushBack(prop->name());
                                }
                            }
                        }
                    }
                }
            }

            // bind base base
            m_base = AddRef(base);

            // notify object about all changed properties
            if (!changedProperties.empty())
            {
                m_localSuppressOverridenPropertyCapture += 1;

                for (const auto& propName : changedProperties)
                    onPropertyChanged(propName.c_str());

                m_localSuppressOverridenPropertyCapture -= 1;
            }
        }
    }
}

void IObjectDirectTemplate::detach(bool mergeOverrideTables)
{
    if (m_base)
    {
        bool somethingAdded = false;
        if (mergeOverrideTables)
        {

            auto base = m_base;
            while (base)
            {
                for (auto& propName : base->m_overridenProperties)
                    somethingAdded |= m_overridenProperties.insert(propName);
                base = base->m_base;
            }
        }

        m_base.reset();

        if (somethingAdded)
            markModified();
    }
}

bool IObjectDirectTemplate::checkPropertyOverrideCaps(StringID name) const
{
    // no base object, we can't really override any property, display all
    if (!m_base)
        return false;

    // find property
    if (const auto* prop = cls()->findProperty(name))
    {
        // if the property exists in the base object as well than it can be overridden ONLY if the property here allows it
        if (const auto* baseProp = m_base->cls()->findProperty(name))
        {
            return baseProp->overridable();
        }
        // there's no such property in base object, free range, allow to override the property
        else
        {
            return true;
        }
    }

    // unknown property
    return false;
}

bool IObjectDirectTemplate::hasPropertyOverride(StringID name) const
{
    if (checkPropertyOverrideCaps(name))
        return m_overridenProperties.contains(name);
    else
        return false;
}

bool IObjectDirectTemplate::resetPropertyOverride(StringID name)
{
    // property must exist here        
    bool wasReset = false;
    if (const auto* prop = cls()->findProperty(name))
    {
        // if we have the base read the property back from base
        if (m_base)
        {
            // if the property exists in the base we will copy the value from the base object
            if (const auto* baseProp = m_base->cls()->findProperty(name))
            {
                // copy data from the base property in base object to this property in this object
                wasReset = CopyPropertyValue(m_base, baseProp, this, prop);
            }
        }

        // no base, use lame reset to default object of class
        else
        {
            if (const auto* defaultData = (const IObject*)cls()->defaultObject())
            {
                wasReset = CopyPropertyValue(defaultData, prop, this, prop);
            }
        }
    }

    // copy worked, we can unflag the property as overridden as well
    if (wasReset)
    {
        m_overridenProperties.remove(name);
            
        {
            m_localSuppressOverridenPropertyCapture += 1;
            onPropertyChanged(name.view());
            m_localSuppressOverridenPropertyCapture -= 1;
        }
    }

    return wasReset;
}

void IObjectDirectTemplate::markPropertyOverride(StringID name)
{
    if (name)
        m_overridenProperties.insert(name);
}

void IObjectDirectTemplate::onPropertyChanged(StringView path)
{
    IObject::onPropertyChanged(path);

    // store information that we've changed this property
    if (0 == m_localSuppressOverridenPropertyCapture)
    {
        StringView propertyName;
        if (ParsePropertyName(path, propertyName))
        {
            if (auto propertyStringID = StringID::Find(propertyName))
                markPropertyOverride(propertyStringID);
        }
    }
}

bool IObjectDirectTemplate::onPropertyShouldLoad(const Property* prop)
{
    if (prop->overridable())
    {
        m_overridenProperties.insert(prop->name());
        return true;
    }

    return IObject::onPropertyShouldLoad(prop);
}

bool IObjectDirectTemplate::onPropertyShouldSave(const Property* prop) const
{
    if (prop->overridable())
        return m_overridenProperties.contains(prop->name());

    return IObject::onPropertyShouldSave(prop);
}

//---

DataViewResult IObjectDirectTemplate::describeDataView(StringView viewPath, DataViewInfo& outInfo) const
{
    {
        auto ret = IObject::describeDataView(viewPath, outInfo);
        if (!ret.valid())
            return ret;
    }

    if (viewPath.empty())
    {
        // remove the non-overridable member
        if (outInfo.requestFlags.test(DataViewRequestFlagBit::MemberList))
        {
            if (m_base)
            {
                auto oldMembers = std::move(outInfo.members);
                outInfo.members.reserve(oldMembers.size());

                for (auto& member : oldMembers)
                {
                    if (outInfo.categoryFilter && member.category != outInfo.categoryFilter)
                        continue;

                    bool keep = false;

                    if (const auto* prop = cls()->findProperty(member.name))
                    {
                        const auto* baseProp = m_base->cls()->findProperty(member.name);
                        if (baseProp && baseProp->overridable())
                            keep = true;
                    }
                    else
                    {
                        // keep non property members
                        keep = true;
                    }

                    if (keep)
                        outInfo.members.emplaceBack(std::move(member));
                }
            }
        }
    }

    StringView propertyName;
    if (ParsePropertyName(viewPath, propertyName))
    {
        if (outInfo.requestFlags.test(DataViewRequestFlagBit::CheckIfResetable))
        {
            if (const auto* prop = cls()->findProperty(StringID::Find(propertyName)))
            {
                if (prop->overridable())
                {
                    if (m_overridenProperties.contains(prop->name()) && prop->resetable())
                        outInfo.flags |= DataViewInfoFlagBit::ResetableToBaseValue;
                    else
                        outInfo.flags -= DataViewInfoFlagBit::ResetableToBaseValue;
                }

                //const auto* data = prop->offsetPtr(this);
                //return prop->type()->describeDataView(viewPath, data, outInfo);
            }
        }
    }

    return DataViewResultCode::OK;
}

DataViewResult IObjectDirectTemplate::readDataView(StringView viewPath, void* targetData, Type targetType) const
{
    return IObject::readDataView(viewPath, targetData, targetType);
}

DataViewResult IObjectDirectTemplate::writeDataView(StringView viewPath, const void* sourceData, Type sourceType)
{
    return IObject::writeDataView(viewPath, sourceData, sourceType);
}

//---

class ObjectTemplateDataView : public DataViewNative
{
public:
    ObjectTemplateDataView(IObjectDirectTemplate* ot, bool readOnly)
        : DataViewNative(ot, readOnly)
        , m_objectTemplate(ot)
    {}

    virtual DataViewResult readDefaultDataView(StringView viewPath, void* targetData, Type targetType) const
    {
        if (!m_objectTemplate)
            return DataViewResultCode::ErrorNullObject;

        if (auto base = m_objectTemplate->base())
            return base->readDataView(viewPath, targetData, targetType);

        return DataViewNative::readDataView(viewPath, targetData, targetType);
    }

    virtual DataViewResult resetToDefaultValue(StringView viewPath, void* targetData, Type targetType) const
    {
        if (!m_objectTemplate)
            return DataViewResultCode::ErrorNullObject;

        StringView propertyNameStr;
        if (ParsePropertyName(viewPath, propertyNameStr))
        {
            if (viewPath.empty())
            {
                if (const auto propertyName = StringID::Find(propertyNameStr))
                {
                    if (m_objectTemplate->resetPropertyOverride(propertyName))
                        return DataViewResultCode::OK;
                }
            }
        }

        return DataViewNative::resetToDefaultValue(viewPath, targetData, targetType);
    }

    virtual bool checkIfCurrentlyADefaultValue(StringView viewPath) const
    {
        auto originalPath = viewPath;

        if (!m_objectTemplate)
            return false;

        StringView propertyNameStr;
        if (ParsePropertyName(viewPath, propertyNameStr))
        {
            if (viewPath.empty())
            {
                if (const auto propertyName = StringID::Find(propertyNameStr))
                {
                    if (m_objectTemplate->checkPropertyOverrideCaps(propertyName))
                    {
                        return !m_objectTemplate->hasPropertyOverride(propertyName);
                    }
                }
            }
            /*else if (const auto& base = m_objectTemplate->base())
            {

            }*/
        }

        return DataViewNative::checkIfCurrentlyADefaultValue(originalPath);
    }

private:
    IObjectDirectTemplate* m_objectTemplate;
};

DataViewPtr IObjectDirectTemplate::createDataView(bool forceReadOnly) const
{
    return RefNew<ObjectTemplateDataView>(const_cast<IObjectDirectTemplate*>(this), forceReadOnly);
}

//---

END_BOOMER_NAMESPACE()
