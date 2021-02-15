/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: object #]
***/

#include "build.h"
#include "objectIndirectTemplate.h"
#include "objectIndirectTemplateDataView.h"
#include "base/object/include/rttiDataView.h"
#include "base/object/include/action.h"

namespace base
{
    //---

    ObjectIndirectTemplateDataView::ObjectIndirectTemplateDataView(ObjectIndirectTemplate* editableTemplate, const ObjectIndirectTemplate** baseTemplates /*= nullptr*/, uint32_t numBaseTemplates /*= 0*/)
        : m_editableTemplate(AddRef(editableTemplate))
        , m_events(this)
    {
        if (editableTemplate)
        {
            m_allTemplatesRev.pushBack(AddRef(editableTemplate));

            m_events.bind(editableTemplate->eventKey(), EVENT_OBJECT_PROPERTY_CHANGED) = [this](StringBuf path)
            {
                dispatchPropertyChanged(path);
            };

            m_events.bind(editableTemplate->eventKey(), EVENT_OBJECT_STRUCTURE_CHANGED) = [this]()
            {
                dispatchFullStructureChanged();
            };
        }

        if (numBaseTemplates && baseTemplates)
        {
            m_baseTemplatesRev.reserve(numBaseTemplates);

            for (int i = numBaseTemplates - 1; i >= 0; --i)
            {
                if (baseTemplates[i] && baseTemplates[i]->enabled())
                {
                    m_baseTemplatesRev.pushBack(AddRef(baseTemplates[i]));
                    m_allTemplatesRev.pushBack(AddRef(baseTemplates[i]));
                }
            }
        }

        updateClass();
    }

    ObjectIndirectTemplateDataView::~ObjectIndirectTemplateDataView()
    {}

    void ObjectIndirectTemplateDataView::updateClass()
    {
        for (const auto& ptr : m_allTemplatesRev)
        {
            if (ptr->templateClass())
            {
                m_class = ptr->templateClass();
                break;
            }
        }

        m_templateProperties.reset();
        m_templatePropertiesByID.reset();

        if (m_class)
        {
            const auto& props = m_class->allTemplateProperties();

            m_templateProperties.reserve(props.size());
            m_templatePropertiesByID.reserve(props.size());

            for (const auto& prop : props)
            {
                m_templateProperties[prop.name.view()] = &prop; // StringID view is immutable so this is legal
                m_templatePropertiesByID[prop.name] = &prop;
            }
        }
    }

    const rtti::TemplateProperty* ObjectIndirectTemplateDataView::findTemplateProperty(StringView name) const
    {
        const rtti::TemplateProperty* ret = nullptr;
        m_templateProperties.find(name, ret);
        return ret;
    }

    const rtti::TemplateProperty* ObjectIndirectTemplateDataView::findTemplateProperty(StringID name) const
    {
        const rtti::TemplateProperty* ret = nullptr;
        m_templatePropertiesByID.find(name, ret);
        return ret;
    }

    const void* ObjectIndirectTemplateDataView::findBaseDataForProperty(StringID name, Type expectedType) const
    {
        for (const auto& ptr : m_baseTemplatesRev)
            if (const auto* prop = ptr->findProperty(name))
                if (prop->type == expectedType)
                    return prop->data.data();

        if (m_class)
            if (const auto* prop = findTemplateProperty(name))
                if (prop->type == expectedType)
                    return prop->defaultValue;

        return nullptr;
    }

    const void* ObjectIndirectTemplateDataView::findDataForProperty(StringID name, Type expectedType) const
    {
        if (m_editableTemplate)
            if (const auto* prop = m_editableTemplate->findProperty(name))
                if (prop->type == expectedType)
                    return prop->data.data();

        return findBaseDataForProperty(name, expectedType);
    }

