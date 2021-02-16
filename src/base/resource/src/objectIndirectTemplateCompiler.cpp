/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: object #]
***/

#include "build.h"
#include "objectIndirectTemplate.h"
#include "objectIndirectTemplateCompiler.h"
#include "resourceAsyncReferenceType.h"
#include "resourceAsyncReference.h"
#include "resourceReference.h"
#include "resourceLoader.h"
#include "resourceLoadingService.h"

#include "base/object/include/rttiResourceReferenceType.h"

namespace base
{
    //---

    ObjectIndirectTemplateCompiler::ObjectIndirectTemplateCompiler()
    {
        m_loader = GetService<res::LoadingService>()->loader();
    }

    ObjectIndirectTemplateCompiler::ObjectIndirectTemplateCompiler(res::ResourceLoader* loader)
        : m_loader(loader)
    {
    }

    ObjectIndirectTemplateCompiler::~ObjectIndirectTemplateCompiler()
    {
    }

    void ObjectIndirectTemplateCompiler::clear()
    {
        m_templates.reset();
        updateObjectClass();
    }

    void ObjectIndirectTemplateCompiler::addTemplate(const ObjectIndirectTemplate* ptr)
    {
        DEBUG_CHECK_RETURN_EX(ptr != nullptr, "Invalid object template");
        DEBUG_CHECK_RETURN_EX(!m_templates.contains(ptr), "Template already on list");

        m_templates.pushBack(ptr);

        if (ptr->enabled())
            m_enabledTemplates.insert(0, ptr);

        updateObjectClass();
    }

    void ObjectIndirectTemplateCompiler::removeTemplate(const ObjectIndirectTemplate* ptr)
    {
        DEBUG_CHECK_RETURN_EX(ptr != nullptr, "Invalid object template");
        DEBUG_CHECK_RETURN_EX(m_templates.contains(ptr), "Template not on list");

        m_templates.remove(ptr);
        m_enabledTemplates.remove(ptr);

        updateObjectClass();
    }

    void ObjectIndirectTemplateCompiler::updateObjectClass()
    {
        m_objectClass = ClassType();

        for (const auto* ptr : m_enabledTemplates)
        {
            if (ptr->templateClass())
            {
                m_objectClass = ptr->templateClass();
                break;
            }
        }
    }

    static base::EulerTransform MergeTransforms(const base::EulerTransform& base, const base::EulerTransform& cur)
    {
        // handle most common case - no transform
        if (base.isIdentity())
            return cur;
        else if (cur.isIdentity())
            return base;

        base::EulerTransform ret;
        ret.T = base.T + cur.T;
        ret.R = base.R + cur.R;
        ret.S = base.S * cur.S;
        return ret;
    }

    EulerTransform ObjectIndirectTemplateCompiler::compileTransform() const
    {
        EulerTransform ret;
        bool first = true;

        for (const auto& data : m_templates)
        {
            if (data->enabled())
            {
                if (!data->placement().isIdentity())
                {
                    if (first)
                        ret = data->placement();
                    else
                        ret = MergeTransforms(ret, data->placement());

                    first = false;
                }
            }
        }

        return ret;
    }

    ClassType ObjectIndirectTemplateCompiler::compileClass() const
    {
        return m_objectClass;
    }

    bool ObjectIndirectTemplateCompiler::compileValue(StringID name, Type expectedType, void* ptr) const
    {
        // direct get
        if (compileValueRaw(name, expectedType, ptr))
            return true;

        // if we failed check if we wanted resource ref instead of actual ref
        if (expectedType->metaType() == rtti::MetaType::ResourceRef)
        {
            const auto* refType = static_cast<const rtti::IResourceReferenceType*>(expectedType.ptr());
            const auto resourceClass = refType->referenceResourceClass().cast<res::IResource>();
            if (resourceClass)
            {
                const auto* asyncRefType = res::CreateAsyncRefType(resourceClass);

                res::BaseAsyncReference asyncRef;
                if (compileValue(name, asyncRefType, &asyncRef))
                {
                    auto* outRef = (res::BaseReference*)ptr;

                    if (asyncRef.empty())
                    {
                        *outRef = res::BaseReference();
                    }
                    else
                    {
                        if (m_loader)
                            *outRef = m_loader->loadResource(asyncRef.path());
                        else
                            *outRef = res::BaseReference(asyncRef.path());
                    }

                    return true;
                }
            }
        }

        return false;
    }

    bool ObjectIndirectTemplateCompiler::compileValueRaw(StringID name, Type expectedType, void* ptr) const
    {
        // look for value in any enabled template
        // TODO: case for arrays 
        // TODO: case for arrays with named structures
        // TODO: case for math types like transforms that can be merged together
        for (const auto* templatePtr : m_enabledTemplates)
        {
            if (const auto* prop = templatePtr->findProperty(name))
            {
                if (prop->data.get(ptr, expectedType))
                    return true;
            }
        }

        // try default value from class itself
        if (m_objectClass)
        {
            const auto& templateProperties = m_objectClass->allTemplateProperties();
            for (const auto& prop : templateProperties)
            {
                if (prop.name == name)
                {
                    if (rtti::ConvertData(prop.defaultValue, prop.type, ptr, expectedType))
                        return true;
                }
            }
        }

        // no source of value 
        return false;
    }

    ObjectIndirectTemplatePtr ObjectIndirectTemplateCompiler::flatten() const
    {
        PC_SCOPE_LVL1(FlattentTemplate);

        // TODO: optimize ;)

        auto ret = RefNew<ObjectIndirectTemplate>();

        ret->templateClass(compileClass());

        HashSet<StringID> propertyNames;
        for (const auto& ptr : m_enabledTemplates)
            for (const auto& prop : ptr->properties())
                propertyNames.insert(prop.name);

        for (const auto& name : propertyNames.keys())
        {
            for (const auto& ptr : m_enabledTemplates)
            {
                bool found = false;
                for (const auto& prop : ptr->properties())
                {
                    if (prop.name == name)
                    {
                        ret->addProperty(prop);
                        found = true;
                        break;
                    }
                }

                if (found)
                    break;
            }
        }

        ret->placement(compileTransform());

        return ret;
    }

    //---

} // base

