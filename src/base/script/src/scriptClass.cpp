/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"
#include "scriptClass.h"
#include "scriptObject.h"

#include "base/object/include/rttiArrayType.h"
#include "base/object/include/rttiClassType.h"
#include "base/object/include/rttiHandleType.h"

BEGIN_BOOMER_NAMESPACE(base::script)

//---

ScriptedProperty::ScriptedProperty(Type parent, const rtti::PropertySetup& setup)
    : rtti::Property(parent.ptr(), setup)
{
    ASSERT(scripted());
}

ScriptedProperty::~ScriptedProperty()
{}

void ScriptedProperty::bindScriptedPropertyOffset(uint32_t offset)
{
    m_offset = offset;
}

//---

ScriptedClassProperty::ScriptedClassProperty(Type parent, const rtti::PropertySetup& setup)
    : ScriptedProperty(parent, setup)
{
    ASSERT(scripted());
    ASSERT(externalBuffer());
}

ScriptedClassProperty::~ScriptedClassProperty()
{}

/*const void* ScriptedClassProperty::offsetPtr(const void* data) const
{
    auto obj  = (const ScriptedObject*) data;
    return OffsetPtr(obj->m_scriptPropertiesData, offset());
}

void* ScriptedClassProperty::offsetPtr(void* data) const
{
    auto obj  = (ScriptedObject*) data;
    return OffsetPtr(obj->m_scriptPropertiesData, offset());
}*/

//---

ScriptedClass::ScriptedClass(StringID name, ClassType nativeClass)
    : rtti::IClassType(name, nativeClass->size(), nativeClass->alignment(), POOL_SCRIPTED_OBJECT)
    , m_nativeClass(nativeClass)
    , m_scriptedDataSize(0)
    , m_scriptedDatAlignment(1)
    , m_scriptedAbstract(false)
    , m_defaultObject(nullptr)
    , m_functionCtor(nullptr)
    , m_functionDtor(nullptr)
{
    ASSERT(nativeClass->is(ScriptedObject::GetStaticClass()));
    m_traits.scripted = true;
}

const void* ScriptedClass::createDefaultObject() const
{
    auto obj = m_nativeClass->createDefaultObject();
    return obj;
}

void ScriptedClass::destroyDefaultObject() const
{
    m_nativeClass->destroyDefaultObject();
}

const void* ScriptedClass::defaultObject() const
{
    if (m_defaultObject == nullptr)
        m_defaultObject = (void*)createDefaultObject();

    return m_defaultObject;
}

bool ScriptedClass::isAbstract() const
{
    return m_scriptedAbstract;
}

void ScriptedClass::destroyScriptedObject(void* object) const
{
    // call destructor
    if (m_functionDtor)
    {
        rtti::FunctionCallingParams params;
        m_functionDtor->run(nullptr, object, params);
    }

    // destroy scripted properties
    auto obj  = (ScriptedObject*) object;
    if (obj->m_scriptPropertiesData != nullptr)
    {
        mem::FreeBlock(obj->m_scriptPropertiesData);
        obj->m_scriptPropertiesData = nullptr;
    }
}

void ScriptedClass::construct(void *object) const
{
    // initialize the native object, calls the constructor of the native type
    m_nativeClass->construct(object);

    // initialize object class binding
    auto obj  = (ScriptedObject*) object;
    ASSERT(obj->m_scriptedClass == ScriptedObject::GetStaticClass());
    ASSERT( is(ScriptedObject::GetStaticClass()) );
    obj->m_scriptedClass = this;
    ASSERT( obj->is(this));

    // create scripted data
    ASSERT(obj->m_scriptPropertiesData == nullptr);
    if (m_scriptedDataSize > 0)
    {
        // allocate block for the script properties in the scripted pool
        auto data = mem::AllocateBlock(POOL_SCRIPTED_OBJECT, m_scriptedDataSize, m_scriptedDatAlignment, name().c_str());
        memzero(data, m_scriptedDataSize);
        obj->m_scriptPropertiesData = data;
    }

    // call the script constructor on the object
    //TRACE_INFO("Script: Created scripted class '{}'", name());
    if (m_functionCtor)
    {
        rtti::FunctionCallingParams params;
        m_functionCtor->run(nullptr, obj, params);
    }
}

