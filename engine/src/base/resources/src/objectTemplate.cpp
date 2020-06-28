/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: template #]
***/

#include "build.h"
#include "objectTemplate.h"
#include "base/object/include/rttiHandleType.h"
#include "base/containers/include/hashMap.h"
#include "base/object/include/rttiProperty.h"
#include "resource.h"
#include "resourceReferenceType.h"

namespace base
{
    ///---

    RTTI_BEGIN_TYPE_STRUCT(ObjectTemplateParam);
        RTTI_PROPERTY(m_name);
        RTTI_PROPERTY(m_value);
    RTTI_END_TYPE();

    ObjectTemplateParam::ObjectTemplateParam()
    {}

    ///---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ObjectTemplate);
        RTTI_PROPERTY(m_parameters);
    RTTI_END_TYPE();

    ObjectTemplate::ObjectTemplate()
    {}

    ObjectTemplate::~ObjectTemplate()
    {}

    bool ObjectTemplate::applyPropertyRaw(const base::ObjectPtr& target, base::StringID name, Type dataType, const void* data) const
    {
        auto prop  = target->cls()->findProperty(name);
        if (prop)
        {
            void* targetData = prop->offsetPtr(target.get());
            if (!rtti::ConvertData(data, dataType, targetData, prop->type()))
            {
                TRACE_WARNING("Unable to apply template value of type '{}' to property '{}' of type '{}' in '{}'", dataType->name(), name, prop->type()->name(), cls()->name());
                return false;
            }

            target->markModified();
            return true;
        }

        return false;
    }


    bool ObjectTemplate::applyProperty(const base::ObjectPtr& target, base::StringID name, Type dataType, const void* data) const
    {
        // a reference ?
        if (dataType->metaType() == rtti::MetaType::ResourceRef)
        {
            auto refType  = static_cast<const res::ResourceRefType*>(dataType.ptr());

            if (auto resourceClass  = refType->resourceClass())
            {
                res::BaseReference resourceRef;
                refType->readResourceRef(data, resourceRef);

                //resourceRef.ensureLoaded();

                applyPropertyRaw(target, name, refType, &resourceRef);
            }
        }
        else if (dataType->metaType() == rtti::MetaType::StrongHandle)
        {
            ObjectPtr handle;

            auto handleType  = static_cast<const rtti::IHandleType*>(dataType.ptr());
            handleType->readPointedObject(data, handle);

            handle = base::CloneObjectUntyped(handle, target);

            // apply the cloned and reparented object
            return applyPropertyRaw(target, name, handleType, &handle);
        }

        // pass to object
        return applyPropertyRaw(target, name, dataType, data);
    }

    void ObjectTemplate::applyProperties(const base::ObjectPtr& target) const
    {
        for (auto& param : m_parameters)
        {
            if (param.m_value.empty())
                continue;

            applyProperty(target, param.m_name, param.m_value.type(), param.m_value.data());
        }
    }

    void ObjectTemplate::CopyProperties(const Array<ObjectTemplateParam>& sourceList, Array<ObjectTemplateParam>& outProperties, const base::ObjectPtr& owner)
    {
        DEBUG_CHECK_EX(outProperties.empty(), "Output list should be empty");
        outProperties.reserve(sourceList.size());
         
        for (const auto& prop : sourceList)
        {
            if (prop.m_value.empty() || !prop.m_name)
                continue;

            auto& outProp = outProperties.emplaceBack();
            outProp.m_name = prop.m_name;
            outProp.m_value = prop.m_value;

            TakeOwnershipOfPropertyValue(outProp.m_value, owner);
        }
    }

    void ObjectTemplate::TakeOwnershipOfPropertyValue(Variant& value, const base::ObjectPtr& owner)
    {
        auto dataType = value.type();

        // clone and re-parent all objects
        if (dataType->metaType() == rtti::MetaType::StrongHandle)
        {
            ObjectPtr data;

            auto handleType = static_cast<const rtti::IHandleType*>(dataType.ptr());
            handleType->readPointedObject(value.data(), data);

            data = base::CloneObjectUntyped(data, owner);

            handleType->writePointedObject(value.data(), data);
        }

        // TODO: any thing else?
    }

