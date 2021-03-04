/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "core/object/include/rttiClassType.h"
#include "core/object/include/rttiProperty.h"
#include "core/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//----

// scripted property
class CORE_SCRIPT_API ScriptedProperty : public Property
{
public:
    ScriptedProperty(Type parent, const PropertySetup& setup);
    virtual ~ScriptedProperty();

    // bind offset to scripted property
    void bindScriptedPropertyOffset(uint32_t offset);
};

//----

// scripted property
class CORE_SCRIPT_API ScriptedClassProperty : public ScriptedProperty
{
public:
    ScriptedClassProperty(Type parent, const PropertySetup& setup);
    virtual ~ScriptedClassProperty();

    /// get property data given the pointer to the object
    //virtual const void* offsetPtr(const void* data) const override final;
    //virtual void* offsetPtr(void* data) const override final;
};

//----

// scripted class type that derives from native type
class CORE_SCRIPT_API ScriptedClass : public IClassType
{
public:
    ScriptedClass(StringID name, ClassType nativeClass);

    /// IClassType
    virtual const void* createDefaultObject() const  override final;
    virtual const void* defaultObject() const override final;
    virtual void destroyDefaultObject() const override final;
    virtual bool isAbstract() const override final;
    virtual void construct(void *object) const override final;
    virtual void destruct(void *object) const override final;
    virtual bool compare(const void* data1, const void* data2) const override final;
    virtual void copy(void* dest, const void* src) const override final;

    /// destroy scripted only data
    void destroyScriptedObject(void* object) const;

    /// update size of the script data in class, returns true if size changed
    bool updateScriptedDataSize();

    /// mark as abstract
    void markAsAbstract();

    /// clear class for script reloading
    void clearForReloading();

    /// bind internal functions
    void bindFunctions();

private:
    ClassType m_nativeClass;
    uint32_t m_scriptedDataSize;
    uint32_t m_scriptedDatAlignment;
    bool m_scriptedAbstract;

    mutable void* m_defaultObject;

    const Function* m_functionCtor;
    const Function* m_functionDtor;
};

//----

// scripted struct
class CORE_SCRIPT_API ScriptedStruct : public IClassType
{
public:
    ScriptedStruct(StringID name);

    /// IClassType
    virtual const void* createDefaultObject() const  override final;
    virtual const void* defaultObject() const override final;
    virtual void destroyDefaultObject() const override final;
    virtual bool isAbstract() const override final;
    virtual void construct(void *object) const override final;
    virtual void destruct(void *object) const override final;
    virtual bool compare(const void* data1, const void* data2) const override final;
    virtual void copy(void* dest, const void* src) const override final;

    /// try to compute size and alignment, returns true if size changes
    bool updateDataSize();

    /// clear class for script reloading
    void clearForReloading();

    /// bind internal functions
    void bindFunctions();

public:
    mutable void* m_defaultObject;

    const Function* m_functionCtor;
    const Function* m_functionDtor;

    struct PropEntry
    {
        uint32_t offset = 0;
        Type type = nullptr;

        INLINE PropEntry(const Property* prop)
            : offset(prop->offset()), type(prop->type())
        {}
    };

    InplaceArray<PropEntry, 32> m_compareCopyList;
};

//--

END_BOOMER_NAMESPACE_EX(script)