    /*bool ObjectIndirectTemplateDataView::checkPropertyValueIsDefault(StringView viewPath, bool& outIsDefault) const
    {
        if (!m_editableTemplate)
        {
            outIsDefault = true;
            return true;
        }

        base::StringView propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            if (const auto* prop = findTemplateProperty(propertyName))
            {
                if (const auto* localProp = m_editableTemplate->findProperty(prop->name))
                {
                    if (viewPath.empty())
                    {
                        outIsDefault = false;
                        return true;
                    }

                    const auto* basePropData = findBaseDataForProperty(prop->name, prop->type);
                    if (!basePropData)
                        return false;

                    rtti::DataHolder localValueHolder(prop->type);
                    if (!localProp->data.type()->readDataView(viewPath, localProp->data.data(), localValueHolder.data(), localValueHolder.type()).valid())
                        return false;

                    rtti::DataHolder baseValueHolder(prop->type);
                    if (!prop->type->readDataView(viewPath, prop->defaultValue, baseValueHolder.data(), baseValueHolder.type()).valid())
                        return false;

                    outIsDefault = prop->type->compare(localValueHolder.data(), baseValueHolder.data());
                    return true;
                }
            }
        }

        return false;
    }

    DataViewResult ObjectIndirectTemplateDataView::resetPropertyValue(StringView viewPath)
    {
        base::StringView propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            if (const auto* prop = findTemplateProperty(propertyName))
            {
                if (!m_editableTemplate)
                    return DataViewResultCode::ErrorReadOnly;

                if (viewPath.empty())
                {
                    if (m_editableTemplate->removeProperty(prop->name))
                    {
                        const_cast<ObjectIndirectTemplateDataView*>(this)->dispatchPropertyChanged(propertyName);
                        return DataViewResultCode::OK;
                    }

                    return DataViewResultCode::ErrorIllegalAccess;
                }
                else
                {
                    auto* editableData = m_editableTemplate->findProperty(prop->name);
                    if (!editableData)
                        return DataViewResultCode::ErrorIllegalAccess;

                    const auto* basePropData = findBaseDataForProperty(prop->name, prop->type);
                    if (!basePropData)
                        return DataViewResultCode::ErrorIllegalAccess;

                    auto ret = prop->type->writeDataView(viewPath, (void*)editableData->data.data(), basePropData, prop->type);
                    if (ret.valid())
                        const_cast<ObjectIndirectTemplateDataView*>(this)->dispatchPropertyChanged(propertyName);

                    return ret;
                }
            }

            return DataViewResultCode::ErrorUnknownProperty;
        }

        return DataViewResultCode::ErrorIllegalOperation;
    }*/

    bool ObjectIndirectTemplateDataView::removeTemplateProperty(StringID name)
    {
        if (m_editableTemplate)
            return m_editableTemplate->removeProperty(name);
       
        return false;
    }

    DataViewResult ObjectIndirectTemplateDataView::describeDataView(StringView viewPath, rtti::DataViewInfo& outInfo) const
    {
        if (viewPath.empty())
        {
            outInfo.objectClass = m_class;

            if (m_class)
            {
                outInfo.flags |= base::rtti::DataViewInfoFlagBit::LikeStruct;

                if (outInfo.requestFlags.test(base::rtti::DataViewRequestFlagBit::MemberList))
                {
                    for (const auto& prop : m_class->allTemplateProperties())
                    {
                        if (outInfo.categoryFilter.empty() || prop.category == outInfo.categoryFilter)
                        {
                            auto& member = outInfo.members.emplaceBack();
                            member.name = prop.name;
                            member.category = prop.category;
                            member.type = prop.type;
                        }
                    }
                }
            }
            
            return DataViewResultCode::OK;
        }
        else
        {
            base::StringView propertyName;
            if (base::rtti::ParsePropertyName(viewPath, propertyName))
            {
                if (const auto* prop = findTemplateProperty(propertyName))
                {
                    const auto* propData = findDataForProperty(prop->name, prop->type);
                    DEBUG_CHECK(propData);

                    if (viewPath.empty())
                    {
                        outInfo.dataType = prop->type;

                        if (!m_editableTemplate)
                            outInfo.flags |= rtti::DataViewInfoFlagBit::ReadOnly; // we don't have any editable object, all properties are read only

                        if (outInfo.requestFlags.test(rtti::DataViewRequestFlagBit::CheckIfResetable))
                            if (m_editableTemplate->findProperty(prop->name))
                                outInfo.flags |= rtti::DataViewInfoFlagBit::ResetableToBaseValue;
                                
                        if (outInfo.requestFlags.test(rtti::DataViewRequestFlagBit::PropertyEditorData))
                            outInfo.editorData = prop->editorData;
                    }
                    else
                    {
                        if (outInfo.requestFlags.test(rtti::DataViewRequestFlagBit::CheckIfResetable))
                        {
                            // TODO
                        }
                    }

                    return prop->type->describeDataView(viewPath, propData, outInfo);
                }

                return DataViewResultCode::ErrorUnknownProperty;
            }
        }

        return DataViewResultCode::ErrorIllegalOperation;
    }

    DataViewResult ObjectIndirectTemplateDataView::readDataView(StringView viewPath, void* targetData, Type targetType) const
    {
        base::StringView propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            if (const auto* templateProp = findTemplateProperty(propertyName))
            {
                // TODO: arrays!!

                // find value in the objects
                for (const auto& ptr : m_allTemplatesRev)
                {
                    if (const auto* prop = ptr->findProperty(templateProp->name))
                        return prop->data.type()->readDataView(viewPath, prop->data.data(), targetData, targetType);
                }

                // read from default object 
                return templateProp->type->readDataView(viewPath, templateProp->defaultValue, targetData, targetType);
            }

            return DataViewResultCode::ErrorUnknownProperty;
        }