    void ObjectTemplate::MergeProperties(const Array<ObjectTemplateParam>** parameterLists, uint32_t numParameterLists, Array<ObjectTemplateParam>& outProperties, const base::ObjectPtr& owner)
    {
        HashMap<StringID, const ObjectTemplateParam*> names;

        for (uint32_t i = 0; i < numParameterLists; ++i)
            for (auto& entry : *parameterLists[i])
                if (!entry.m_value.empty())
                    names[entry.m_name] = &entry;

        outProperties.reserve(names.size());

        for (uint32_t i = 0; i < names.keys().size(); ++i)
        {
            auto name  = names.keys()[i];
            const ObjectTemplateParam* source = names.values()[i];

            if (source->m_value.empty())
                continue;

            auto& ret = outProperties.emplaceBack();
            ret.m_name = name;
            ret.m_value = source->m_value;

            TakeOwnershipOfPropertyValue(ret.m_value, owner);           
        }
    }

    base::RefPtr<CachedTemplatePropertyList> ObjectTemplate::propertyList() const
    {
        auto lock  = base::CreateLock(m_propertyListLock);

        auto editClass  = objectClassForEdit();
        if (m_propertyList && m_propertyList->m_class != editClass)
            m_propertyList.reset();;

        if (!m_propertyList && editClass)
        {
            m_propertyList = CreateSharedPtr<CachedTemplatePropertyList>();
            m_propertyList->m_class = editClass;

            for (auto prop  : editClass->allProperties())
            {
                if (prop->editable())//&& prop->verridable())
                {
                    auto& outInfo = m_propertyList->m_properties.emplaceBack();
                    outInfo.m_category = prop->category();
                    outInfo.m_name = prop->name();
                    //outInfo.m_setup = prop->valueSetup();
                    outInfo.m_type = prop->type();
                    outInfo.m_defaultValue = Variant(prop->type(), prop->offsetPtr(cls()->defaultObject()));
                }
            }
        }

        return m_propertyList;
    }

    ClassType ObjectTemplate::objectClassForEdit() const
    {
        // find the value for the "class" override property
        if (auto objClass  = objectClass())
            if (objClass->is(rootTemplateClass()))
                return objClass;

        // use parent
        if (m_editorOnlyBaseTemplate)
            return m_editorOnlyBaseTemplate->objectClassForEdit();

        // unknown class
        return nullptr;
    }

    const CachedTemplatePropertyInfo* CachedTemplatePropertyList::findProperty(base::StringID name) const
    {
        for (auto& prop : m_properties)
            if (prop.m_name == name)
                return &prop;
        return nullptr;
    }

    #if 0
    void ObjectTemplate::queryDynamicPropertiesList(Array<StructureElementInfo>& outDynamicProperties) const
    {
        if (auto propList  = propertyList())
        {
            for (auto& prop : propList->m_properties)
            {
                auto& info = outDynamicProperties.emplaceBack();
                info.m_name = prop.m_name;
                info.m_category = prop.m_category;
            }
        }
    }


    ViewPtr ObjectTemplate::createDynamicPropertyView(const ViewPtr& baseView, bool isReadOnly, const AccessPath& initialPath, StringID propertyName) const
    {
        if (auto propList = propertyList())
        {
            if (auto propertyInfo  = propList->findProperty(propertyName))
            {
                auto propPath = initialPath[propertyName];
                return base::CreateSharedPtr<base::NativeView>(baseView, propPath, propertyInfo->m_type, propertyInfo->m_setup);
            }
        }

        return nullptr;
    }

    bool ObjectTemplate::onReadProperty(const AccessPath &path, Variant &outValue) const
    {
        // the component property name is given just as any other property name
        base::AccessPathIterator it(path);
        if (!it.endOfPath())
        {
            // read the name of the component property
            base::StringID propertyName;
            if (it.enterProperty(propertyName))
            {
                // load the component property value
                base::Variant propertyValue;
                if (readParameter(propertyName, propertyValue))
                {
                    // read the sub-value value in the component property
                    return base::ReadNativeData(propertyValue.data(), propertyValue.type(), it.extractRemainingPath(), outValue);
                }
            }
        }

        // try the normal stuff
        return TBaseClass::onReadProperty(path, outValue);
    }