void ScriptedClass::destruct(void *object) const
{
    // destroy the scripted part of the object
    destroyScriptedObject(object);

    // destroy the native object
    m_nativeClass->destruct(object);
}

bool ScriptedClass::compare(const void* data1, const void* data2) const
{
    ASSERT(!"Scripted classes cannot be compared directly");
    return false;
}

void ScriptedClass::copy(void* dest, const void* src) const
{
    ASSERT(!"Scripted classes cannot be copied directly");
}

void ScriptedClass::markAsAbstract()
{
    m_scriptedAbstract = true;
}

void ScriptedClass::clearForReloading()
{
    m_scriptedDataSize = 0;
    m_scriptedDatAlignment = 1;
    m_scriptedAbstract = false;
    m_baseClass = nullptr;
    m_functionDtor = nullptr;
    m_functionCtor = nullptr;

    m_allProperties.reset();
    m_allPropertiesCached = false;

    m_allFunctions.reset();
    m_allFunctionsCached = false;

    while (!m_localProperties.empty())
    {
        auto prop  = m_localProperties.back();
        if (!prop->scripted())
            break;
        m_localProperties.popBack();
    }

    while (!m_localFunctions.empty())
    {
        auto func  = m_localFunctions.back();
        if (!func->scripted())
            break;
        m_localFunctions.popBack();
    }
}

bool ScriptedClass::updateScriptedDataSize()
{
    uint32_t newSize = 0;
    uint32_t newAlignment = 1;

    if (m_baseClass && m_baseClass->scripted())
    {
        auto scriptedBaseClass  = static_cast<const ScriptedClass*>(m_baseClass);
        newSize = scriptedBaseClass->m_scriptedDataSize;
        newAlignment = scriptedBaseClass->m_scriptedDatAlignment;
    }

    for (auto prop  : localProperties())
    {
        ASSERT(prop->scripted())

        auto propAlignment = prop->type()->alignment();
        auto propSize = prop->type()->size();

        auto offset = base::Align(newSize, propAlignment);

        auto scriptedProp  = static_cast<ScriptedProperty*>(const_cast<rtti::Property*>(prop));
        scriptedProp->bindScriptedPropertyOffset(offset);

        newAlignment = std::max(newAlignment, propAlignment);
        newSize = offset + propSize;
    }

    // do not update if not required
    if (newSize == m_scriptedDataSize && newAlignment == m_scriptedDatAlignment)
        return false;

    // update size
    TRACE_INFO("Scripted class '{}' {}->{} (align {})", name(), m_scriptedDataSize, newSize, newAlignment);
    m_scriptedDataSize = newSize;
    m_scriptedDatAlignment = newAlignment;

    // changed, another pass may be required
    return true;
}

void ScriptedClass::bindFunctions()
{
    m_functionCtor = findFunction("__ctor"_id);
    m_functionDtor = findFunction("__dtor"_id);
}

//---

ScriptedStruct::ScriptedStruct(StringID name)
    : rtti::IClassType(name, 0, 1, POOL_SCRIPTED_OBJECT)
    , m_defaultObject(nullptr)
    , m_functionCtor(nullptr)
    , m_functionDtor(nullptr)
{
    m_traits.scripted = true;
}

const void* ScriptedStruct::createDefaultObject() const
{
    auto ret = mem::AllocateBlock(POOL_SCRIPTED_DEFAULT_OBJECT, size(), alignment(), name().c_str());
    memzero(ret, size());

    if (m_functionCtor)
    {
        rtti::FunctionCallingParams params;
        m_functionCtor->run(nullptr, ret, params);
    }

    return ret;
}

const void* ScriptedStruct::defaultObject() const
{
    if (m_defaultObject == nullptr)
        m_defaultObject = (void*)createDefaultObject();

    return m_defaultObject;
}

