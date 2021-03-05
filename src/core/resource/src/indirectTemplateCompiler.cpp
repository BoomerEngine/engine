/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: object #]
***/

#include "build.h"
#include "indirectTemplate.h"
#include "indirectTemplateCompiler.h"
#include "asyncReferenceType.h"
#include "asyncReference.h"
#include "reference.h"
#include "loader.h"
#include "loadingService.h"

#include "core/object/include/rttiResourceReferenceType.h"

BEGIN_BOOMER_NAMESPACE()

//---

ObjectIndirectTemplateCompiler::ObjectIndirectTemplateCompiler(bool loadImports)
    : m_loadImports(loadImports)
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

static EulerTransform MergeTransforms(const EulerTransform& base, const EulerTransform& cur)
{
    // handle most common case - no transform
    if (base.isIdentity())
        return cur;
    else if (cur.isIdentity())
        return base;

    EulerTransform ret;
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
    if (expectedType->metaType() == MetaType::ResourceRef)
    {
        const auto* refType = static_cast<const IResourceReferenceType*>(expectedType.ptr());
        const auto resourceClass = refType->referenceResourceClass().cast<IResource>();
        if (resourceClass)
        {
            const auto* asyncRefType = CreateAsyncRefType(resourceClass);

            BaseAsyncReference asyncRef;
            if (compileValue(name, asyncRefType, &asyncRef))
            {
                auto* outRef = (BaseReference*)ptr;

                if (asyncRef.empty())
                {
                    *outRef = BaseReference();
                }
                else
                {
                    const auto loaded = m_loadImports ? LoadResource(asyncRef.id()) : nullptr;
                    *outRef = BaseReference(asyncRef.id(), loaded);
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
                if (ConvertData(prop.defaultValue, prop.type, ptr, expectedType))
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

END_BOOMER_NAMESPACE()