    bool ObjectTemplate::onReadDefaultProperty(const AccessPath &path, Variant &outValue) const
    {
        // the component property name is given just as any other property name
        base::AccessPathIterator it(path);
        if (!it.endOfPath())
        {
            // read the name of the component property
            base::StringID propertyName;
            if (it.enterProperty(propertyName))
            {
                // use the default object
                if (m_editorOnlyBaseTemplate)
                    return m_editorOnlyBaseTemplate->onReadProperty(path, outValue);

                // use the default from the class
                if (auto propList = propertyList())
                {
                    if (auto propertyInfo  = propList->findProperty(propertyName))
                    {
                        return base::ReadNativeData(propertyInfo->m_defaultValue.data(), propertyInfo->m_defaultValue.type(), it.extractRemainingPath(), outValue);
                    }
                }
            }
        }

        // try the normal stuff
        return TBaseClass::onReadDefaultProperty(path, outValue);
    }

    bool ObjectTemplate::onWriteProperty(const AccessPath &path, const Variant &newValue)
    {
        // the component property name is given just as any other property name
        base::AccessPathIterator it(path);
        if (!it.endOfPath())
        {
            // read the name of the component property
            base::StringID propertyName;
            if (it.enterProperty(propertyName))
            {
                // load the current property value
                base::Variant propertyValue;
                if (readParameter(propertyName, propertyValue))
                {
                    // allow object to modify the property value
                    base::Variant changedValue;
                    if (!onPropertyChanging(path, newValue, changedValue))
                    {
                        TRACE_ERROR("Object did not accept property value '{}'", path);
                        return false;
                    }

                    // write over the sub-value value in the component property
                    auto& valueToSet = changedValue.empty() ? newValue : changedValue;
                    if (!base::WriteNativeData(propertyValue.data(), propertyValue.type(), it.extractRemainingPath(), valueToSet))
                    {
                        TRACE_ERROR("Failed to write data into component parameter '{}", path);
                        return false;
                    }

                    // store the value in the component
                    if (!writeParameter(propertyName, propertyValue))
                    {
                        TRACE_ERROR("Failed to write component parameter '{}'", path);
                        return false;
                    }

                    onPropertyChanged(path);
                    return true;
                }
            }
        }

        // write normal value
        return TBaseClass::onWriteProperty(path, newValue);
    }
    #endif

    bool ObjectTemplate::resetParameter(StringID name)
    {
        for (uint32_t i = 0; i < m_parameters.size(); ++i)
        {
            if (m_parameters[i].m_name == name)
            {
                m_parameters.erase(i);
                markModified();

                onPropertyChanged(name.view());
                return true;
            }
        }

        // not found
        return false;
    }

    bool ObjectTemplate::readParameter(StringID name, Variant& outData) const
    {
        // read local value
        for (auto& it : m_parameters)
        {
            if (it.m_name == name)
            {
                outData = it.m_value;
                return true;
            }
        }

        // read from base object if defined
        if (m_editorOnlyBaseTemplate)
            return m_editorOnlyBaseTemplate->readParameter(name, outData);

        // read from default properties of template class
        if (auto propList = propertyList())
        {
            if (auto propertyInfo  = propList->findProperty(name))
            {
                outData = propertyInfo->m_defaultValue;
                return true;
            }
        }

        // not a valid value
        return false;
    }

    bool ObjectTemplate::writeParameter(StringID name, const Variant& newData)
    {
        // we cannot override local parameters
        if (cls()->findProperty(name) != nullptr)
        {
            TRACE_ERROR("Local class property '{}' aliases template property. Not allowed.", name);
            return false;
        }

        // read from default properties of template class
        if (auto propList = propertyList())
        {
            if (!propList->findProperty(name))
            {
                TRACE_ERROR("Template property '{}' is not reported by template class '{}' and cannot be overriden", name, propList->m_class->name());
            }
        }
        else
        {
            return false;
        }

        // modify existing param
        for (auto& it : m_parameters)
        {
            if (it.m_name == name)
            {
                it.m_value = newData;
                markModified();
                onPropertyChanged(name.view());
                return true;
            }
        }

        // add new param
        auto& param = m_parameters.emplaceBack();
        param.m_name = name;
        param.m_value = newData;
        return true;
    }

    
    ///---

} // base
