/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: object #]
***/

#include "build.h"
#include "objectTemplate.h"
#include "rttiNativeClassType.h"
#include "rttiDataView.h"
#include "dataViewNative.h"

#include "base/containers/include/inplaceArray.h"

namespace base
{
    //---

    IObjectTemplate::IObjectTemplate()
    {}

    IObjectTemplate::~IObjectTemplate()
    {}

    //--

    SpecificClassType<IObjectTemplate> IObjectTemplate::GetStaticClass()
    {
        static ClassType objectType = RTTI::GetInstance().findClass("base::IObjectTemplate"_id);
        return SpecificClassType<IObjectTemplate>(*objectType.ptr());
    }

    ClassType IObjectTemplate::cls() const
    {
        return GetStaticClass();
    }

    //--

    void IObjectTemplate::rebase(const IObjectTemplate* base, bool copyValuesOfNonOverriddenProperties /*= true*/)
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
                for (const auto* baseProp : m_base->cls()->allProperties())
                {
                    // should we copy it ? we copy ALL properties that are not overridden - that includes also the ones that were NOT overridable
                    if (!baseProp->overridable() || !m_overridenProperties.contains(baseProp->name()))
                    {
                        // find local property
                        if (const auto* prop = cls()->findProperty(baseProp->name()))
                        {
                            // same type, copy only if changed
                            const void* propValue = prop->offsetPtr(this);
                            const void* basePropValue = baseProp->offsetPtr(m_base);
                            if (prop->type() == baseProp->type())
                            {
                                // copy only if the value is different indeed
                                if (!prop->type()->compare(propValue, basePropValue))
                                {
                                    if (CopyPropertyValue(m_base, baseProp, this, prop))
                                    {
                                        // make sure object's are notified only if the value did change
                                        changedProperties.pushBack(prop->name());
                                    }
                                }
                            }
                            else
                            {
                                // capture current value
                                rtti::DataHolder oldValue(prop->type(), propValue);
                                if (CopyPropertyValue(m_base, baseProp, this, prop))
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

    bool IObjectTemplate::checkPropertyOverrideCaps(StringID name) const
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

    bool IObjectTemplate::hasPropertyOverride(StringID name) const
    {
        if (checkPropertyOverrideCaps(name))
            return m_overridenProperties.contains(name);
        else
            return false;
    }

    bool CopyPropertyValue(const IObject* srcObject, const rtti::Property* srcProperty, IObject* targetObject, const rtti::Property* targetProperty)
    {
        const auto srcType = srcProperty->type();
        const auto targetType = targetProperty->type();

        const auto* srcData = srcProperty->offsetPtr(srcObject);
        auto* targetData = targetProperty->offsetPtr(targetObject);

        // TODO: support for inlined objects! and more complex types

        if (!rtti::ConvertData(srcData, srcType, targetData, targetType))
            return false;

        return true;
    }

    bool IObjectTemplate::resetPropertyOverride(StringID name)
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
            markModified(); // save required
        }

        return wasReset;
    }

    void IObjectTemplate::onPropertyChanged(StringView<char> path)
    {
        IObject::onPropertyChanged(path);

        // store information that we've changed this property
        if (0 == m_localSuppressOverridenPropertyCapture)
        {
            base::StringView<char> propertyName;
            if (base::rtti::ParsePropertyName(path, propertyName))
            {
                if (auto propertyStringID = StringID::Find(propertyName))
                    m_overridenProperties.insert(propertyStringID);
            }
        }
    }

    bool IObjectTemplate::onPropertyShouldLoad(const rtti::Property* prop)
    {
        if (prop->overridable())
        {
            m_overridenProperties.insert(prop->name());
            return true;
        }

        return IObject::onPropertyShouldLoad(prop);
    }

    bool IObjectTemplate::onPropertyShouldSave(const rtti::Property* prop) const
    {
        if (prop->overridable())
            return m_overridenProperties.contains(prop->name());

        return IObject::onPropertyShouldSave(prop);
    }

    //---

    DataViewResult IObjectTemplate::describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const
    {
        if (auto ret = HasError(IObject::describeDataView(viewPath, outInfo)))
            return ret;

        if (viewPath.empty())
        {
            // remove the non-overridable member
            if (outInfo.requestFlags.test(rtti::DataViewRequestFlagBit::MemberList))
            {
                if (m_base)
                {
                    auto oldMembers = std::move(outInfo.members);
                    outInfo.members.reserve(oldMembers.size());

                    for (auto& member : oldMembers)
                    {
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

        base::StringView<char> propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            if (outInfo.requestFlags.test(base::rtti::DataViewRequestFlagBit::CheckIfResetable))
            {
                if (const auto* prop = cls()->findProperty(StringID::Find(propertyName)))
                {
                    if (prop->overridable())
                    {
                        if (m_overridenProperties.contains(prop->name()))
                            outInfo.flags |= base::rtti::DataViewInfoFlagBit::ResetableToBaseValue;
                        else
                            outInfo.flags -= base::rtti::DataViewInfoFlagBit::ResetableToBaseValue;
                    }
                }
            }
        }

        return DataViewResultCode::OK;
    }

    DataViewResult IObjectTemplate::readDataView(StringView<char> viewPath, void* targetData, Type targetType) const
    {
        return IObject::readDataView(viewPath, targetData, targetType);
    }

    DataViewResult IObjectTemplate::writeDataView(StringView<char> viewPath, const void* sourceData, Type sourceType)
    {
        return IObject::writeDataView(viewPath, sourceData, sourceType);
    }

    //---

    void IObjectTemplate::RegisterType(rtti::TypeSystem& typeSystem)
    {
        const auto base = typeSystem.findClass("base::IObject"_id);
        auto cls = MemNew(rtti::NativeClass, "base::IObjectTemplate", sizeof(IObjectTemplate), alignof(IObjectTemplate), typeid(IObjectTemplate).hash_code()).ptr;
        cls->baseClass(base);
        typeSystem.registerType(cls);
    }

    //---

    class ObjectTemplateDataView : public DataViewNative
    {
    public:
        ObjectTemplateDataView(IObjectTemplate* ot)
            : DataViewNative(ot)
            , m_objectTemplate(ot)
        {}

        virtual base::DataViewResult readDefaultDataView(base::StringView<char> viewPath, void* targetData, base::Type targetType) const
        {
            if (!m_objectTemplate)
                return base::DataViewResultCode::ErrorNullObject;

            if (auto base = m_objectTemplate->base())
                return base->readDataView(viewPath, targetData, targetType);

            return DataViewNative::readDataView(viewPath, targetData, targetType);
        }

        virtual base::DataViewResult resetToDefaultValue(base::StringView<char> viewPath, void* targetData, base::Type targetType) const
        {
            if (!m_objectTemplate)
                return base::DataViewResultCode::ErrorNullObject;

            base::StringView<char> propertyNameStr;
            if (base::rtti::ParsePropertyName(viewPath, propertyNameStr))
            {
                if (viewPath.empty())
                {
                    if (const auto propertyName = StringID::Find(propertyNameStr))
                    {
                        if (m_objectTemplate->resetPropertyOverride(propertyName))
                            return base::DataViewResultCode::OK;
                    }
                }
            }

            return base::DataViewNative::resetToDefaultValue(viewPath, targetData, targetType);
        }

        virtual bool checkIfCurrentlyADefaultValue(base::StringView<char> viewPath) const
        {
            auto originalPath = viewPath;

            if (!m_objectTemplate)
                return false;

            base::StringView<char> propertyNameStr;
            if (base::rtti::ParsePropertyName(viewPath, propertyNameStr))
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

            return base::DataViewNative::checkIfCurrentlyADefaultValue(originalPath);
        }

    private:
        IObjectTemplate* m_objectTemplate;
    };

    DataViewPtr IObjectTemplate::createDataView() const
    {
        return base::CreateSharedPtr<ObjectTemplateDataView>(const_cast<IObjectTemplate*>(this));
    }

    //---

} // base