void ScriptedStruct::destroyDefaultObject() const
{
    // TODO
}

bool ScriptedStruct::isAbstract() const
{
    return false;
}

void ScriptedStruct::construct(void *object) const
{
    if (m_functionCtor)
    {
        rtti::FunctionCallingParams params;
        m_functionCtor->run(nullptr, object, params);
    }
}

void ScriptedStruct::destruct(void *object) const
{
    if (m_functionDtor)
    {
        rtti::FunctionCallingParams params;
        m_functionDtor->run(nullptr, object, params);
    }
}

bool ScriptedStruct::compare(const void* data1, const void* data2) const
{
    if (m_traits.simpleCopyCompare)
    {
        return 0 == memcmp(data1, data2, size());
    }
    else
    {
        for (auto &prop : m_compareCopyList)
        {
            auto propData1 = OffsetPtr(data1, prop.offset);
            auto propData2 = OffsetPtr(data2, prop.offset);
            if (!prop.type->compare(propData1, propData2))
                return false;
        }

        return true;
    }
}

void ScriptedStruct::copy(void* dest, const void* src) const
{
    if (m_traits.simpleCopyCompare)
    {
        memcpy(dest, src, size());
    }
    else
    {
        for (auto &prop : m_compareCopyList)
        {
            auto propSrc = OffsetPtr(src, prop.offset);
            auto propDest = OffsetPtr(dest, prop.offset);
            prop.type->copy(propDest, propSrc);
        }
    }
}

bool ScriptedStruct::updateDataSize()
{
    uint32_t newSize = 0;
    uint32_t newAlign = 1;

    for (auto prop  : localProperties())
    {
        ASSERT(prop->scripted());

        auto propAlignment = prop->type()->alignment();
        auto propSize = prop->type()->size();

        auto offset = base::Align(newSize, propAlignment);

        auto scriptedProp  = static_cast<ScriptedProperty*>(const_cast<rtti::Property*>(prop));
        scriptedProp->bindScriptedPropertyOffset(offset);

        newAlign = std::max(newAlign, propAlignment);
        newSize = offset + propSize;
    }

    if ((m_traits.size == newSize) && (m_traits.alignment == newAlign))
        return false;

    TRACE_INFO("Scripted struct '{}' {}->{} (align {})", name(), m_traits.size, newSize, newAlign);
    m_traits.size = newSize;
    m_traits.alignment = newAlign;

    bool hasAnyPropertyWithCopyOrCompare = false;
    for (auto prop  : localProperties())
        hasAnyPropertyWithCopyOrCompare |= !prop->type()->traits().simpleCopyCompare;

    if (hasAnyPropertyWithCopyOrCompare)
    {
        m_traits.simpleCopyCompare = false;
        for (auto prop  : localProperties())
            m_compareCopyList.pushBack(prop);
    }
    else
    {
        m_traits.simpleCopyCompare = true;
    }

    m_traits.requiresConstructor = false;
    m_traits.requiresDestructor = false;
    m_traits.initializedFromZeroMem = true;

    for (auto prop  : localProperties())
    {
        m_traits.requiresConstructor |= prop->type()->traits().requiresConstructor;
        m_traits.requiresDestructor |= prop->type()->traits().requiresDestructor;
        m_traits.initializedFromZeroMem &= prop->type()->traits().initializedFromZeroMem;
    }

    return true;
}

void ScriptedStruct::clearForReloading()
{
    m_traits.size = 0;
    m_traits.alignment = 1;

    m_allProperties.reset();
    m_allPropertiesCached = false;

    m_allFunctions.reset();
    m_allFunctionsCached = false;

    m_localProperties.reset();
    m_localFunctions.reset();

    m_functionCtor = nullptr;
    m_functionDtor = nullptr;
}

void ScriptedStruct::bindFunctions()
{
    m_functionCtor = findFunction("__ctor"_id);
    m_functionDtor = findFunction("__dtor"_id);
}

END_BOOMER_NAMESPACE(base::script)
