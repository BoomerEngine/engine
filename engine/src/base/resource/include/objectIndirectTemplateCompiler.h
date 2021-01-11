/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once

namespace base
{
    //--

    /// helper interface to compile final property view from multiple indirect object templates
    class BASE_RESOURCE_API ObjectIndirectTemplateCompiler : public ITemplatePropertyValueContainer
    {
    public:
        ObjectIndirectTemplateCompiler();
        virtual ~ObjectIndirectTemplateCompiler();

        //--

        // clear templates on list
        void clear();

        // add template to list
        void addTemplate(const ObjectIndirectTemplate* ptr);

        // remove template from list
        void removeTemplate(const ObjectIndirectTemplate* ptr);

        //--

        // determine final object class
        virtual ClassType compileClass() const override final;

        // compile value of single property
        virtual bool compileValue(StringID name, Type expectedType, void* ptr) const override final;

        // compile stacked transform
        EulerTransform compileTransform() const;

        //--

        // flatten all templates into a single template
        // NOTE: created template is ALWAYS a clone so it's safe to use regardless of what happens to source data
        ObjectIndirectTemplatePtr flatten() const;

        //--

    private:
        InplaceArray<const ObjectIndirectTemplate*, 8> m_templates;
        InplaceArray<const ObjectIndirectTemplate*, 8> m_enabledTemplates; // reversed order so we can break on first match

        ClassType m_objectClass;

        void updateObjectClass();
    };

    //--

} // base
