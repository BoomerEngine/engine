/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once

#include "base/object/include/dataView.h"

namespace base
{
    //--

    /// data view for templated objects
    class BASE_RESOURCE_API ObjectIndirectTemplateDataView : public IDataView
    {
    public:
        ObjectIndirectTemplateDataView(ObjectIndirectTemplate* editableTemplate, const ObjectIndirectTemplate** baseTemplates = nullptr, uint32_t numBaseTemplates = 0);
        virtual ~ObjectIndirectTemplateDataView();

        // check if given property value is default or not
        //bool checkPropertyValueIsDefault(StringView viewPath, bool& outIsDefault) const;

        // reset property to default value
        //DataViewResult resetPropertyValue(StringView viewPath);

        // reset property to default value
        bool removeTemplateProperty(StringID name);

        /// IDataView
        virtual DataViewResult describeDataView(StringView viewPath, rtti::DataViewInfo& outInfo) const override final;
        virtual DataViewResult readDataView(StringView viewPath, void* targetData, Type targetType) const override final;
        virtual DataViewResult writeDataView(StringView viewPath, const void* sourceData, Type sourceType) const override final;

        /// IDataView actions
        virtual DataViewActionResult actionValueWrite(StringView viewPath, const void* sourceData, Type sourceType) const override final;
        virtual DataViewActionResult actionValueReset(StringView viewPath) const override final;
        virtual DataViewActionResult actionArrayClear(StringView viewPath) const override final;
        virtual DataViewActionResult actionArrayInsertElement(StringView viewPath, uint32_t index) const override final;
        virtual DataViewActionResult actionArrayRemoveElement(StringView viewPath, uint32_t index) const override final;
        virtual DataViewActionResult actionArrayNewElement(StringView viewPath) const override final;
        virtual DataViewActionResult actionObjectClear(StringView viewPath) const override final;
        virtual DataViewActionResult actionObjectNew(StringView viewPath, ClassType objectClass) const override final;

    private:
        ObjectIndirectTemplatePtr m_editableTemplate;
        InplaceArray<ObjectIndirectTemplatePtr, 8> m_baseTemplatesRev;
        InplaceArray<ObjectIndirectTemplatePtr, 8> m_allTemplatesRev;

        HashMap<StringView, const rtti::TemplateProperty*> m_templateProperties;
        HashMap<StringID, const rtti::TemplateProperty*> m_templatePropertiesByID;

        const rtti::TemplateProperty* findTemplateProperty(StringView name) const;
        const rtti::TemplateProperty* findTemplateProperty(StringID name) const;

        const void* findDataForProperty(StringID name, Type expectedType) const;
        const void* findBaseDataForProperty(StringID name, Type expectedType) const;

        void updateClass();

        ClassType m_class;

        GlobalEventTable m_events;
    };

    //--

} // base