        return DataViewResultCode::ErrorIllegalOperation;
    }

    DataViewResult ObjectIndirectTemplateDataView::writeDataView(StringView viewPath, const void* sourceData, Type sourceType) const
    {
        const auto orgViewPath = viewPath;

        base::StringView propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            if (const auto* templateProp = findTemplateProperty(propertyName))
            {
                if (!m_editableTemplate)
                    return DataViewResultCode::ErrorReadOnly;

                // do we have existing property ?
                auto* prop = m_editableTemplate->findProperty(templateProp->name);

                // no property, create one
                if (!prop)
                {
                    if (!m_editableTemplate->createProperty(templateProp))
                        return DataViewResultCode::ErrorReadOnly;

                    prop = m_editableTemplate->findProperty(templateProp->name);
                }

                // write value into the editable object's property
                if (prop)
                {
                    auto ret = prop->data.type()->writeDataView(viewPath, (void*)prop->data.data(), sourceData, sourceType);
                    if (ret.valid())
                        m_editableTemplate->onPropertyChanged(orgViewPath);
                    return ret;
                }
            }

            return DataViewResultCode::ErrorUnknownProperty;
        }

        return DataViewResultCode::ErrorIllegalOperation;
    }

    //--

    struct ActionWriteTemplateProperty : public IAction
    {
    public:
        ActionWriteTemplateProperty(const ObjectIndirectTemplateDataView* view, StringView viewPath, rtti::DataHolder&& oldValue, rtti::DataHolder&& newValue)
            : m_newValue(std::move(newValue))
            , m_oldValue(std::move(oldValue))
            , m_view(AddRef(view))
            , m_path(viewPath)
        {}

        virtual StringID id() const override
        {
            return "WriteProperty"_id;
        }

        StringBuf description() const override
        {
            return TempString("Change property '{}'", m_path);
        }

        virtual bool execute() override
        {
            return m_view->writeDataViewSimple(m_path, m_newValue).valid();
        }

        virtual bool undo() override
        {
            return m_view->writeDataViewSimple(m_path, m_oldValue).valid();
        }

        virtual bool tryMerge(const IAction& lastUndoAction)
        {
            return false;
        }

    private:
        rtti::DataHolder m_newValue;
        rtti::DataHolder m_oldValue;

        RefPtr<ObjectIndirectTemplateDataView> m_view;
        StringBuf m_path;
    };

    struct ActionCreateTemplateProperty : public IAction
    {
    public:
        ActionCreateTemplateProperty(const ObjectIndirectTemplateDataView* view, StringID propertyName, StringView viewPath, rtti::DataHolder&& newValue)
            : m_newValue(std::move(newValue))
            , m_propertyName(propertyName)
            , m_view(AddRef(view))
            , m_path(viewPath)
        {}

        virtual StringID id() const override
        {
            return "CreateProperty"_id;
        }

        StringBuf description() const override
        {
            return TempString("Set property '{}'", m_path);
        }

        virtual bool execute() override
        {
            return m_view->writeDataViewSimple(m_path, m_newValue).valid();
        }

        virtual bool undo() override
        {
            return m_view->removeTemplateProperty(m_propertyName);
        }

        virtual bool tryMerge(const IAction& lastUndoAction)
        {
            return false;
        }

    private:
        rtti::DataHolder m_newValue;

        RefPtr<ObjectIndirectTemplateDataView> m_view;
        StringID m_propertyName;
        StringBuf m_path;
    };

    DataViewActionResult ObjectIndirectTemplateDataView::actionValueWrite(StringView viewPath, const void* sourceData, Type sourceType) const
    {
        auto originalViewPath = viewPath;

        base::StringView propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            if (const auto* templateProp = findTemplateProperty(propertyName))
            {
                if (!m_editableTemplate)
                    return DataViewResultCode::ErrorReadOnly;

                // write/create whole properties
                if (auto* prop = m_editableTemplate->findProperty(templateProp->name))
                {
                    rtti::DataViewInfo templatePropInfo;
                    auto ret = templateProp->type->describeDataView(viewPath, templateProp->defaultValue, templatePropInfo);
                    if (!ret.valid() || !templatePropInfo.dataType)
                        return ret;

                    rtti::DataHolder currentValue(templatePropInfo.dataType);
                    ret = templateProp->type->readDataView(viewPath, prop->data.data(), currentValue.data(), currentValue.type());
                    if (!ret.valid())
                        return ret;

                    rtti::DataHolder newValue(sourceType, sourceData);
                    return RefNew<ActionWriteTemplateProperty>(this, originalViewPath, std::move(currentValue), std::move(newValue));
                }
                else
                {
                    const auto* defaultValue = findBaseDataForProperty(templateProp->name, templateProp->type);
                    if (!defaultValue)
                        return DataViewResultCode::ErrorIllegalAccess;

                    rtti::DataHolder newValue(templateProp->type, defaultValue);
                    auto ret = templateProp->type->writeDataView(viewPath, newValue.data(), sourceData, sourceType);
                    if (!ret.valid())
                        return ret;

                    return RefNew<ActionCreateTemplateProperty>(this, templateProp->name, propertyName, std::move(newValue));
                }
            }

            return DataViewResultCode::ErrorUnknownProperty;
        }

        return DataViewResultCode::ErrorIllegalOperation;
    }

    struct ActionResetTemplateProperty : public IAction
    {
    public:
        ActionResetTemplateProperty(const ObjectIndirectTemplateDataView* view, StringID propertyName, rtti::DataHolder&& oldValue)
            : m_oldValue(std::move(oldValue))
            , m_view(AddRef(view))
            , m_propertyName(propertyName)
        {}

        virtual StringID id() const override
        {
            return "ResetProperty"_id;
        }

        StringBuf description() const override
        {
            return TempString("Reset property '{}'", m_propertyName);
        }

        virtual bool execute() override
        {
            return m_view->removeTemplateProperty(m_propertyName);
        }

        virtual bool undo() override
        {
            return m_view->writeDataViewSimple(m_propertyName.view(), m_oldValue).valid();
        }

        virtual bool tryMerge(const IAction& lastUndoAction)
        {
            return false;
        }

    private:
        rtti::DataHolder m_oldValue;

        RefPtr<ObjectIndirectTemplateDataView> m_view;
        StringID m_propertyName;
    };

    DataViewActionResult ObjectIndirectTemplateDataView::actionValueReset(StringView viewPath) const
    {
        auto originalViewPath = viewPath;

        base::StringView propertyName;
        if (base::rtti::ParsePropertyName(viewPath, propertyName))
        {
            if (const auto* templateProp = findTemplateProperty(propertyName))
            {
                if (!m_editableTemplate)
                    return DataViewResultCode::ErrorReadOnly;

                const auto* prop = m_editableTemplate->findProperty(templateProp->name);
                if (!prop)
                    return DataViewResultCode::ErrorIllegalAccess;

                if (viewPath.empty())
                {
                    rtti::DataHolder currentValue(prop->data.type(), prop->data.data());
                    return RefNew<ActionResetTemplateProperty>(this, propertyName, std::move(currentValue));
                }
                else
                {
                    const auto* defaultData = findBaseDataForProperty(templateProp->name, templateProp->type);
                    if (!defaultData)
                        return DataViewResultCode::ErrorIllegalAccess;

                    rtti::DataViewInfo defaultPropInfo;
                    auto ret = templateProp->type->describeDataView(viewPath, defaultData, defaultPropInfo);
                    if (!ret.valid() || !defaultPropInfo.dataType)
                        return ret;

                    rtti::DataHolder defaultValue(defaultPropInfo.dataType);
                    ret = templateProp->type->readDataView(viewPath, defaultData, defaultValue.data(), defaultValue.type());
                    if (!ret.valid())
                        return ret;

                    rtti::DataHolder currentValue(defaultPropInfo.dataType);
                    ret = templateProp->type->readDataView(viewPath, prop->data.data(), currentValue.data(), currentValue.type());
                    if (!ret.valid())
                        return ret;

                    return RefNew<ActionWriteTemplateProperty>(this, originalViewPath, std::move(currentValue), std::move(defaultValue));
                }
            }

            return DataViewResultCode::ErrorUnknownProperty;
        }

        return DataViewResultCode::ErrorIllegalOperation;
    }

    DataViewActionResult ObjectIndirectTemplateDataView::actionArrayClear(StringView viewPath) const
    {
        return DataViewResultCode::ErrorIllegalOperation;
    }

    DataViewActionResult ObjectIndirectTemplateDataView::actionArrayInsertElement(StringView viewPath, uint32_t index) const
    {
        return DataViewResultCode::ErrorIllegalOperation;
    }

    DataViewActionResult ObjectIndirectTemplateDataView::actionArrayRemoveElement(StringView viewPath, uint32_t index) const
    {
        return DataViewResultCode::ErrorIllegalOperation;
    }

    DataViewActionResult ObjectIndirectTemplateDataView::actionArrayNewElement(StringView viewPath) const
    {
        return DataViewResultCode::ErrorIllegalOperation;
    }

    DataViewActionResult ObjectIndirectTemplateDataView::actionObjectClear(StringView viewPath) const
    {
        return DataViewResultCode::ErrorIllegalOperation;
    }

    DataViewActionResult ObjectIndirectTemplateDataView::actionObjectNew(StringView viewPath, ClassType objectClass) const
    {
        return DataViewResultCode::ErrorIllegalOperation;
    }

    //---

} // base

